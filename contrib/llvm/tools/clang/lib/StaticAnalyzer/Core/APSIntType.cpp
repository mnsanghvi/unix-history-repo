//===--- APSIntType.cpp - Simple record of the type of APSInts ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/StaticAnalyzer/Core/PathSensitive/APSIntType.h"

using namespace clang;
using namespace ento;

APSIntType::RangeTestResultKind
APSIntType::testInRange(const llvm::APSInt &Value) const {
  // Negative numbers cannot be losslessly converted to unsigned type.
  if (IsUnsigned && Value.isSigned() && Value.isNegative())
    return RTR_Below;

  // Signed integers can be converted to signed integers of the same width
  // or (if positive) unsigned integers with one fewer bit.
  // Unsigned integers can be converted to unsigned integers of the same width
  // or signed integers with one more bit.
  unsigned MinBits;
  if (Value.isSigned())
    MinBits = Value.getMinSignedBits() - IsUnsigned;
  else
    MinBits = Value.getActiveBits() + !IsUnsigned;

  if (MinBits <= BitWidth)
    return RTR_Within;

  if (Value.isSigned() && Value.isNegative())
    return RTR_Below;
  else
    return RTR_Above;
}
