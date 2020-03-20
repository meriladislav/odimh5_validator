#include <iostream>
#include <string>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "class_H5Layout.hpp"

using namespace testing;
using namespace myodim;

const std::string TEST_ODIM_FILE = "./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf";

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

TEST(testH5Layout, canSayDatasetIsUchar) {
  const H5Layout h5layout(TEST_ODIM_FILE);
  const h5Entry dataset = h5layout.datasets[0];

  ASSERT_TRUE( h5layout.isUcharDataset(dataset.name()) );
}
