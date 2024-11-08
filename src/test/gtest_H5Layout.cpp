#include <iostream>
#include <string>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "class_H5Layout.hpp"

using namespace testing;
using namespace myodim;

const std::string TEST_ODIM_FILE = "./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf";
const std::string WRONG_ODIM_FILE = "./data/example/T_PAGZ41_C_LZIB_20180403000000.hdfx";
const std::string TEST_ODIM_FILE_V24 = "./data/test/T_PAZE50_C_LFPW_20190426132340.h5";
const std::string TEST_PALETTE_FILE = "./data/test/test_palette.h5";

TEST(testH5Layout, isEmptyWhenConstructed) {
  const H5Layout h5layout;

  ASSERT_THAT( h5layout.groups, IsEmpty() );
  ASSERT_THAT( h5layout.datasets, IsEmpty() );
  ASSERT_THAT( h5layout.attributes, IsEmpty() );
}

TEST(testH5Layout, isPopulatedWhenConstructedWithODIMH5FileName) {
  const H5Layout h5layout(TEST_ODIM_FILE);

  ASSERT_THAT( h5layout.groups.size(), Gt(0u) );
  ASSERT_THAT( h5layout.datasets.size(), Gt(0u) );
  ASSERT_THAT( h5layout.attributes.size(), Gt(0u) );
}

TEST(testH5Layout, throwsWhenConstructedWithWrongODIMH5FileName) {
  ASSERT_ANY_THROW( const H5Layout h5layout(WRONG_ODIM_FILE) );
}

TEST(testH5Layout, canGetAttributeNamesFromGroupOrDataset) {
  const H5Layout h5layout(TEST_ODIM_FILE);
  const std::string groupName{"/where"};
  const std::string datasetName{"/dataset1/data1/data"};
  std::vector<std::string> attrNames;

  ASSERT_NO_THROW( attrNames = h5layout.getAttributeNames(groupName) );
  ASSERT_THAT( attrNames, SizeIs(3) );
  ASSERT_THAT( attrNames, Contains("lat") );
  ASSERT_THAT( attrNames, Contains("lon") );
  ASSERT_THAT( attrNames, Contains("height") );

  ASSERT_NO_THROW( attrNames = h5layout.getAttributeNames(datasetName) );
  ASSERT_THAT( attrNames, SizeIs(2) );
  ASSERT_THAT( attrNames, Contains("CLASS") );
  ASSERT_THAT( attrNames, Contains("IMAGE_VERSION") );
}

TEST(testH5Layout, canCheckEntryExistence) {
  const H5Layout h5layout(TEST_ODIM_FILE);

  ASSERT_TRUE( h5layout.hasAttribute("/where/lon") );
  ASSERT_FALSE( h5layout.hasAttribute("/where/lonx") );

  ASSERT_TRUE( h5layout.hasGroup("/dataset1/where") );
  ASSERT_FALSE( h5layout.hasGroup("/dataset1/wherex") );

  ASSERT_TRUE( h5layout.hasDataset("/dataset1/data1/data") );
  ASSERT_FALSE( h5layout.hasDataset("/dataset1/data1x/data") );
}

TEST(testH5Layout, canGetAttributeValues) {
  const H5Layout h5layout(TEST_ODIM_FILE);
  std::string attrName = "/what/object";
  std::string stringValue;

  ASSERT_NO_THROW( h5layout.getAttributeValue(attrName, stringValue) );
  ASSERT_THAT( stringValue, Eq("PVOL") );

  attrName = "/where/lon";
  double doubleValue;
  h5layout.getAttributeValue(attrName, doubleValue);
  ASSERT_THAT( doubleValue, DoubleEq(17.1531) );

  attrName = "/dataset1/where/nbins";
  int64_t intValue;
  ASSERT_NO_THROW( h5layout.getAttributeValue(attrName, intValue) );
  ASSERT_THAT( intValue, Eq(960) );
}

