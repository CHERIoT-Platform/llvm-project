//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ContainerContainsCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang::tidy::readability {

void ContainerContainsCheck::registerMatchers(MatchFinder *Finder) {
  const auto HasContainsMatchingParamType = hasMethod(
      cxxMethodDecl(isConst(), parameterCountIs(1), returns(booleanType()),
                    hasName("contains"), unless(isDeleted()), isPublic(),
                    hasParameter(0, hasType(hasUnqualifiedDesugaredType(
                                        equalsBoundNode("parameterType"))))));

  const auto CountCall =
      cxxMemberCallExpr(
          argumentCountIs(1),
          callee(cxxMethodDecl(
              hasName("count"),
              hasParameter(0, hasType(hasUnqualifiedDesugaredType(
                                  type().bind("parameterType")))),
              ofClass(cxxRecordDecl(HasContainsMatchingParamType)))))
          .bind("call");

  const auto FindCall =
      cxxMemberCallExpr(
          argumentCountIs(1),
          callee(cxxMethodDecl(
              hasName("find"),
              hasParameter(0, hasType(hasUnqualifiedDesugaredType(
                                  type().bind("parameterType")))),
              ofClass(cxxRecordDecl(HasContainsMatchingParamType)))))
          .bind("call");

  const auto EndCall = cxxMemberCallExpr(
      argumentCountIs(0),
      callee(
          cxxMethodDecl(hasName("end"),
                        // In the matchers below, FindCall should always appear
                        // before EndCall so 'parameterType' is properly bound.
                        ofClass(cxxRecordDecl(HasContainsMatchingParamType)))));

  const auto Literal0 = integerLiteral(equals(0));
  const auto Literal1 = integerLiteral(equals(1));

  const auto StringNpos = anyOf(declRefExpr(to(varDecl(hasName("npos")))),
                                memberExpr(member(hasName("npos"))));

  Finder->addMatcher(
      traverse(TK_AsIs,
               implicitCastExpr(hasImplicitDestinationType(booleanType()),
                                hasSourceExpression(CountCall))
                   .bind("positiveComparison")),
      this);

  const auto PositiveComparison =
      anyOf(allOf(hasOperatorName("!="), hasOperands(CountCall, Literal0)),
            allOf(hasLHS(CountCall), hasOperatorName(">"), hasRHS(Literal0)),
            allOf(hasLHS(Literal0), hasOperatorName("<"), hasRHS(CountCall)),
            allOf(hasLHS(CountCall), hasOperatorName(">="), hasRHS(Literal1)),
            allOf(hasLHS(Literal1), hasOperatorName("<="), hasRHS(CountCall)),
            allOf(hasOperatorName("!="),
                  hasOperands(FindCall, anyOf(EndCall, StringNpos))));

  const auto NegativeComparison =
      anyOf(allOf(hasOperatorName("=="), hasOperands(CountCall, Literal0)),
            allOf(hasLHS(CountCall), hasOperatorName("<="), hasRHS(Literal0)),
            allOf(hasLHS(Literal0), hasOperatorName(">="), hasRHS(CountCall)),
            allOf(hasLHS(CountCall), hasOperatorName("<"), hasRHS(Literal1)),
            allOf(hasLHS(Literal1), hasOperatorName(">"), hasRHS(CountCall)),
            allOf(hasOperatorName("=="),
                  hasOperands(FindCall, anyOf(EndCall, StringNpos))));

  Finder->addMatcher(
      binaryOperation(
          anyOf(allOf(PositiveComparison, expr().bind("positiveComparison")),
                allOf(NegativeComparison, expr().bind("negativeComparison")))),
      this);
}

void ContainerContainsCheck::check(const MatchFinder::MatchResult &Result) {
  // Extract the information about the match
  const auto *Call = Result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
  const auto *PositiveComparison =
      Result.Nodes.getNodeAs<Expr>("positiveComparison");
  const auto *NegativeComparison =
      Result.Nodes.getNodeAs<Expr>("negativeComparison");
  assert((!PositiveComparison || !NegativeComparison) &&
         "only one of PositiveComparison or NegativeComparison should be set");
  const bool Negated = NegativeComparison != nullptr;
  const auto *Comparison = Negated ? NegativeComparison : PositiveComparison;

  // Diagnose the issue.
  auto Diag =
      diag(Call->getExprLoc(), "use 'contains' to check for membership");

  // Don't fix it if it's in a macro invocation. Leave fixing it to the user.
  const SourceLocation FuncCallLoc = Comparison->getEndLoc();
  if (!FuncCallLoc.isValid() || FuncCallLoc.isMacroID())
    return;

  // Create the fix it.
  const auto *Member = cast<MemberExpr>(Call->getCallee());
  Diag << FixItHint::CreateReplacement(
      Member->getMemberNameInfo().getSourceRange(), "contains");
  SourceLocation ComparisonBegin = Comparison->getSourceRange().getBegin();
  SourceLocation ComparisonEnd = Comparison->getSourceRange().getEnd();
  SourceLocation CallBegin = Call->getSourceRange().getBegin();
  SourceLocation CallEnd = Call->getSourceRange().getEnd();
  Diag << FixItHint::CreateReplacement(
      CharSourceRange::getCharRange(ComparisonBegin, CallBegin),
      Negated ? "!" : "");
  Diag << FixItHint::CreateRemoval(CharSourceRange::getTokenRange(
      CallEnd.getLocWithOffset(1), ComparisonEnd));
}

} // namespace clang::tidy::readability
