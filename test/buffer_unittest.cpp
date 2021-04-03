// =============================================================================
// Created by yangb on 2021/4/2.
// =============================================================================

#include "gtest/gtest.h"
#include "../src/buffer/buffer.h"

TEST(TestBuffer, testAppendRetrieve) {
  Buffer buf;
  EXPECT_EQ(buf.ReadableBytes(), 0);
  EXPECT_EQ(buf.PrependableBytes(), 0);

  const std::string str(200, 'x');
  buf.Append(str);
  EXPECT_EQ(buf.ReadableBytes(), str.size());
}
