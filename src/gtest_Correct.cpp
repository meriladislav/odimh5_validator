#include <iostream>
#include <string>
#include <cstdlib>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "module_Correct.hpp"
#include "module_Compare.hpp"

using namespace testing;
using namespace myodim;

static long int fileSize(const std::string& fName);

const std::string TEST_IN_FILE = "./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf";
const std::string TEST_OUT_FILE = "./out/T_PAGZ41_C_LZIB_20180403000000.hdf";
const std::string TEST_CSV_FILE = "./data/ODIM_H5_V2_1_PVOL.csv";
const std::string CSV_TO_CORRECT = "./data/example/failed_entries.csv";

TEST(testRepair, canCopyHdf5File) {
  std::remove(TEST_OUT_FILE.c_str());

  ASSERT_NO_THROW( copyFile(TEST_IN_FILE, TEST_OUT_FILE) );
  FILE* f = fopen(TEST_OUT_FILE.c_str(), "r");
  ASSERT_THAT( f, NotNull() );
  fclose(f);

  ASSERT_THAT( fileSize(TEST_IN_FILE), Eq(fileSize(TEST_OUT_FILE)) );
}

TEST(testRepair, canRepairReal64AttributeDataTypes) {
  std::remove(TEST_OUT_FILE.c_str());

  printInfo = false;

  H5Layout h5LayIn(TEST_IN_FILE);
  OdimStandard oStand(TEST_CSV_FILE);
  const bool checkOptional = true; // - some optional entries are not full compliant
  const bool checkExtras = false;
  OdimStandard failedEntries;
  ASSERT_FALSE( compare(h5LayIn, oStand, checkOptional, checkExtras, &failedEntries) );
  ASSERT_FALSE( failedEntries.entries.empty() );
  ASSERT_THAT( failedEntries.entries.size(), Eq(4u) );
  OdimStandard toCorrect(CSV_TO_CORRECT);

  ASSERT_NO_THROW( correct(TEST_IN_FILE, TEST_OUT_FILE, toCorrect) );

  H5Layout h5LayOut(TEST_OUT_FILE);
  OdimStandard failedEntriesOut;
  ASSERT_TRUE( compare(h5LayOut, oStand, checkOptional, checkExtras, &failedEntriesOut) );
  ASSERT_TRUE( failedEntriesOut.entries.empty() );
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
