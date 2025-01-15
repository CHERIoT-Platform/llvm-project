//===- unittests/AST/cheri/AttrTests.cpp --- CHERI Attribute tests --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/AST/Attr.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/AttrKinds.h"
#include "clang/Tooling/Tooling.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace clang;

namespace {

using clang::ast_matchers::constantExpr;
using clang::ast_matchers::equals;
using clang::ast_matchers::functionDecl;
using clang::ast_matchers::has;
using clang::ast_matchers::hasDescendant;
using clang::ast_matchers::hasName;
using clang::ast_matchers::integerLiteral;
using clang::ast_matchers::match;
using clang::ast_matchers::selectFirst;
using clang::ast_matchers::stringLiteral;
using clang::ast_matchers::varDecl;
using clang::tooling::buildASTFromCode;
using clang::tooling::buildASTFromCodeWithArgs;

template <typename Attr>
testing::AssertionResult HasAttribute(const FunctionDecl *FD,
                                      const std::string attrName) {
  if (FD->hasAttr<Attr>())
    return testing::AssertionSuccess();

  return testing::AssertionFailure()
         << *FD << " doesn't have a " << attrName << " attribute";
}

TEST(Attr, CHERICompartmentName) {
  // cheri_compartment name is a strange attribute which is both:
  //
  // * a function type attribute: affects the calling convention
  // * a function declaration attribute: marks the function as an compartment
  // entrypoint
  //
  // Thus this attribute is both handled by SemaType and SemaDeclAttr and in
  // order to not output twice a diagnosis we have to handle it specially. The
  // following test ensure we don't skip silently the attribute.

  auto AST = buildASTFromCode(R"cpp(
    #define cheri_compartment(name) __attribute__((cheri_compartment(name)))
    #define default_compartment cheri_compartment("default_compartment")

    void default_compartment      f_00();
    default_compartment void      f_01();
    default_compartment void     *f_03();
    void default_compartment     *f_04();
    void * default_compartment    f_05();
 )cpp");

  {
    for (auto Node : match(functionDecl().bind("fn"), AST->getASTContext())) {
      const FunctionDecl *FD = Node.getNodeAs<FunctionDecl>("fn");

      EXPECT_TRUE(
          HasAttribute<CHERICompartmentNameAttr>(FD, "cheri_compartment"));
    };
  }
}

} // namespace
