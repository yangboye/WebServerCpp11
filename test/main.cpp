// =============================================================================
// Created by yangb on 2021/4/2.
// =============================================================================

#include "gtest/gtest.h"

int Add(int x, int y) {
  return x+y;
}

TEST(AddCase, test1) {
  EXPECT_EQ(Add(1, 2), 3);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}