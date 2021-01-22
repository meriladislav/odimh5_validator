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
const std::string TEST_OUT_DIR = "./out/";
const std::string TEST_CSV_FILE = "./data/ODIM_H5_V2_1_PVOL.csv";
const std::string CSV_TO_CORRECT = "./data/example/failed_entries.csv";
const std::string CSV_TO_ADD = "./data/example/add_entries.csv";
const std::string CSV_TO_ADD_WRONG = "./data/example/add_entries_wrong.csv";
const std::string CSV_TO_ADD_REGEX = "./data/example/add_entries_regex.csv";
const std::string CSV_TO_REPLACE = "./data/example/replace_entries.csv";
const std::string CSV_CORRECT_ALL = "./data/example/correct_all.csv";

TEST(testRepair, canCopyHdf5File) {
  const std::string testOutFile = TEST_OUT_DIR+"testRepair"+"."+"canCopyHdf5File"+".hdf";
  std::remove(testOutFile.c_str());

  ASSERT_NO_THROW( copyFile(TEST_IN_FILE, testOutFile) );
  FILE* f = fopen(testOutFile.c_str(), "r");
  ASSERT_THAT( f, NotNull() );
  fclose(f);

  ASSERT_THAT( fileSize(TEST_IN_FILE), Eq(fileSize(testOutFile)) );
}

TEST(testRepair, canRepairReal64AttributeDataTypes) {
  const std::string testOutFile = TEST_OUT_DIR+"testRepair"+"."+"canRepairReal64AttributeDataTypes"+".hdf";
  std::remove(testOutFile.c_str());

  printInfo = false;

  H5Layout h5LayIn(TEST_IN_FILE);
  OdimStandard oStand(TEST_CSV_FILE);
  const bool checkOptional = true; // - some optional entries are not full compliant
  const bool checkExtras = false;
  OdimStandard failedBeforeCorrect;
  ASSERT_FALSE( compare(h5LayIn, oStand, checkOptional, checkExtras, &failedBeforeCorrect) );
  ASSERT_FALSE( failedBeforeCorrect.entries.empty() );
  ASSERT_THAT( failedBeforeCorrect.entries.size(), Eq(4u) );
  OdimStandard toCorrect(CSV_TO_CORRECT);

  ASSERT_NO_THROW( correct(TEST_IN_FILE, testOutFile, toCorrect) );

  H5Layout h5LayOut(testOutFile);
  OdimStandard failedAfterCorrect;
  ASSERT_TRUE( compare(h5LayOut, oStand, checkOptional, checkExtras, &failedAfterCorrect) );
  ASSERT_TRUE( failedAfterCorrect.entries.empty() );
}

TEST(testRepair, canAddAttributesAndGroups) {
  const std::string testOutFile = TEST_OUT_DIR+"testRepair"+"."+"canAddAttributesAndGroups"+".hdf";
  std::remove(testOutFile.c_str());

  printInfo = false;

  OdimStandard toAdd(CSV_TO_ADD);

  ASSERT_NO_THROW( correct(TEST_IN_FILE, testOutFile, toAdd) );

  H5Layout h5LayOut(testOutFile);
  ASSERT_TRUE( h5LayOut.hasGroup("/testGroup") );
  ASSERT_TRUE( h5LayOut.hasAttribute("/testGroup/testInt") );
  ASSERT_TRUE( h5LayOut.hasAttribute("/testGroup/testReal") );
  ASSERT_TRUE( h5LayOut.hasAttribute("/testGroup/testString") );
  ASSERT_TRUE( h5LayOut.isInt64Attribute("/testGroup/testInt") );
  ASSERT_TRUE( h5LayOut.isReal64Attribute("/testGroup/testReal") );
  ASSERT_TRUE( h5LayOut.isStringAttribute("/testGroup/testString") );
  int64_t i64;
  ASSERT_NO_THROW( h5LayOut.getAttributeValue("/testGroup/testInt", i64) );
  ASSERT_THAT( (int)i64, Eq(5678) );
  double r64;
  ASSERT_NO_THROW( h5LayOut.getAttributeValue("/testGroup/testReal", r64) );
  ASSERT_THAT( r64, DoubleEq(0.1234) );
  std::string s;
  ASSERT_NO_THROW( h5LayOut.getAttributeValue("/testGroup/testString", s) );
  ASSERT_THAT( s, StrEq("foobar") );
}

