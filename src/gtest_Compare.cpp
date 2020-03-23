#include <iostream>
#include <string>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "module_Compare.hpp"

using namespace testing;
using namespace myodim;

const std::string TEST_ODIM_FILE = "./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf";
const std::string WRONG_ODIM_FILE = "./data/example/T_PAGZ41_C_LZIB_20180403000000.hdfx";
const std::string TEST_CSV_FILE = "./data/ODIM_H5_V2_1_PVOL.csv";
const std::string UPDATE_CSV_FILE = "./data/example/T_PAGZ41_C_LZIB.values.interval.csv";
const std::string WRONG_CSV_FILE = "./data/ODIM_H5_V2_1_PVOL.csvx";

TEST(testCompare, canCreateCVSFileNameFromH5ayout) {
  H5Layout h5Lay(TEST_ODIM_FILE);

  ASSERT_THAT( getCsvFileNameFrom(h5Lay), StrEq("./data/ODIM_H5_V2_1_PVOL.csv") );
  ASSERT_THAT( getCsvFileNameFrom(h5Lay, "2.3"), StrEq("./data/ODIM_H5_V2_3_PVOL.csv") );
}

TEST(testCompare, compareReturnsTrueWhenIsComplant) {
  H5Layout h5Lay(TEST_ODIM_FILE);
  OdimStandard oStand(TEST_CSV_FILE);
  const bool checkOptional = false;
  const bool checkExtras = false;

  ASSERT_TRUE( compare(h5Lay, oStand, checkOptional, checkExtras) );
}

TEST(testCompare, compareReturnsFalseWhenProblemFound) {
  printInfo = false; // turn-off INFO messages

  H5Layout h5Lay(TEST_ODIM_FILE);
  OdimStandard oStand(TEST_CSV_FILE);
  const bool checkOptional = true; // - some optional entries are not full compliant
  const bool checkExtras = false;

  ASSERT_FALSE( compare(h5Lay, oStand, checkOptional, checkExtras) );
}

TEST(testCompare, isStringWhenAtLeastOneAlphabetcalCharacterPresent) {
  ASSERT_TRUE( isStringValue("123a") );
  ASSERT_FALSE(isStringValue("1.2") );
}

TEST(testCompare, hasDoublePointWorks) {
  ASSERT_FALSE( hasDoublePoint("123") );
  ASSERT_TRUE(hasDoublePoint("1.2") );
}
