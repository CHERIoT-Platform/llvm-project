#include "Cheri.h"

#include "../InputFiles.h"
#include "../OutputSections.h"
#include "../SymbolTable.h"
#include "../SyntheticSections.h"
#include "../Target.h"
#include "../Writer.h"
#include "lld/Common/CommonLinkerContext.h"
#include "lld/Common/Memory.h"
#include "llvm/Support/Path.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::ELF;

namespace lld {
namespace elf {

bool hasDynamicLinker(Ctx &ctx) {
  return ctx.arg.shared || ctx.arg.pie || !ctx.sharedFiles.empty();
}

// See CheriBSD crt_init_globals()
template <class ELFT>
struct InMemoryCapRelocEntry {
  static constexpr size_t fieldSize = ELFT::Is64Bits ? 8 : 4;
  static constexpr size_t relocSize = fieldSize * 5;
  using NativeUint = typename ELFT::uint;
  using CapRelocUint = llvm::support::detail::packed_endian_specific_integral<
      NativeUint, ELFT::Endianness, llvm::support::aligned>;
  InMemoryCapRelocEntry(NativeUint loc, NativeUint obj, NativeUint off,
                        NativeUint s, NativeUint perms)
      : capability_location(loc),
        object(obj),
        offset(off),
        size(s),
        permissions(perms) {}
  CapRelocUint capability_location;
  CapRelocUint object;
  CapRelocUint offset;
  CapRelocUint size;
  CapRelocUint permissions;
};

CheriCapRelocsSection::CheriCapRelocsSection(Ctx &ctx)
    : SyntheticSection(ctx, "__cap_relocs", SHT_PROGBITS,
                       (ctx.arg.isPic && !ctx.arg.relativeCapRelocsOnly)
                           ? SHF_ALLOC | SHF_WRITE /* XXX: actually RELRO */
                           : SHF_ALLOC,
                       ctx.arg.wordsize) {
  this->entsize = ctx.arg.wordsize * 5;
}

SymbolAndOffset SymbolAndOffset::fromSectionWithOffset(
    Ctx &ctx, InputSectionBase *isec, int64_t offset,
    const SymbolAndOffset *Default) {
  Symbol *fallbackResult = nullptr;
  assert((int64_t)offset >= 0);
  int64_t fallbackOffset = offset;
  // For internal symbols we don't have a matching InputFile, just return
  auto *file = isec->file;
  if (!file || file->isInternal()) {
    if (Default) return *Default;
    return {isec, offset};
  }
  for (Symbol *b : isec->file->getSymbols()) {
    if (auto *d = dyn_cast<Defined>(b)) {
      if (d->section != isec) continue;
      if ((int64_t)d->value <= offset &&
          offset <= (int64_t)d->value + (int64_t)d->getSize(ctx)) {
        // XXXAR: should we accept any symbol that encloses or only exact
        // matches?
        if ((int64_t)d->value == offset && (d->isFunc() || d->isObject()))
          return {d, 0};  // perfect match
        fallbackResult = d;
        fallbackOffset = offset - d->value;
      }
    }
  }
  // When using the legacy __cap_relocs style (where clang emits __cap_relocs
  // instead of R_CHERI_CAPABILITY) the local symbols might not exist so we
  // may have fall back to the section.
  if (!fallbackResult) {
    // worst case we fall back to the section + offset
    // Don't warn if the relocation is against an anonymous string constant
    // since clang won't emit a symbol (and no size) for those
    if (!isec->name.starts_with(".rodata.str")) {
      Warn(ctx) << "Could not find a real symbol for " << isec->name << "+0x"
                << utohexstr(offset) << " in " << toStr(ctx, isec->file);
    }
    // Could not find a symbol -> return section+offset
    assert(offset == fallbackOffset);
    if (Default) return *Default;
    return {isec, offset};
  }
  return {fallbackResult, fallbackOffset};
}

SymbolAndOffset SymbolAndOffset::findRealSymbol(Ctx &ctx) const {
  // If we only have a section, see if we can resolve section+offset to a
  // symbol (that may have been added later).
  if (auto *ISB = dyn_cast<InputSectionBase *>(symOrSec)) {
    return SymbolAndOffset::fromSectionWithOffset(ctx, ISB, offset, this);
  }
  auto s = cast<Symbol *>(symOrSec);
  if (!s->isSection()) return *this;
  if (Defined *definedSym = dyn_cast<Defined>(s)) {
    if (auto *isec = dyn_cast<InputSectionBase>(definedSym->section)) {
      return SymbolAndOffset::fromSectionWithOffset(ctx, isec, offset, this);
    }
  }
  return *this;
}

SymbolAndOffset SymbolAndOffset::findSymbolForCapabilityRelocation(
    Ctx &ctx) const {
  // In some cases we can end up with a symbol+offset that points to an
  // assembler local symbol without a type or size.
  // This happens e.g. for exception landing pads that are a relocation
  // against "fnstart+(lpad-fnstart)", but when building with RISC-V -mrelax,
  // the addend value may not be a constant, so the compiler must emit a
  // relocation against `lpad` instead which does not have a type or size
  // information. For such STT_NOTYPE symbols, we identify the surrounding one.
  // NOTE: This must only be called after linker relaxations are processed.
  Defined *self = dyn_cast<Defined>(sym());
  if (!self) return *this;
  assert(self->getSize(ctx) == 0);
  assert(self->type == STT_NOTYPE);
  Defined *bestMatch = self;
  uint64_t bestSize = bestMatch->getSize(ctx);
  const uint64_t targetValue = self->value + offset;
  auto foundBetterMatch = [&](Defined *newMatch) {
    int64_t oldOffset = targetValue - bestMatch->value;
    int64_t newOffset = targetValue - newMatch->value;
    Log(ctx) << "Found better match for capability relocation against "
             << toStr(ctx, *bestMatch) << "+" << Twine(oldOffset) << ": "
             << toStr(ctx, *newMatch) << "+" << Twine(newOffset);
    bestMatch = newMatch;
    bestSize = bestMatch->getSize(ctx);
  };
  if (auto *isec = dyn_cast_or_null<InputSectionBase>(self->section)) {
    if (!isec->file) return *this;
    for (Symbol *b : isec->file->getSymbols()) {
      if (auto *d = dyn_cast<Defined>(b)) {
        if (d->section != isec) continue;
        uint64_t dsize = d->getSize(ctx);
        if (d->value <= targetValue && targetValue < d->value + dsize) {
          // Try to find a better match (with a size/type). The match is
          // considered better if
          //  - The current match has no size, and the new one does
          //  - The current match is preemptible, and the new one isn't
          //  - The current match has a smaller (non-zero) size
          //  - The current match has no type, and the new one is func/object
          if (d->type != STT_FUNC && d->type != STT_OBJECT) continue;
          if (bestSize == 0 && dsize != 0)
            foundBetterMatch(d);
          else if (bestMatch->isPreemptible && !d->isPreemptible)
            foundBetterMatch(d);
          else if (dsize != 0 && dsize < bestSize)
            foundBetterMatch(d);
          else if (bestMatch->type == STT_NOTYPE)
            foundBetterMatch(d);
        }
      }
    }
  }
  return {bestMatch, (int64_t)(targetValue - bestMatch->value)};
}

std::string CheriCapRelocLocation::toString(Ctx &ctx) const {
  return SymbolAndOffset(section, offset).verboseToString(ctx);
}

template <class ELFT>
void CheriCapRelocsSection::processSection(InputSectionBase *s) {
  // TODO: sort by offset (or is that always true?
  const auto rels = s->relsOrRelas<ELFT>().relas;
  for (auto i = rels.begin(), end = rels.end(); i != end; ++i) {
    const auto &locationRel = *i;
    ++i;
    const auto &targetRel = *i;
    if ((locationRel.r_offset % entsize) != 0) {
      error(
          "corrupted __cap_relocs:  expected Relocation offset to be a "
          "multiple of " +
          Twine(entsize) + " but got " + Twine(locationRel.r_offset));
      return;
    }
    constexpr unsigned fieldSize = InMemoryCapRelocEntry<ELFT>::fieldSize;
    if (targetRel.r_offset != locationRel.r_offset + fieldSize) {
      error("corrupted __cap_relocs: expected target relocation (" +
            Twine(targetRel.r_offset) +
            " to directly follow location relocation (" +
            Twine(locationRel.r_offset) + ")");
      return;
    }
    if (locationRel.r_addend < 0) {
      Msg(ctx) << "corrupted __cap_relocs: addend is less than zero in" << *s
               << ": " << Twine(locationRel.r_addend);
      return;
    }
    uint64_t capRelocsOffset = locationRel.r_offset;
    assert(capRelocsOffset + entsize <= s->getSize());
    if (ctx.arg.emachine == EM_MIPS) {
      if (locationRel.getType(ctx.arg.isMips64EL) != R_MIPS_64) {
        error("Exptected a R_MIPS_64 relocation in __cap_relocs but got " +
              toString(locationRel.getType(ctx.arg.isMips64EL)));
        continue;
      }
      if (targetRel.getType(ctx.arg.isMips64EL) != R_MIPS_64) {
        error("Exptected a R_MIPS_64 relocation in __cap_relocs but got " +
              toString(targetRel.getType(ctx.arg.isMips64EL)));
        continue;
      }
    } else {
      if (locationRel.getType(ctx.arg.isMips64EL) !=
          *ctx.target->absPointerRel) {
        error(
            "Exptected an absolute pointer relocation in __cap_relocs "
            "but got " +
            toString(locationRel.getType(ctx.arg.isMips64EL)));
        continue;
      }
      if (targetRel.getType(ctx.arg.isMips64EL) != *ctx.target->absPointerRel) {
        error(
            "Exptected an absolute pointer relocation in __cap_relocs "
            "but got " +
            toString(targetRel.getType(ctx.arg.isMips64EL)));
        continue;
      }
    }
    Symbol *locationSym = &s->getFile<ELFT>()->getRelocTargetSym(locationRel);
    Symbol &targetSym = s->getFile<ELFT>()->getRelocTargetSym(targetRel);

    if (locationSym->file != s->file) {
      Err(ctx) << "Expected capability relocation to point to " << *s->file
               << " but got " << *locationSym->file;
      continue;
    }
    //    errs() << "Adding cap reloc at " << toString(LocationSym) << " type "
    //           << Twine((int)LocationSym.Type) << " against "
    //           << toString(TargetSym) << "\n";
    auto *rawInput = reinterpret_cast<const InMemoryCapRelocEntry<ELFT> *>(
        s->content().begin() + capRelocsOffset);
    int64_t targetCapabilityOffset = (int64_t)rawInput->offset;
    assert(rawInput->size == 0 &&
           "Clang should not have set size in __cap_relocs");
    if (!isa<Defined>(locationSym)) {
      error("Unhandled symbol kind for cap_reloc: " +
            Twine(locationSym->kind()));
      continue;
    }

    const SymbolAndOffset relocLocation{locationSym, locationRel.r_addend};
    const SymbolAndOffset relocTarget{&targetSym, targetRel.r_addend};
    SymbolAndOffset realLocation = relocLocation.findRealSymbol(ctx);
    SymbolAndOffset realTarget = relocTarget.findRealSymbol(ctx);
    if (ctx.arg.verboseCapRelocs) {
      message("Adding capability relocation at " +
              realLocation.verboseToString(ctx) + "\nagainst " +
              realTarget.verboseToString(ctx));
    }

    bool targetNeedsDynReloc = false;
    if (targetSym.isPreemptible) {
      // Do we need this?
      // TargetNeedsDynReloc = true;
    }
    switch (targetSym.kind()) {
      case Symbol::DefinedKind:
        break;
      case Symbol::SharedKind:
        if (!hasDynamicLinker(ctx)) {
          error(
              "cannot create a capability relocation against a shared symbol"
              " when linking statically");
          continue;
        }
        targetNeedsDynReloc = true;
        break;
      case Symbol::UndefinedKind:
        // addCapReloc() will add an error if we are building an executable
        // instead of a shlib
        // TODO: we really should add a dynamic SIZE relocation as well
        targetNeedsDynReloc = true;
        break;
      default:
        error("Unhandled symbol kind for cap_reloc target: " +
              Twine(targetSym.kind()));
        continue;
    }
    assert(locationSym->isSection());
    auto *locationDef = cast<Defined>(locationSym);
    auto *locationSec = cast<InputSectionBase>(locationDef->section);
    addCapReloc<ELFT>({locationSec, (uint64_t)locationRel.r_addend}, realTarget,
                      targetNeedsDynReloc, targetCapabilityOffset,
                      realLocation.sym());
  }
}

template <class ELFT>
void CheriCapRelocsSection::addCapReloc(CheriCapRelocLocation loc,
                                        const SymbolAndOffset &target,
                                        bool targetNeedsDynReloc,
                                        int64_t capabilityOffset,
                                        Symbol *sourceSymbol) {
  if (ctx.arg.relativeCapRelocsOnly) {
    if (targetNeedsDynReloc) {
      error(
          "Cannot add __cap_reloc against target that needs a dynamic "
          "relocation when --relative-cap-relocs is enabled: " +
          target.verboseToString(ctx));
      return;
    }
  } else {
    // Allow relocations in __cap_relocs section for legacy mode
    targetNeedsDynReloc = targetNeedsDynReloc || ctx.arg.isPic || ctx.arg.pie;
  }
  uint64_t currentEntryOffset = relocsMap.size() * entsize;

  auto sourceMsg = [&]() -> std::string {
    return sourceSymbol ? verboseToString(ctx, sourceSymbol) : loc.toString(ctx);
  };
  if (target.sym()->isUndefined() && !target.sym()->isUndefWeak()) {
    auto diag = (ctx.arg.unresolvedSymbols == UnresolvedPolicy::ReportError)
                    ? Err(ctx)
                : (errorHandler().fatalWarnings) ? Msg(ctx)
                                                 : Warn(ctx);
    if (errorHandler().fatalWarnings) {
      diag << "warning: ";
    }
    diag << "cap_reloc against undefined symbol: " << toStr(ctx, *target.sym())
         << "\n>>> referenced by " << sourceMsg();
  }

  // assert(CapabilityOffset >= 0 && "Negative offsets not supported");
  if (errorHandler().verbose && capabilityOffset < 0)
    message("global capability offset " + Twine(capabilityOffset) +
            " is less than 0:\n>>> Location: " + loc.toString(ctx) +
            "\n>>> Target: " + target.verboseToString(ctx));

  bool canWriteLoc = (loc.section->flags & SHF_WRITE) || !ctx.arg.zText;
  if (!canWriteLoc) {
    readOnlyCapRelocsError(ctx, *target.sym(),
                           "\n>>> referenced by " + sourceMsg());
    return;
  }

  if (!addEntry(loc, {target, capabilityOffset, targetNeedsDynReloc})) {
    return;  // Maybe happens with vtables?
  }
  if (targetNeedsDynReloc) {
    bool relativeToLoadAddress = false;
    // The addend is not used as the offset into the capability here, as we
    // have the offset field in the __cap_relocs for that. The Addend
    // will be zero unless we are targeting a string constant as these
    // don't have a symbol and will be like .rodata.str+0x1234
    int64_t addend = target.offset;
    constexpr unsigned fieldSize = InMemoryCapRelocEntry<ELFT>::fieldSize;
    // Capability target is the second field
    if (target.sym()->isPreemptible) {
      ctx.mainPart->relaDyn->addSymbolReloc(
          *ctx.target->absPointerRel, *this, currentEntryOffset + fieldSize,
          *target.sym(), addend, ctx.target->symbolicRel);
    } else {
      // If the target is not preemptible we can optimize this to a relative
      // relocation against the image base.
      relativeToLoadAddress = true;
      ctx.mainPart->relaDyn->addRelativeReloc(
          ctx.target->relativeRel, *this, currentEntryOffset + fieldSize,
          *target.sym(), addend, ctx.target->symbolicRel, R_ABS);
    }
    assert((currentEntryOffset + fieldSize) < getSize());
    containsDynamicRelocations = true;
    if (!relativeToLoadAddress) {
      // We also add a size relocation for the size field here
      RelType sizeRel = *ctx.target->sizeRel;
      // Capability size is the fourth field
      assert((currentEntryOffset + 3 * fieldSize) < getSize());
      ctx.mainPart->relaDyn->addSymbolReloc(
          sizeRel, *this, currentEntryOffset + 3 * fieldSize, *target.sym());
    }
  }
}

template <typename ELFT>
static uint64_t getTargetSize(Ctx &ctx, const CheriCapRelocLocation &location,
                              const SymbolAndOffset &target) {
  uint64_t targetSize = target.sym()->getSize(ctx);
  if (targetSize > INT_MAX) {
    error("Insanely large symbol size for " + target.verboseToString(ctx) +
          "for cap_reloc at" + location.toString(ctx));
    return 0;
  }
  auto targetSym = target.sym();
  if (targetSize == 0 && !targetSym->isPreemptible) {
    StringRef name = targetSym->getName();
    // Section end symbols like __preinit_array_end, etc. should actually be
    // zero size symbol since they are just markers for the end of a section
    // and not usable as a valid pointer
    if (isSectionEndSymbol(name) || isSectionStartSymbol(name))
      return targetSize;

    bool isAbsoluteSym = targetSym->getOutputSection() == nullptr;
    // Symbols previously declared as weak can have size 0 (if they then resolve
    // to NULL). For example __preinit_array_start, etc. are generated by the
    // linker as ABS symbols with value 0.
    // A symbol is linker-synthesized/linker script generated if File == nullptr
    if (isAbsoluteSym && targetSym->file == nullptr) return targetSize;

    if (targetSym->isUndefWeak() && targetSym->getVA(ctx, 0) == 0)
      // Weak symbol resolved to NULL -> zero size is fine
      return 0;

    // Absolute value provided by -defsym or assignment in .o file is fine
    if (isAbsoluteSym) return targetSize;

    // Otherwise warn about missing sizes for symbols
    bool warnAboutUnknownSize = true;
    // currently clang doesn't emit the necessary symbol information for local
    // string constants such as: struct config_opt opts[] = { { ..., "foo" },
    // { ..., "bar" } }; As this pattern is quite common don't warn if the
    // target section is .rodata.str
    if (Defined *definedSym = dyn_cast<Defined>(targetSym)) {
      if (definedSym->isSection() &&
          definedSym->section->name.starts_with(".rodata.str")) {
        warnAboutUnknownSize = false;
      }
    }
    // TODO: are there any other cases that can be ignored?

    bool UnknownSectionSize = true;
    if (OutputSection *os = targetSym->getOutputSection()) {
      // Cast must succeed since getOutputSection() returned non-NULL
      Defined *def = cast<Defined>(targetSym);
      // warn("Could not find size for symbol " + target.verboseToString() +
      //    " and could not determine section size. Using 0.");
      if ((int64_t)def->value < 0 || def->value > os->size) {
        // Note: we allow def->value == os->size for pointers to the end
        warn("Symbol " + target.verboseToString(ctx) +
             " is defined as being in section " + os->name +
             " but the value (0x" + utohexstr(targetSym->getVA(ctx)) +
             ") is outside this section. Will create a zero-size capability."
             "\n>>> referenced by " +
             location.toString(ctx));
        return 0;
      }
      // For negative offsets use 0 instead (we want the range of the full
      // symbol in that case)
      int64_t offset = std::max((int64_t)0, target.offset);
      uint64_t targetVA = targetSym->getVA(ctx, offset);
      assert(targetVA >= os->addr);
      uint64_t offsetInOS = targetVA - os->addr;
      // Check this isn't a symbol defined outside a section in a linker script.
      // Use less-or-equal here to account for __end_foo symbols which point 1
      // past the section
      if (offsetInOS <= os->size) {
        targetSize = os->size - offsetInOS;
        UnknownSectionSize = false;
      }
    }
    if (warnAboutUnknownSize || errorHandler().verbose) {
      std::string msg = "could not determine size of cap reloc against " +
                        target.verboseToString(ctx) + "\n>>> referenced by " +
                        location.toString(ctx);
      warn(msg);
    }
    if (UnknownSectionSize) {
      warn("Could not find size for symbol " + target.verboseToString(ctx) +
           " and could not determine section size. Using 0.");
      // TargetSize = std::numeric_limits<uint64_t>::max();
      return 0;
    }
  }
  return targetSize;
}

template <class ELFT>
struct CaptablePermissions {
  static const uint64_t function = UINT64_C(1)
                                   << ((sizeof(typename ELFT::uint) * 8) - 1);
  static const uint64_t readOnly = UINT64_C(1)
                                   << ((sizeof(typename ELFT::uint) * 8) - 2);
};

template <class ELFT>
void CheriCapRelocsSection::writeToImpl(uint8_t *buf) {
  assert(entsize == sizeof(InMemoryCapRelocEntry<ELFT>) &&
         "cap relocs size mismatch");
  uint64_t offset = 0;
  for (const auto &i : relocsMap) {
    const CheriCapRelocLocation &location = i.first;
    const CheriCapReloc &reloc = i.second;
    assert(location.offset <= location.section->getSize());

    // If we are targeting a symbol that does not have type and size
    // information, try to find the intended target symbol. This can happen for
    // relocations against a label inside a function or a subobject.
    SymbolAndOffset realTarget = reloc.target;
    if (Symbol *s = reloc.target.symOrSec.dyn_cast<Symbol *>())
      if (s->type == STT_NOTYPE && s->getSize(ctx) == 0)
        realTarget = reloc.target.findSymbolForCapabilityRelocation(ctx);

    // We write the virtual address of the location in both static and the
    // shared library case:
    // In the static case we can compute the final virtual address and write it
    // In the dynamic case we write the virtual address relative to the load
    // address and the runtime linker will add the load address to that
    uint64_t outSecOffset = location.section->getOffset(location.offset);
    uint64_t locationVA =
        location.section->getOutputSection()->addr + outSecOffset;

    // The target VA is the base address of the capability, so symbol + 0
    uint64_t targetVA = realTarget.sym()->getVA(ctx, 0);
    bool preemptibleDynReloc =
        reloc.needsDynReloc && realTarget.sym()->isPreemptible;
    uint64_t targetSize = 0;
    if (preemptibleDynReloc) {
      // If we have a relocation against a preemptible symbol (even in the
      // current DSO) we can't compute the virtual address here so we only write
      // the addend
      if (realTarget.offset != 0)
        error(
            "Dyn Reloc Target offset was nonzero: " + Twine(realTarget.offset) +
            " - " + realTarget.verboseToString(ctx));
      targetVA = realTarget.offset;
    } else {
      // For non-preemptible symbols we can write the target size:
      targetSize = getTargetSize<ELFT>(ctx, location, realTarget);
    }
    uint64_t targetOffset = reloc.capabilityOffset + realTarget.offset;
    uint64_t permissions = 0;
    // Fow now Function implies ReadOnly so don't add the flag
    if (realTarget.sym()->isFunc()) {
      permissions |= CaptablePermissions<ELFT>::function;
    } else if (auto os = realTarget.sym()->getOutputSection()) {
      assert(!realTarget.sym()->isTls());
      // if ((OS->getPhdrFlags() & PF_W) == 0) {
      if (((os->flags & SHF_WRITE) == 0) || isRelroSection(ctx, os)) {
        permissions |= CaptablePermissions<ELFT>::readOnly;
      } else if (os->flags & SHF_EXECINSTR) {
#if 0
        // This generates a load of annoying spurious warnings with CHERIoT.
        // It should be reintroduced but conditional on some flag that we can
        // use to disable it.
        warn("Non-function __cap_reloc against symbol in section with "
             "SHF_EXECINSTR (" +
             toString(os->name) + ") for symbol " +
             realTarget.verboseToString());
#endif
      }
    }

    // TODO: should we warn about symbols that are out-of-bounds?
    // mandoc seems to do it so I guess we need it
    // if (TargetOffset < 0 || TargetOffset > TargetSize) warn(...);

    InMemoryCapRelocEntry<ELFT> entry(locationVA, targetVA, targetOffset,
                                      targetSize, permissions);
    memcpy(buf + offset, &entry, sizeof(entry));
    //     if (errorHandler().verbose) {
    //       errs() << "Added capability reloc: loc=" << utohexstr(LocationVA)
    //              << ", object=" << utohexstr(TargetVA)
    //              << ", offset=" << utohexstr(TargetOffset)
    //              << ", size=" << utohexstr(TargetSize)
    //              << ", permissions=" << utohexstr(Permissions) << "\n";
    //     }
    offset += InMemoryCapRelocEntry<ELFT>::relocSize;
  }

  // FIXME: this totally breaks dynamic relocs!!! need to do in finalize()

  // Sort the cap_relocs by target address for better cache and TLB locality
  // It also makes it much easier to read the llvm-objdump -C output since it
  // is sorted in a sensible order
  // However, we can't do this if we added any dynamic relocations since it
  // will mean the dynamic relocation offset refers to a different location
  // FIXME: do the sorting in finalizeSection instead
  if (ctx.arg.sortCapRelocs && !containsDynamicRelocations)
    std::stable_sort(
        reinterpret_cast<InMemoryCapRelocEntry<ELFT> *>(buf),
        reinterpret_cast<InMemoryCapRelocEntry<ELFT> *>(buf + offset),
        [](const InMemoryCapRelocEntry<ELFT> &a,
           const InMemoryCapRelocEntry<ELFT> &b) {
          return a.capability_location < b.capability_location;
        });
  assert(offset == getSize() && "Not all data written?");
}

void CheriCapRelocsSection::writeTo(uint8_t *buf) {
  invokeELFT(writeToImpl, buf);
}

CheriCapTableSection::CheriCapTableSection(Ctx &ctx)
    : SyntheticSection(
          ctx, ".captable", SHT_PROGBITS,
          SHF_ALLOC | SHF_WRITE, /* XXX: actually RELRO for BIND_NOW*/
          ctx.arg.capabilitySize) {
  assert(ctx.arg.capabilitySize > 0);
  this->entsize = ctx.arg.capabilitySize;
}

void CheriCapTableSection::writeTo(uint8_t *buf) {
  // Capability part should be filled with all zeros and crt_init_globals fills
  // it in. For the TLS part, assignValuesAndAddCapTableSymbols adds any static
  // relocations needed, and should be procesed by relocateAlloc.
  // TODO: Fill in the raw capability bits and use CBuildCap
  ctx.target->relocateAlloc(*this, buf);
}

static Defined *findMatchingFunction(const InputSectionBase *isec,
                                     uint64_t symOffset) {
  return isec->getEnclosingFunction(symOffset);
}

CheriCapTableSection::CaptableMap &
CheriCapTableSection::getCaptableMapForFileAndOffset(
    const InputSectionBase *isec, uint64_t offset) {
  if (LLVM_LIKELY(ctx.arg.capTableScope == CapTableScopePolicy::All))
    return globalEntries;
  if (ctx.arg.capTableScope == CapTableScopePolicy::File) {
    // operator[] will insert if missing
    return perFileEntries[isec->file];
  }
  if (ctx.arg.capTableScope == CapTableScopePolicy::Function) {
    Symbol *func = findMatchingFunction(isec, offset);
    if (!func) {
      Warn(ctx) << "Could not find corresponding function with per-function "
                   "captable: "
                << isec->getObjMsg(offset);
    }
    // operator[] will insert if missing
    return perFunctionEntries[func];
  }
  llvm_unreachable("INVALID CONFIG OPTION");
  return globalEntries;
}

void CheriCapTableSection::addEntry(Symbol &sym, RelExpr expr,
                                    InputSectionBase *isec, uint64_t offset) {
  // FIXME: can this be called from multiple threads?
  CapTableIndex idx;
  idx.needsSmallImm = false;
  idx.usedInCallExpr = false;
  idx.firstUse = SymbolAndOffset(isec, offset);
  assert(!idx.firstUse->symOrSec.isNull());
  switch (expr) {
    case R_CHERI_CAPABILITY_TABLE_INDEX_SMALL_IMMEDIATE:
    case R_CHERI_CAPABILITY_TABLE_INDEX_CALL_SMALL_IMMEDIATE:
      idx.needsSmallImm = true;
      break;
    default:
      break;
  }
  // If the symbol is only ever referenced by the captable call relocations we
  // can emit a capability call relocation instead of a normal capability
  // relocation. This indicates to the runtime linker that the capability is
  // not used as a function pointer and therefore does not need a unique
  // address (plt stub) across all DSOs.
  switch (expr) {
    case R_CHERI_CAPABILITY_TABLE_INDEX_CALL:
    case R_CHERI_CAPABILITY_TABLE_INDEX_CALL_SMALL_IMMEDIATE:
      if (!sym.isFunc() && !sym.isUndefWeak()) {
        CheriCapRelocLocation loc{isec, offset};
        std::string msg = "call relocation against non-function symbol " +
                          verboseToString(ctx, &sym, 0) + "\n>>> referenced by " +
                          loc.toString(ctx);
        if (sym.isUndefined() &&
            ctx.arg.unresolvedSymbolsInShlib == UnresolvedPolicy::Ignore) {
          // Don't fail the build for shared libraries unless
          nonFatalWarning(msg);
        } else {
          warn(msg);
        }
      }
      idx.usedInCallExpr = true;
      break;
    default:
      break;
  }
  CaptableMap &entries = getCaptableMapForFileAndOffset(isec, offset);
  if (ctx.arg.zCapTableDebug) {
    // Add a local helper symbol to improve disassembly:
    StringRef helperSymName = saver().save(
        "$captable_load_" +
        (sym.getName().empty() ? "$anonymous_symbol" : sym.getName()));
    addSyntheticLocal(ctx, helperSymName, STT_NOTYPE, offset, 0, *isec);
  }

  auto it = entries.map.insert(std::make_pair(&sym, idx));
  if (!it.second) {
    // If it is references by a small immediate relocation we need to update
    // the small immediate flag
    if (idx.needsSmallImm) it.first->second.needsSmallImm = true;
    // If one of the uses is not a call expression, we need to emit a unique
    // function pointer and reuse that for the call expression, too.
    // TODO: should we emit two relocations instead?
    if (!idx.usedInCallExpr) it.first->second.usedInCallExpr = false;
  }
}

void CheriCapTableSection::addDynTlsEntry(Symbol &sym) {
  dynTlsEntries.map.insert(std::make_pair(&sym, CapTableIndex()));
}

void CheriCapTableSection::addTlsIndex() {
  dynTlsEntries.map.insert(std::make_pair(nullptr, CapTableIndex()));
}

void CheriCapTableSection::addTlsEntry(Symbol &sym) {
  tlsEntries.map.insert(std::make_pair(&sym, CapTableIndex()));
}

uint32_t CheriCapTableSection::getIndex(const Symbol &sym,
                                        const InputSectionBase *isec,
                                        uint64_t offset) const {
  assert(valuesAssigned && "getIndex called before index assignment");
  const CaptableMap &entries =
      const_cast<CheriCapTableSection *>(this)->getCaptableMapForFileAndOffset(
          isec, offset);
  auto it = entries.map.find(const_cast<Symbol *>(&sym));
  assert(entries.firstIndex != std::numeric_limits<uint64_t>::max() &&
         "First index not set yet?");
  assert(it != entries.map.end());
  // The index that is written as part of the relocation is relative to the
  // start of the current captable subset (or the global table in the default
  // case). When using per-function tables the first index in every function
  // will always be zero.
  return *it->second.index - entries.firstIndex;
}

uint32_t CheriCapTableSection::getDynTlsOffset(const Symbol &sym) const {
  assert(valuesAssigned && "getDynTlsOffset called before index assignment");
  auto it = dynTlsEntries.map.find(const_cast<Symbol *>(&sym));
  assert(it != dynTlsEntries.map.end());
  return *it->second.index * ctx.arg.wordsize;
}

uint32_t CheriCapTableSection::getTlsIndexOffset() const {
  assert(valuesAssigned && "getTlsIndexOffset called before index assignment");
  auto it = dynTlsEntries.map.find(nullptr);
  assert(it != dynTlsEntries.map.end());
  return *it->second.index * ctx.arg.wordsize;
}

uint32_t CheriCapTableSection::getTlsOffset(const Symbol &sym) const {
  assert(valuesAssigned && "getTlsOffset called before index assignment");
  auto it = tlsEntries.map.find(const_cast<Symbol *>(&sym));
  assert(it != tlsEntries.map.end());
  return *it->second.index * ctx.arg.wordsize;
}

template <class ELFT>
uint64_t CheriCapTableSection::assignIndices(uint64_t startIndex,
                                             CaptableMap &entries,
                                             const Twine &symContext) {
  // Usually StartIndex will be zero (one global captable) but if we are
  // compiling with per-file/per-function
  uint64_t smallEntryCount = 0;
  assert(entries.firstIndex == std::numeric_limits<uint64_t>::max() &&
         "Should not be initialized yet!");
  entries.firstIndex = startIndex;
  for (auto &it : entries.map) {
    // TODO: looping twice is inefficient, we could keep track of the number of
    // small entries during insertion
    if (it.second.needsSmallImm) {
      smallEntryCount++;
    }
  }

  if (ctx.arg.emachine == EM_MIPS) {
    unsigned maxSmallEntries = (1 << 19) / ctx.arg.capabilitySize;
    if (smallEntryCount > maxSmallEntries) {
      // Use warn here since the calculation may be wrong if the 11 bit clc is
      // used. We will error when writing the relocation values later anyway
      // so this will help find the error
      warn("added " + Twine(smallEntryCount) +
           " entries to .captable but "
           "current maximum is " +
           Twine(maxSmallEntries) +
           "; try recompiling "
           "non-performance critical source files with -mxcaptable");
    }
    if (errorHandler().verbose) {
      message("Total " + Twine(entries.size()) + " .captable entries: " +
              Twine(smallEntryCount) + " use a small immediate and " +
              Twine(entries.size() - smallEntryCount) + " use -mxcaptable. ");
    }
  }

  // Only add the @CAPTABLE symbols when running the LLD unit tests
  // errorHandler().exitEarly is set to false if LLD_IN_TEST=1 so just reuse
  // that instead of calling getenv on every iteration
  const bool shouldAddAtCaptableSymbols = !errorHandler().exitEarly;
  uint32_t assignedSmallIndexes = 0;
  uint32_t assignedLargeIndexes = 0;
  for (auto &it : entries.map) {
    CapTableIndex &cti = it.second;
    if (cti.needsSmallImm) {
      assert(assignedSmallIndexes < smallEntryCount);
      cti.index = startIndex + assignedSmallIndexes;
      assignedSmallIndexes++;
    } else {
      cti.index = startIndex + smallEntryCount + assignedLargeIndexes;
      assignedLargeIndexes++;
    }

    uint32_t index = *cti.index;
    assert(index >= startIndex && index < startIndex + entries.size());
    Symbol *targetSym = it.first;

    StringRef name = targetSym->getName();
    // Avoid duplicate symbol name errors for unnamed string constants:
    StringRef refName;
    // For now always append .INDEX to local symbols @CAPTABLE names since they
    // might not be unique. If there is a global with the same name we always
    // want the global to have the plain @CAPTABLE name
    if (name.empty() /* || Name.starts_with(".L") */ || targetSym->isLocal())
      refName =
          saver().save(name + "@CAPTABLE" + symContext + "." + Twine(index));
    else
      refName = saver().save(name + "@CAPTABLE" + symContext);
    // XXXAR: This should no longer be necessary now that I am using
    // addSyntheticLocal?
#if 0
    if (Symtab->find(RefName)) {
      std::string NewRefName =
          (Name + "@CAPTABLE" + SymContext + "." + Twine(Index)).str();
      // XXXAR: for some reason we sometimes create more than one cap table entry
      // for a given global name, for now just rename the symbol
      // assert(TargetSym->isLocal());
      if (!TargetSym->isLocal()) {
        error("Found duplicate global captable ref name " + RefName +
              " but referenced symbol was not local\n>>> " +
              verboseToString(TargetSym));
      } else {
        // TODO: make this a warning
        message("Found duplicate captable name " + RefName +
                "\n>>> Replacing with " + NewRefName);
      }
      RefName = std::move(NewRefName);
      assert(!Symtab->find(RefName) && "RefName should be unique");
    }
#endif
    uint64_t off = index * ctx.arg.capabilitySize;
    if (shouldAddAtCaptableSymbols) {
      addSyntheticLocal(ctx, refName, STT_OBJECT, off, ctx.arg.capabilitySize,
                        *this);
    }
    // If the symbol is used as a function pointer the runtime linker has to
    // ensure that all pointers to that function compare equal. This is done
    // by ensuring that they all point to the same PLT stub.
    // If it is not used as a function pointer we can use a capability call
    // relocation instead which allows the runtime linker to create non-unique
    // plt stubs.
    RelType elfCapabilityReloc = it.second.usedInCallExpr
                                     ? *ctx.target->cheriCapCallRel
                                     : *ctx.target->cheriCapRel;
    // All capability call relocations should end up in the pltrel section
    // rather than the normal relocation section to make processing of PLT
    // relocations in RTLD more efficient.
    RelocationBaseSection *dynRelSec = it.second.usedInCallExpr
                                           ? ctx.in.relaPlt.get()
                                           : ctx.mainPart->relaDyn.get();
    addCapabilityRelocation<ELFT>(
        ctx, targetSym, elfCapabilityReloc, ctx.in.cheriCapTable.get(), off,
        R_CHERI_CAPABILITY, 0, it.second.usedInCallExpr,
        [&]() {
          return ("\n>>> referenced by " + refName + "\n>>> first used in " +
                  it.second.firstUse->verboseToString(ctx))
              .str();
        },
        dynRelSec);
  }
  assert(assignedSmallIndexes + assignedLargeIndexes == entries.size());
  return assignedSmallIndexes + assignedLargeIndexes;
}

template <class ELFT>
void CheriCapTableSection::assignValuesAndAddCapTableSymbols() {
  // First assign the global indices (which will usually be the only ones)
  uint64_t assignedEntries = assignIndices<ELFT>(0, globalEntries, "");
  if (LLVM_UNLIKELY(ctx.arg.capTableScope != CapTableScopePolicy::All)) {
    assert(assignedEntries == 0 &&
           "Should not have any global entries in"
           " per-file/per-function captable mode");
    for (auto &it : perFileEntries) {
      std::string fullContext = toStr(ctx, it.first);
      auto lastSlash = StringRef(fullContext).find_last_of("/\\") + 1;
      StringRef context = StringRef(fullContext).substr(lastSlash);
      assignedEntries +=
          assignIndices<ELFT>(assignedEntries, it.second, "@" + context);
    }
    for (auto &it : perFunctionEntries)
      assignedEntries += assignIndices<ELFT>(assignedEntries, it.second,
                                             "@" + toStr(ctx, *it.first));
  }
  assert(assignedEntries == nonTlsEntryCount());

  uint32_t assignedTlsIndexes = 0;
  uint32_t tlsBaseIndex =
      assignedEntries * (ctx.arg.capabilitySize / ctx.arg.wordsize);

  // TODO: support TLS for per-function captable
  if (ctx.arg.capTableScope != CapTableScopePolicy::All &&
      (!dynTlsEntries.empty() || !tlsEntries.empty())) {
    error("TLS is not supported yet with per-file or per-function captable");
    return;
  }

  for (auto &it : dynTlsEntries.map) {
    CapTableIndex &cti = it.second;
    assert(!cti.needsSmallImm);
    cti.index = tlsBaseIndex + assignedTlsIndexes;
    assignedTlsIndexes += 2;
    Symbol *s = it.first;
    uint64_t offset = *cti.index * ctx.arg.wordsize;
    if (s == nullptr) {
      if (!ctx.arg.shared)
        this->relocations.push_back(
            {R_ADDEND, ctx.target->symbolicRel, offset, 1, s});
      else
        ctx.mainPart->relaDyn->addReloc(
            {ctx.target->tlsModuleIndexRel, this, offset});
    } else {
      // When building a shared library we still need a dynamic relocation
      // for the module index. Therefore only checking for
      // s->isPreemptible is not sufficient (this happens e.g. for
      // thread-locals that have been marked as local through a linker script)
      if (!s->isPreemptible && !ctx.arg.shared)
        this->relocations.push_back(
            {R_ADDEND, ctx.target->symbolicRel, offset, 1, s});
      else
        ctx.mainPart->relaDyn->addSymbolReloc(ctx.target->tlsModuleIndexRel,
                                              *this, offset, *s);

      offset += ctx.arg.wordsize;

      // However, we can skip writing the TLS offset reloc for non-preemptible
      // symbols since it is known even in shared libraries
      if (!s->isPreemptible)
        this->relocations.push_back(
            {R_ABS, ctx.target->tlsOffsetRel, offset, 0, s});
      else
        ctx.mainPart->relaDyn->addSymbolReloc(ctx.target->tlsOffsetRel, *this,
                                              offset, *s);
    }
  }

  for (auto &it : tlsEntries.map) {
    CapTableIndex &cti = it.second;
    assert(!cti.needsSmallImm);
    cti.index = tlsBaseIndex + assignedTlsIndexes++;
    Symbol *s = it.first;
    uint64_t offset = *cti.index * ctx.arg.wordsize;
    // When building a shared library we still need a dynamic relocation
    // for the TP-relative offset as we don't know how much other data will
    // be allocated before us in the static TLS block.
    if (!s->isPreemptible && !ctx.arg.shared)
      this->relocations.push_back(
          {R_TPREL, ctx.target->symbolicRel, offset, 0, s});
    else
      // FIXME: casting to GotSection here is a massive hack!!
      ctx.mainPart->relaDyn->addAddendOnlyRelocIfNonPreemptible(
          ctx.target->tlsGotRel, *this, offset, *s, ctx.target->symbolicRel);
  }

  valuesAssigned = true;
}

template void
CheriCapTableSection::assignValuesAndAddCapTableSymbols<ELF32LE>();
template void
CheriCapTableSection::assignValuesAndAddCapTableSymbols<ELF32BE>();
template void
CheriCapTableSection::assignValuesAndAddCapTableSymbols<ELF64LE>();
template void
CheriCapTableSection::assignValuesAndAddCapTableSymbols<ELF64BE>();

CheriCapTableMappingSection::CheriCapTableMappingSection(Ctx &ctx)
    : SyntheticSection(ctx, ".captable_mapping", SHT_PROGBITS, SHF_ALLOC, 8) {
  assert(ctx.arg.capabilitySize > 0);
  this->entsize = sizeof(CaptableMappingEntry);
  static_assert(sizeof(CaptableMappingEntry) == 24, "");
}

size_t CheriCapTableMappingSection::getSize() const {
  assert(ctx.arg.capTableScope != CapTableScopePolicy::All);
  if (!isNeeded()) return 0;
  size_t count = 0;
  if (!ctx.in.symTab) {
    error("Cannot use " + this->name + " without .symtab section!");
    return 0;
  }
  for (const SymbolTableEntry &ste : ctx.in.symTab->getSymbols()) {
    if (!ste.sym->isDefined() || !ste.sym->isFunc()) continue;
    count++;
  }
  return count * sizeof(CaptableMappingEntry);
}

void CheriCapTableMappingSection::writeTo(uint8_t *buf) {
  assert(ctx.arg.capTableScope != CapTableScopePolicy::All);
  if (!ctx.in.cheriCapTable) return;
  if (!ctx.in.symTab) {
    error("Cannot write " + this->name + " without .symtab section!");
    return;
  }

  // Write the mapping from function vaddr -> captable subset for RTLD
  std::vector<CaptableMappingEntry> entries;
  // Note: Symtab->getSymbols() only returns the symbols in .dynsym. We need
  // to use In.sym()tab instead since we also want to add all local functions!
  for (const SymbolTableEntry &ste : ctx.in.symTab->getSymbols()) {
    Symbol *sym = ste.sym;
    if (!sym->isDefined() || !sym->isFunc()) continue;
    const CheriCapTableSection::CaptableMap *capTableMap = nullptr;
    if (ctx.arg.capTableScope == CapTableScopePolicy::Function) {
      auto it = ctx.in.cheriCapTable->perFunctionEntries.find(sym);
      if (it != ctx.in.cheriCapTable->perFunctionEntries.end())
        capTableMap = &it->second;
    } else if (ctx.arg.capTableScope == CapTableScopePolicy::File) {
      auto it = ctx.in.cheriCapTable->perFileEntries.find(sym->file);
      if (it != ctx.in.cheriCapTable->perFileEntries.end())
        capTableMap = &it->second;
    } else {
      llvm_unreachable("Invalid mode!");
    }
    CaptableMappingEntry entry;
    entry.funcStart = sym->getVA(ctx, 0);
    entry.funcEnd = entry.funcStart + sym->getSize(ctx);
    if (capTableMap) {
      assert(capTableMap->firstIndex != std::numeric_limits<uint64_t>::max());
      entry.capTableOffset = capTableMap->firstIndex * ctx.arg.capabilitySize;
      entry.subTableSize = capTableMap->size() * ctx.arg.capabilitySize;
    } else {
      // TODO: don't write an entry for functions that don't use the captable
      entry.capTableOffset = 0;
      entry.subTableSize = 0;
    }
    entries.push_back(entry);
  }
  // Sort all the entries so that RTLD can do a binary search to find the
  // correct entry instead of having to scan all of them.
  // Do this before swapping to target endianess to simplify the comparisons.
  llvm::sort(entries, [](const CaptableMappingEntry &e1,
                         const CaptableMappingEntry &e2) {
    if (e1.funcStart == e2.funcStart) return e1.funcEnd < e2.funcEnd;
    return e1.funcStart < e2.funcStart;
  });
  // Byte-swap all the values so that we can memcpy the sorted buffer
  for (CaptableMappingEntry &e : entries) {
    e.funcStart = support::endian::byte_swap(e.funcStart, ctx.arg.endianness);
    e.funcEnd = support::endian::byte_swap(e.funcEnd, ctx.arg.endianness);
    e.capTableOffset =
        support::endian::byte_swap(e.capTableOffset, ctx.arg.endianness);
    e.subTableSize =
        support::endian::byte_swap(e.subTableSize, ctx.arg.endianness);
  }
  assert(entries.size() * sizeof(CaptableMappingEntry) == getSize());
  memcpy(buf, entries.data(), entries.size() * sizeof(CaptableMappingEntry));
}

static bool isSymIncludedInDynsym(Ctx &ctx, const Symbol &sym) {
  if (sym.computeBinding(ctx) == STB_LOCAL) return false;
  if (!sym.isDefined() && !sym.isCommon()) return true;

  return sym.isExported ||
         (ctx.arg.exportDynamic && (sym.isUsedInRegularObj || !sym.ltoCanOmit));
}


template <typename ELFT>
void addCapabilityRelocation(Ctx &ctx, Symbol *sym, RelType type,
                             InputSectionBase *sec, uint64_t offset,
                             RelExpr expr, int64_t addend, bool isCallExpr,
                             llvm::function_ref<std::string()> referencedBy,
                             RelocationBaseSection *dynRelSec) {
  assert(expr == R_CHERI_CAPABILITY);
  if (sec->name == ".gcc_except_table" && sym->isPreemptible) {
    // We previously had an ugly workaround here to create a hidden alias for
    // relocations in the exception table, but this has since been fixed in
    // the compiler. Add an explicit error here in case someone tries to
    // link against object files/static libraries from an old toolchain.
    auto diag = ctx.arg.noinhibitExec ? Warn(ctx) : Err(ctx);
    diag << "got relocation against preemptible symbol " << toStr(ctx, *sym)
         << " in exception handling table. Please recompile this file!\n"
         << ">>> referenced by " << sec->getObjMsg(offset);
  }

  // Emit either the legacy __cap_relocs section or a R_CHERI_CAPABILITY reloc
  // For local symbols we can also emit the untagged capability bits and
  // instruct csu/rtld to run CBuildCap
  CapRelocsMode capRelocMode = sym->isPreemptible
                                   ? ctx.arg.preemptibleCapRelocsMode
                                   : ctx.arg.localCapRelocsMode;
  bool needTrampoline = false;
  // In the PLT ABI (and fndesc?) we have to use an elf relocation for function
  // pointers to ensure that the runtime linker adds the required trampolines
  // that sets $cgp:
  if (!isCallExpr && ctx.arg.emachine == llvm::ELF::EM_MIPS && sym->isFunc()) {
    if (!lld::elf::hasDynamicLinker(ctx)) {
      // In static binaries we do not need PLT stubs for function pointers since
      // all functions share the same $cgp
      // TODO: this is no longer true if we were to support dlopen() in static
      // binaries
      if (ctx.arg.verboseCapRelocs)
        Msg(ctx) << "Do not need function pointer trampoline for "
                 << toStr(ctx, *sym) << " in static binary";
      needTrampoline = false;
    } else if (ctx.in.mipsAbiFlags) {
      auto abi = static_cast<MipsAbiFlagsSection<ELFT> &>(*ctx.in.mipsAbiFlags)
                     .getCheriAbiVariant();
      if (abi && (*abi == llvm::ELF::DF_MIPS_CHERI_ABI_PLT ||
                  *abi == llvm::ELF::DF_MIPS_CHERI_ABI_FNDESC))
        needTrampoline = true;
    }
  }

  if (needTrampoline) {
    capRelocMode = CapRelocsMode::ElfReloc;
    assert(capRelocMode == ctx.arg.preemptibleCapRelocsMode);
    if (ctx.arg.verboseCapRelocs)
      message("Using trampoline for function pointer against " +
              verboseToString(ctx, sym));
  }

  // local cap relocs don't need a Elf relocation with a full symbol lookup:
  if (capRelocMode == CapRelocsMode::ElfReloc) {
    assert((sym->isPreemptible || needTrampoline) &&
           "ELF relocs should not be used for non-preemptible symbols");
    assert((!sym->isLocal() || needTrampoline) &&
           "ELF relocs should not be used for local symbols");
    if (ctx.arg.emachine == llvm::ELF::EM_MIPS && ctx.arg.buildingFreeBSDRtld) {
      Err(ctx) << "relocation " << type << " against " << verboseToString(ctx, sym)
               << " cannot be using when building FreeBSD RTLD"
               << referencedBy();
      return;
    }
    if (!lld::elf::hasDynamicLinker(ctx)) {
      auto diag = Err(ctx);
      diag << "attempting to emit a R_CAPABILITY relocation against ";
      if (sym->getName().empty())
        diag << "local symbol";
      else
        diag << "symbol " << toStr(ctx, *sym);
      diag << " in binary without a dynamic linker; try removing -Wl,-"
           << (sym->isPreemptible ? "preemptible" : "local") << "-caprelocs=elf"
           << referencedBy();
      return;
    }
    assert(ctx.hasDynsym && "Should have been checked in Driver.cpp");
    // We don't use a R_MIPS_CHERI_CAPABILITY relocation for the input but
    // instead need to use an absolute pointer size relocation to write
    // the offset addend
    if (!dynRelSec) dynRelSec = ctx.mainPart->relaDyn.get();
    // in the case that -local-caprelocs=elf is passed we need to ensure that
    // the target symbol is included in the dynamic symbol table
    if (!ctx.mainPart->dynSymTab) {
      error("R_CHERI_CAPABILITY relocations need a dynamic symbol table");
      return;
    }
    if (!isSymIncludedInDynsym(ctx, *sym)) {
      if (!needTrampoline) {
        error(
            "added a R_CHERI_CAPABILITY relocation but symbol not included "
            "in dynamic symbol: " +
            verboseToString(ctx, sym));
        return;
      }
      // Hack: Add a new global symbol with a unique name so that we can use
      // a dynamic relocation against it.
      // TODO: should it be possible to add STB_LOCAL symbols to .dynsymtab?
      Defined *newSym = ctx.symtab->ensureSymbolWillBeInDynsym(sym);
      assert(newSym->isFunc() && "This should only be used for functions");
      assert(newSym->isExported);
      assert(newSym->binding == llvm::ELF::STB_GLOBAL);
      assert(newSym->visibility() == llvm::ELF::STV_HIDDEN);
      sym = newSym;  // Make the relocation point to the newly added symbol
    }
    dynRelSec->addReloc(true, type, *sec, offset, *sym, addend, expr,
        /* Relocation type for the addend = */ ctx.target->symbolicRel);

  } else if (capRelocMode == CapRelocsMode::Legacy) {
    if (ctx.arg.relativeCapRelocsOnly) {
      assert(!sym->isPreemptible);
    }
    ctx.in.capRelocs->addCapReloc<ELFT>({sec, offset}, {sym, 0u},
                                        sym->isPreemptible, addend);
  } else {
    assert(ctx.arg.localCapRelocsMode == CapRelocsMode::CBuildCap);
    error("CBuildCap method not implemented yet!");
  }
}

}  // namespace elf
}  // namespace lld

template void lld::elf::addCapabilityRelocation<ELF32LE>(
    Ctx &ctx, Symbol *, RelType, InputSectionBase *, uint64_t, RelExpr, int64_t,
    bool, llvm::function_ref<std::string()>, RelocationBaseSection *);
template void lld::elf::addCapabilityRelocation<ELF32BE>(
    Ctx &ctx, Symbol *, RelType, InputSectionBase *, uint64_t, RelExpr, int64_t,
    bool, llvm::function_ref<std::string()>, RelocationBaseSection *);
template void lld::elf::addCapabilityRelocation<ELF64LE>(
    Ctx &ctx, Symbol *, RelType, InputSectionBase *, uint64_t, RelExpr, int64_t,
    bool, llvm::function_ref<std::string()>, RelocationBaseSection *);
template void lld::elf::addCapabilityRelocation<ELF64BE>(
    Ctx &ctx, Symbol *, RelType, InputSectionBase *, uint64_t, RelExpr, int64_t,
    bool, llvm::function_ref<std::string()>, RelocationBaseSection *);
