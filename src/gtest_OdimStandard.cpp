#include <iostream>
#include <string>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "class_OdimStandard.hpp"

using namespace testing;
using namespace myodim;

const std::string TEST_CSV_FILE = "./data/ODIM_H5_V2_1_PVOL.csv";
const std::string UPDATE_CSV_FILE = "./data/example/T_PAGZ41_C_LZIB.values.interval.csv";
const std::string WRONG_CSV_FILE = "./data/ODIM_H5_V2_1_PVOL.csvx";

TEST(testOdimStandard, isEmptyWhenDefaultConstructed) {
  OdimStandard oStand;

  ASSERT_TRUE( oStand.entries.empty() );
}

TEST(testOdimStandard, canReadFromCSV) {
  OdimStandard oStand;

  ASSERT_NO_THROW( oStand.readFromCsv(TEST_CSV_FILE) );
  ASSERT_FALSE( oStand.entries.empty() );
  ASSERT_ANY_THROW( oStand.readFromCsv(WRONG_CSV_FILE) );
}

TEST(testOdimStandard, canConstructByCSV) {
  ASSERT_NO_THROW( OdimStandard oStand(TEST_CSV_FILE) );
  ASSERT_ANY_THROW( OdimStandard oStand(WRONG_CSV_FILE) );
}

TEST(testOdimStandard, canUpdatByAnotherCSV) {
  OdimStandard oStand(TEST_CSV_FILE);

  ASSERT_NO_THROW( oStand.updateWithCsv(UPDATE_CSV_FILE) );
}
