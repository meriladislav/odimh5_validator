#include <iostream>
#include <string>
#include <cstdlib>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "module_Repair.hpp"

using namespace testing;
using namespace myodim;

static long int fileSize(const std::string& fName);

const std::string TEST_IN_FILE = "./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf";
const std::string TEST_OUT_FILE = "./out/T_PAGZ41_C_LZIB_20180403000000.hdf";

TEST(testRepair, canCopyHdf5File) {
  std::remove(TEST_OUT_FILE.c_str());

  ASSERT_NO_THROW( copyFile(TEST_IN_FILE, TEST_OUT_FILE) );
  FILE* f = fopen(TEST_OUT_FILE.c_str(), "r");
  ASSERT_THAT( f, NotNull() );
  fclose(f);

  ASSERT_THAT( fileSize(TEST_IN_FILE), Eq(fileSize(TEST_OUT_FILE)) );
}

TEST(testRepair, canRepairAttributeDataTypes) {
  std::remove(TEST_OUT_FILE.c_str());

  ASSERT_TRUE(false); //TODO - implement
}


//statics

long int fileSize(const std::string& fName) {
  FILE* f = fopen(fName.c_str(), "rb");
  if ( !f ) return -1;
  fseek(f, 0L, SEEK_END);
  auto sz = ftell(f);
  fclose(f);
  return sz;
}
