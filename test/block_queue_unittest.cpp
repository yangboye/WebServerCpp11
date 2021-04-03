// =============================================================================
// Created by yangb on 2021/4/3.
// =============================================================================

#include "gtest/gtest.h"
#include "../src/log/block_queue.h"

TEST(TestBlockQueue, test1) {
  BlockQueue<int> bque(50);
  EXPECT_EQ(bque.capacity(), 50);
  EXPECT_EQ(bque.size(), 0);
  EXPECT_EQ(bque.empty(), true);
  EXPECT_EQ(bque.full(), false);

  bque.push_back(2);
  bque.push_front(1);
  EXPECT_EQ(bque.size(), 2);
  EXPECT_EQ(bque.front(), 1);
  EXPECT_EQ(bque.back(), 2);

  int item = 0;
  bque.pop(item);
  EXPECT_EQ(item, 1);
}