#include <iostream>
#include <string>
#include <cstdio> // remove
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "class_OdimStandard.hpp"

using namespace testing;
using namespace myodim;

const std::string TEST_CSV_FILE = "./data/ODIM_H5_V2_1_PVOL.csv";
const std::string UPDATE_CSV_FILE = "./data/example/T_PAGZ41_C_LZIB.values.interval.csv";
const std::string WRONG_CSV_FILE = "./data/ODIM_H5_V2_1_PVOL.csvx";
const std::string TEST_CSV_FILE_WITHOUT_REFS = "./data/test/ODIM_H5_V2_1_PVOL_norefs.csv";
const std::string WRITE_CSV_FILE = "./out/ODIM_H5_V2_1_PVOL_write.csv";

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

TEST(testOdimStandard, canReadFromCSVWithoutReferences) {
  OdimStandard oStand;

  ASSERT_NO_THROW( oStand.readFromCsv(TEST_CSV_FILE_WITHOUT_REFS) );
  ASSERT_FALSE( oStand.entries.empty() );
}

TEST(testOdimStandard, canConstructByCSV) {
  ASSERT_NO_THROW( OdimStandard oStand(TEST_CSV_FILE) );
  ASSERT_ANY_THROW( OdimStandard oStand(WRONG_CSV_FILE) );
}

TEST(testOdimStandard, canUpdatByAnotherCSV) {
  OdimStandard oStand(TEST_CSV_FILE);

  ASSERT_NO_THROW( oStand.updateWithCsv(UPDATE_CSV_FILE) );
}

TEST(testOdimStandard, canWriteToCSV) {
  OdimStandard oStand(TEST_CSV_FILE);

  std::remove(WRITE_CSV_FILE.c_str());
  ASSERT_NO_THROW( oStand.writeToCsv(WRITE_CSV_FILE) );
  FILE* f = fopen(WRITE_CSV_FILE.c_str(), "r");
  ASSERT_THAT( f, NotNull() );
  fclose(f);

  OdimStandard oStandWritten;
  ASSERT_NO_THROW( oStandWritten.readFromCsv(WRITE_CSV_FILE) );
  ASSERT_THAT( oStandWritten.entries.size(), Eq(oStand.entries.size()) );
  for (int i=0, n=oStandWritten.entries.size(); i<n; ++i) {
    ASSERT_TRUE( oStandWritten.entries[i].node == oStand.entries[i].node );
    ASSERT_TRUE( oStandWritten.entries[i].category == oStand.entries[i].category );
    ASSERT_TRUE( oStandWritten.entries[i].type == oStand.entries[i].type );
    ASSERT_TRUE( oStandWritten.entries[i].isMandatory == oStand.entries[i].isMandatory );
    ASSERT_TRUE( oStandWritten.entries[i].possibleValues == oStand.entries[i].possibleValues );
    ASSERT_TRUE( oStandWritten.entries[i].reference == oStand.entries[i].reference );
  }
}
