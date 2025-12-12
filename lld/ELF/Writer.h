//===- Writer.h -------------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLD_ELF_WRITER_H
#define LLD_ELF_WRITER_H

#include "Config.h"

namespace lld::elf {
class OutputSection;
void copySectionsIntoPartitions(Ctx &ctx);
template <class ELFT> void writeResult(Ctx &ctx);

void addReservedSymbols(Ctx &ctx);
bool includeInSymtab(Ctx &, const Symbol &);
unsigned getSectionRank(Ctx &, OutputSection &osec);

uint8_t getMipsIsaExt(uint64_t oldExt, llvm::StringRef oldFile, uint64_t newExt,
                      llvm::StringRef newFile);
void checkMipsShlibCompatible(Ctx &ctx, InputFile *f, uint64_t shlibCheriFlags,
                              uint64_t targetCheriFlags);
bool isCheriAbi(const InputFile *f);
bool isRelroSection(Ctx &ctx, const OutputSection *sec,
                    bool ignoreZRelro = false);

} // namespace lld::elf

#endif