TEST(testH5Layout, getAttributeValuesThrowsOnError) {
  const H5Layout h5layout(TEST_ODIM_FILE);
  std::string attrName = "/what/objectx";
  std::string stringValue;

  ASSERT_ANY_THROW( h5layout.getAttributeValue(attrName, stringValue) );

  attrName = "/where/lonx";
  double doubleValue;
  ASSERT_ANY_THROW( h5layout.getAttributeValue(attrName, doubleValue) );

  attrName = "/dataset1/where/nbinsx";
  int64_t intValue;
  ASSERT_ANY_THROW( h5layout.getAttributeValue(attrName, intValue) );
}

TEST(testH5Layout, canCheckAttributeTypes) {
  const H5Layout h5layout(TEST_ODIM_FILE);
  std::string errmsg;

  ASSERT_TRUE( h5layout.isFixedLengthStringAttribute("/what/object", errmsg) );
  ASSERT_TRUE( h5layout.isReal64Attribute("/where/lon") );
  ASSERT_TRUE( h5layout.isInt64Attribute("/dataset1/where/nbins") );
  ASSERT_FALSE( h5layout.isReal64Attribute("/how/startepochs") );
}

TEST(testH5Layout, isXXXAttributeThrowsOnError) {
  const H5Layout h5layout(TEST_ODIM_FILE);
  std::string errmsg;

  ASSERT_ANY_THROW( h5layout.isFixedLengthStringAttribute("/what/objectx", errmsg) );
  ASSERT_ANY_THROW( h5layout.isReal64Attribute("/where/lonx") );
  ASSERT_ANY_THROW( h5layout.isInt64Attribute("/dataset1/where/nbinsx") );
  ASSERT_ANY_THROW( h5layout.isReal64Attribute("/how/startepochsx") );
}

TEST(testH5Layout, canSayDatasetIsUchar) {
  const H5Layout h5layout(TEST_ODIM_FILE);
  const h5Entry dataset = h5layout.datasets[0];

  ASSERT_TRUE( h5layout.isUcharDataset(dataset.name()) );

  //throws on error
  ASSERT_ANY_THROW( h5layout.isUcharDataset(dataset.name()+"x") );
}

TEST(testH5Layout, canSayIsArrayAttribute) {
  const H5Layout h5layout(TEST_ODIM_FILE);

  ASSERT_TRUE( h5layout.is1DArrayAttribute("/dataset1/how/startazA") );
  ASSERT_FALSE( h5layout.is1DArrayAttribute("/where/lon") );
}

TEST(testH5Layout, canGetValuesOfArrayAttribute) {
  const H5Layout h5layout(TEST_ODIM_FILE);
  std::vector<double> values;

  ASSERT_NO_THROW( h5layout.getAttributeValue("/dataset1/how/startazA", values) );
  int n = values.size();
  ASSERT_THAT( n, Eq(360) );
  ASSERT_THAT( values[0], Gt(0.0) );
  ASSERT_THAT( values[0], Lt(1.0) );
  ASSERT_THAT( values[n-1], Gt(359.0) );
  ASSERT_THAT( values[n-1], Lt(360.0) );

  ASSERT_NO_THROW( h5layout.getAttributeValue("/where/lon", values) );
  ASSERT_THAT(  (int)values.size(), Eq(1) );
  ASSERT_THAT( values[0], DoubleEq(17.1531) );
}

TEST(testH5Layout, canReturnMinMaxMeanOfAttribute) {
  const H5Layout h5layout(TEST_ODIM_FILE);
  double first=-1.0, last=-1.0, min=-1.0, max=-1.0, mean=-1.0;

  ASSERT_NO_THROW( h5layout.attributeStatistics("/where/lon", first, last, min, max, mean) );
  ASSERT_THAT( first, DoubleEq(17.1531) );
  ASSERT_THAT( last, DoubleEq(17.1531) );
  ASSERT_THAT( min, DoubleEq(17.1531) );
  ASSERT_THAT( max, DoubleEq(17.1531) );
  ASSERT_THAT( mean, DoubleEq(17.1531) );

  first=-1.0, last=-1.0, min=-1.0, max=-1.0, mean=-1.0;
  ASSERT_NO_THROW( h5layout.attributeStatistics("/dataset1/how/startazA", first, last, min, max, mean) );
  ASSERT_THAT( first, Gt(0.0) );
  ASSERT_THAT( first, Lt(1.0) );
  ASSERT_THAT( last, Gt(359.0) );
  ASSERT_THAT( last, Lt(360.0) );
  ASSERT_THAT( min, Gt(0.0) );
  ASSERT_THAT( min, Lt(1.0) );
  ASSERT_THAT( max, Gt(359.0) );
  ASSERT_THAT( max, Lt(360.0) );
  ASSERT_THAT( mean, Gt(175.0) );
  ASSERT_THAT( mean, Lt(185.0) );
}

