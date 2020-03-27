#include <iostream>
#include <string>
#include <cstdlib>
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

TEST(testCompare, checkValueForStringsWorksForConcreteAssumedValue) {
  std::string myStr = "abc";
  std::string assumedStr = "abc";
  std::string errorMessage = "";
  ASSERT_TRUE( checkValue(myStr, assumedStr, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  myStr = "abc";
  assumedStr = "abcd";
  errorMessage = "";
  ASSERT_FALSE( checkValue(myStr, assumedStr, errorMessage) );
  ASSERT_FALSE( errorMessage.empty() );
}

TEST(testCompare, checkValueForStringsWorksForRegexAssumedValue) {
  std::string myStr = "NOD:skjav,WMO:11812";
  std::string assumedStr = ".*(WMO:[0-9]{5}([0-9]{2})?)?.*(RAD:.*)?.*(PLC:.*)?.*(NOD:.*)?.*(ORG:.*)?.*(CTY:.*)?.*(CMT:.*)?.*";
  std::string errorMessage = "";
  ASSERT_TRUE( checkValue(myStr, assumedStr, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  /* BUG - needs to resolve
  myStr = "1";
  ASSERT_FALSE( checkValue(myStr, assumedStr, errorMessage) );
  ASSERT_FALSE( errorMessage.empty() );
  */
}

TEST(testCompare, checkValueForNumbersWorksForConcreteAssumedValue) {
  double myNum = 1.256;
  std::string assumedNum = "1.256";
  std::string errorMessage = "";
  ASSERT_TRUE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  myNum = 1.255;
  ASSERT_FALSE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_FALSE( errorMessage.empty() );
}

TEST(testCompare, checkValueForNumbersWorksWithEqualSign) {
  double myNum = 1.256;
  std::string assumedNum = "=1.256";
  std::string errorMessage = "";
  ASSERT_TRUE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  myNum = 1.255;
  ASSERT_FALSE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_FALSE( errorMessage.empty() );

  myNum = 1.256;
  assumedNum = "==1.256";
  ASSERT_TRUE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  myNum = 1.255;
  ASSERT_FALSE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_FALSE( errorMessage.empty() );
}

TEST(testCompare, checkValueForNumbersWorksWithGtSign) {
  double myNum = 1.257;
  std::string assumedNum = ">1.256";
  std::string errorMessage = "";
  ASSERT_TRUE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  myNum = 1.256;
  ASSERT_FALSE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_FALSE( errorMessage.empty() );

  myNum = 1.257;
  assumedNum = ">=1.256";
  ASSERT_TRUE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  myNum = 1.256;
  ASSERT_TRUE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  myNum = 1.255;
  ASSERT_FALSE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_FALSE( errorMessage.empty() );
}

TEST(testCompare, checkValueForNumbersWorksWithLtSign) {
  double myNum = 1.255;
  std::string assumedNum = "<1.256";
  std::string errorMessage = "";
  ASSERT_TRUE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  myNum = 1.256;
  ASSERT_FALSE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_FALSE( errorMessage.empty() );

  myNum = 1.255;
  assumedNum = "<=1.256";
  ASSERT_TRUE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  myNum = 1.256;
  ASSERT_TRUE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  myNum = 1.257;
  ASSERT_FALSE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_FALSE( errorMessage.empty() );
}

TEST(testCompare, checkValueForNumbersWorksInsideInterval) {
  double myNum = 1.255;
  std::string assumedNum = ">1.254&&<1.256";
  std::string errorMessage = "";
  ASSERT_TRUE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  myNum = 1.256;
  ASSERT_FALSE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_FALSE( errorMessage.empty() );

  myNum = 1.255;
  assumedNum = ">=1.254&&<=1.256";
  ASSERT_TRUE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  myNum = 1.256;
  ASSERT_TRUE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  myNum = 1.257;
  ASSERT_FALSE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_FALSE( errorMessage.empty() );
}

TEST(testCompare, checkValueForNumbersWorksOutsideInterval) {
  double myNum = 1.253;
  std::string assumedNum = "<1.254||>1.256";
  std::string errorMessage = "";
  ASSERT_TRUE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  myNum = 1.255;
  ASSERT_FALSE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_FALSE( errorMessage.empty() );

  myNum = 1.254;
  assumedNum = "<=1.254||>=1.256";
  ASSERT_TRUE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  myNum = 1.256;
  ASSERT_TRUE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  myNum = 1.255;
  ASSERT_FALSE( checkValue(myNum, assumedNum, errorMessage) );
  ASSERT_FALSE( errorMessage.empty() );
}



TEST(testCompare, canCheckWhatSource) {
  std::string whatSource = "WMO:11812,NOD:skjav";
  std::string basicRegex = "(WIGOS:.*)|"  //WIGOS format
                           "(WMO:.*)|"  //WMO format
                           "(RAD:.*)|"
                           "(PLC:.*)|"
                           "(NOD:.*)|"      //NOD - should be ASCII only
                           "(ORG:.*)|"
                           "(CTY:.*)|"
                           "(CMT:.*)";
  std::string errorMessage = "";
  ASSERT_TRUE( checkValue(whatSource, basicRegex, errorMessage) );
  ASSERT_THAT( errorMessage, IsEmpty() );

  whatSource = "WIGOS:0-380-1-1";
  errorMessage = "";
  ASSERT_TRUE( checkWhatSource(whatSource, basicRegex, errorMessage) );
  ASSERT_TRUE( errorMessage.empty() );

  whatSource = "WMO:11812";
  errorMessage = "";
  ASSERT_TRUE( checkWhatSource(whatSource, basicRegex, errorMessage) );
  ASSERT_TRUE( errorMessage.empty() );

  whatSource = "WMO:0011812";
  errorMessage = "";
  ASSERT_TRUE( checkWhatSource(whatSource, basicRegex, errorMessage) );
  ASSERT_TRUE( errorMessage.empty() );

  whatSource = "WMO:foobar";
  errorMessage = "";
  ASSERT_FALSE( checkWhatSource(whatSource, basicRegex, errorMessage) );
  ASSERT_FALSE( errorMessage.empty() );

  whatSource = "NOD:skjav";
  errorMessage = "";
  ASSERT_TRUE( checkWhatSource(whatSource, basicRegex, errorMessage) );
  ASSERT_TRUE( errorMessage.empty() );

  whatSource = "RAD:FI44";
  errorMessage = "";
  ASSERT_TRUE( checkWhatSource(whatSource, basicRegex, errorMessage) );
  ASSERT_TRUE( errorMessage.empty() );

  whatSource = "PLC:Anjalankoski";
  errorMessage = "";
  ASSERT_TRUE( checkWhatSource(whatSource, basicRegex, errorMessage) );
  ASSERT_TRUE( errorMessage.empty() );

  whatSource = "ORG:86";
  errorMessage = "";
  ASSERT_TRUE( checkWhatSource(whatSource, basicRegex, errorMessage) );
  ASSERT_TRUE( errorMessage.empty() );

  whatSource = "CTY:613";
  errorMessage = "";
  ASSERT_TRUE( checkWhatSource(whatSource, basicRegex, errorMessage) );
  ASSERT_TRUE( errorMessage.empty() );

  whatSource = "CMT:Suomi tutka";
  errorMessage = "";
  ASSERT_TRUE( checkWhatSource(whatSource, basicRegex, errorMessage) );
  ASSERT_TRUE( errorMessage.empty() );

  whatSource = "NOD:skjav,WMO:0011812";
  errorMessage = "";
  ASSERT_TRUE( checkWhatSource(whatSource, basicRegex, errorMessage) );
  ASSERT_TRUE( errorMessage.empty() );

  whatSource = "WMO:0011812,NOD:skjav";
  errorMessage = "";
  ASSERT_TRUE( checkWhatSource(whatSource, basicRegex, errorMessage) );
  ASSERT_TRUE( errorMessage.empty() );

  whatSource = "WMO:foobar,NOD:skjav";
  errorMessage = "";
  ASSERT_FALSE( checkWhatSource(whatSource, basicRegex, errorMessage) );
  ASSERT_FALSE( errorMessage.empty() );
}
