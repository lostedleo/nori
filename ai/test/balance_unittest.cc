// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Tests of CancellationFlag class.

#include <gtest/gtest.h>
#include <iostream>

#include "nrai/core.h"

namespace nrai {

TEST(BalanceTest, SimpleTest) {
  Balance<int> balance;
  ASSERT_EQ(-1, balance.x());
  ASSERT_EQ(1, balance.y());
  ASSERT_EQ(2, balance.connection());
  ASSERT_FALSE(false);
  ASSERT_TRUE(true);
}

TEST(BalanceTest, ComplexTest) {
  ASSERT_FALSE(false);
  ASSERT_TRUE(true);
}

} // namespace nrai