TEST(testRepair, addAttributeThrowsWhenNoOrWrongValuePresent) {
  const std::string testOutFile = TEST_OUT_DIR+"testRepair"+"."+"addAttributeThrowsWhenNoOrWrongValuePresent"+".hdf";
  std::remove(testOutFile.c_str());

  printInfo = false;

  OdimStandard toAdd(CSV_TO_ADD_WRONG);

  ASSERT_ANY_THROW( correct(TEST_IN_FILE, testOutFile, toAdd) );
}

TEST(testRepair, worksWithRegexInNodes) {
  const std::string testOutFile = TEST_OUT_DIR+"testRepair"+"."+"worksWithRegexInNodes"+".hdf";
  std::remove(testOutFile.c_str());

  printInfo = false;

  OdimStandard toAdd(CSV_TO_ADD_REGEX);

  ASSERT_NO_THROW( correct(TEST_IN_FILE, testOutFile, toAdd) );

  H5Layout h5LayOut(testOutFile);
  ASSERT_TRUE( h5LayOut.hasGroup("/dataset1/testGroup") );
  ASSERT_TRUE( h5LayOut.hasGroup("/dataset12/testGroup") );

  int64_t i64;
  ASSERT_NO_THROW( h5LayOut.getAttributeValue("/dataset1/testGroup/testInt", i64) );
  ASSERT_THAT( (int)i64, Eq(5678) );
  double r64;
  ASSERT_NO_THROW( h5LayOut.getAttributeValue("/dataset1/testGroup/testReal", r64) );
  ASSERT_THAT( r64, DoubleEq(0.1234) );
  std::string s;
  ASSERT_NO_THROW( h5LayOut.getAttributeValue("/dataset1/testGroup/testString", s) );
  ASSERT_THAT( s, StrEq("foobar") );

  ASSERT_NO_THROW( h5LayOut.getAttributeValue("/dataset12/testGroup/testInt", i64) );
  ASSERT_THAT( (int)i64, Eq(5678) );
  ASSERT_NO_THROW( h5LayOut.getAttributeValue("/dataset12/testGroup/testReal", r64) );
  ASSERT_THAT( r64, DoubleEq(0.1234) );
  ASSERT_NO_THROW( h5LayOut.getAttributeValue("/dataset12/testGroup/testString", s) );
  ASSERT_THAT( s, StrEq("foobar") );
  ASSERT_NO_THROW( h5LayOut.getAttributeValue("/dataset12/data1/testGroup/testString", s) );
  ASSERT_THAT( s, StrEq("foobar") );
}

TEST(testRepair, canReplaceExistingValues) {
  const std::string testOutFile = TEST_OUT_DIR+"testRepair"+"."+"canReplaceExistingValues"+".hdf";
  std::remove(testOutFile.c_str());

  printInfo = false;

  OdimStandard toAdd(CSV_TO_REPLACE);

  ASSERT_NO_THROW( correct(TEST_IN_FILE, testOutFile, toAdd) );

  H5Layout h5LayOut(testOutFile);
  int64_t i64;
  ASSERT_NO_THROW( h5LayOut.getAttributeValue("/how/highprf", i64) );
  ASSERT_THAT( (int)i64, Eq(601) );
  double r64;
  ASSERT_NO_THROW( h5LayOut.getAttributeValue("/how/beamwidth", r64) );
  ASSERT_THAT( r64, DoubleEq(1.0) );
  std::string s;
  ASSERT_NO_THROW( h5LayOut.getAttributeValue("/how/system", s) );
  ASSERT_THAT( s, StrEq("selex") );
}

TEST(testRepair, hasMetadataChangedAttributeAfterCorrect) {
  const std::string testOutFile = TEST_OUT_DIR+"testRepair"+"."+"hasMetadataChangedAttributeAfterCorrect"+".hdf";
  std::remove(testOutFile.c_str());

  printInfo = false;

  OdimStandard toAdd(CSV_CORRECT_ALL);

  ASSERT_NO_THROW( correct(TEST_IN_FILE, testOutFile, toAdd) );

  H5Layout h5LayOut(testOutFile);
  ASSERT_TRUE( h5LayOut.hasAttribute("/how/metadata_changed") );
  std::string s;
  ASSERT_NO_THROW( h5LayOut.getAttributeValue("/how/metadata_changed", s) );
  ASSERT_THAT( s.find("/how/system"), Ne(std::string::npos) );
  ASSERT_THAT( s.find("testGroup"), Ne(std::string::npos) );
  ASSERT_THAT( s.find("testInt"), Ne(std::string::npos) );
  ASSERT_THAT( s.find("startepochs"), Ne(std::string::npos) );
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
