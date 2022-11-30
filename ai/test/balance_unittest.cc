// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Tests of CancellationFlag class.

#include <gtest/gtest.h>
#include <iostream>

#include "nrai/core.h"

namespace nrai {

TEST(BalanceTest, SimpleTest) {
  // unitset for sum
  Balance<int> balance;
  ASSERT_EQ(-1, balance.x());
  ASSERT_EQ(1, balance.y());
  ASSERT_EQ(2, balance.connection());

  balance.grow();
  ASSERT_EQ(-2, balance.x());
  ASSERT_EQ(2, balance.y());
  ASSERT_EQ(4, balance.connection());

  balance.set_coeff(100);
  balance.grow();
  ASSERT_EQ(-102, balance.x());
  ASSERT_EQ(102, balance.y());
  ASSERT_EQ(204, balance.connection());

  balance.swap();
  ASSERT_EQ(102, balance.x());
  ASSERT_EQ(-102, balance.y());
  ASSERT_EQ(-204, balance.connection());

  balance.die();
  ASSERT_EQ(-1, balance.x());
  ASSERT_EQ(1, balance.y());
  ASSERT_EQ(2, balance.connection());

  // unittest for product
  Balance<double> balance2(2, BT_PROD);
  ASSERT_EQ(1, balance2.x());
  ASSERT_EQ(1, balance2.y());
  ASSERT_EQ(1, balance2.connection());

  balance2.grow();
  ASSERT_EQ(0.5, balance2.x());
  ASSERT_EQ(2, balance2.y());
  ASSERT_EQ(4, balance2.connection());

  balance2.set_coeff(4);
  balance2.grow();
  ASSERT_EQ(0.125, balance2.x());
  ASSERT_EQ(8, balance2.y());
  ASSERT_EQ(64, balance2.connection());

  balance2.swap();
  ASSERT_EQ(8, balance2.x());
  ASSERT_EQ(0.125, balance2.y());
  ASSERT_EQ(0.015625, balance2.connection());

  balance2.die();
  ASSERT_EQ(1, balance2.x());
  ASSERT_EQ(1, balance2.y());
  ASSERT_EQ(1, balance2.connection());
}

TEST(BalanceTest, ComplexTest) {
  ASSERT_FALSE(false);
  ASSERT_TRUE(true);
}

} // namespace nrai