TEST(testH5Layout, canSayIs2DArrayAttribute) {
  const H5Layout h5layout(TEST_ODIM_FILE_V24); // this test file needs to be created with the dev-create-v24-file-for-test program

  ASSERT_TRUE( h5layout.is2DArrayAttribute("/dataset1/how/zr_a_A") );
  ASSERT_FALSE( h5layout.is2DArrayAttribute("/dataset1/how/startazA") );
  ASSERT_FALSE( h5layout.is2DArrayAttribute("/where/lon") );
}

TEST(testH5Layout, canGetValuesOf2DArrayAttribute) {
  const H5Layout h5layout(TEST_ODIM_FILE_V24);
  std::vector<double> values;

  ASSERT_NO_THROW( h5layout.getAttributeValue("/dataset1/how/zr_a_A", values) );
  const int n = values.size();
  ASSERT_THAT( n, Eq(360*22) );
  ASSERT_THAT( values[0], DoubleEq(200.0) );
  ASSERT_THAT( values[n-1], DoubleEq(200.0) );

}

TEST(testH5Layout, canReturnMinMaxMeanOf2DAttribute) {
  const H5Layout h5layout(TEST_ODIM_FILE_V24);
  double first=-1.0, last=-1.0, min=-1.0, max=-1.0, mean=-1.0;

  ASSERT_NO_THROW( h5layout.attributeStatistics("/dataset1/how/zr_a_A", first, last, min, max, mean) );
  ASSERT_THAT( first, DoubleEq(200.0) );
  ASSERT_THAT( last, DoubleEq(200.0) );
  ASSERT_THAT( min, DoubleEq(200.0) );
  ASSERT_THAT( max, DoubleEq(200.0) );
}

TEST(testH5Layout, isFixedLengthStringAttributeWorks) {
  H5Layout h5layoutWithSTRNULLPAD("./data/test/raa01-ry_10000-2310161645-dwd---bin.hdf5");
  std::string errmsg;

  ASSERT_FALSE( h5layoutWithSTRNULLPAD.isFixedLengthStringAttribute("/what/source", errmsg) );
  ASSERT_THAT( errmsg, HasSubstr("H5T_STR_NULLTERM") );
  
}

TEST(testH5Layout, canWorkWithPalette) {
  H5Layout h5layout(TEST_PALETTE_FILE);
  std::string errmsg;

  ASSERT_TRUE( h5layout.hasDataset("/01-PALETTE") );
  ASSERT_TRUE( h5layout.isStringAttribute("/01-PALETTE/CLASS") );
  ASSERT_TRUE( h5layout.hasAttribute("/CT/PALETTE") );
  ASSERT_TRUE( h5layout.isLinkAttribute("/CT/PALETTE") );
}

TEST(BUGH5Layout, shouldThrowOnWrongSTRSIZEOfHowSystem) {
  const H5Layout h5layout("./data/test/T_PAJZ41_C_LZIB_20231023000000.hdf");
  std::string attrName = "/how/system";
  std::string stringValue;

  //ASSERT_FALSE( h5layout.stringAttributeHasProperSize(attrName) );

  ASSERT_THROW( {
    try {
      h5layout.getAttributeValue(attrName, stringValue);
    }
    catch (const std::runtime_error& e) {
      ASSERT_THAT( e.what(), HasSubstr("STRSIZE") );
      throw;
    }
  },std::runtime_error);
}




