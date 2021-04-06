// =============================================================================
// Created by yangb on 2021/4/2.
// =============================================================================

#include "gtest/gtest.h"
#include <typeinfo>

int main(int argc, char** argv) {
//  testing::InitGoogleTest(&argc, argv);
//  return RUN_ALL_TESTS();
  std::cout << "Hello world!" << std::endl;
  std::cout << "File: " << __FILE__ << "\tLine: " << __LINE__ << "\tFunction: " << __FUNCTION__ << std::endl;

  return 0;
}