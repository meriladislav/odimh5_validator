#include <iostream>
#include <string>
#include <cmath>
#include "cxxopts.hpp"
#include "class_H5Layout.hpp"
#include "module_Compare.hpp"

int main(int argc, const char* argv[]) {

  //check and parse arguments
  cxxopts::Options options(argv[0], " Program to check the value of an attribute in a ODIM-H5 file");
  options.add_options("Mandatory")
    ("i,input", "input ODIM-H5 file to analyse", cxxopts::value<std::string>())
    ("a,attribute", "the full path to the attribute to check", cxxopts::value<std::string>())
    ("v,value", "the assumed value of the attribute", cxxopts::value<std::string>());
  options.add_options("Optional")
    ("h,help", "print this help message")
    ("noInfo", "don`t print INFO messages, only WARNINGs and ERRORs, default is False",
        cxxopts::value<bool>()->default_value("false"));

  auto cmdLineOptions = options.parse(argc, argv);
  if ( cmdLineOptions.count("input") != 1 || cmdLineOptions.count("help") > 0 ) {
    std::cout << options.help({"Mandatory", "Optional"}) << std::endl;
    return -1;
  }

  myodim::printInfo = !(cmdLineOptions["noInfo"].as<bool>());

  const std::string h5File{cmdLineOptions["input"].as<std::string>()};

  myodim::H5Layout h5layout;
  h5layout.checkAndOpenFile(h5File);

  const std::string attrName{cmdLineOptions["attribute"].as<std::string>()};
  const std::string strAssumedValue{cmdLineOptions["value"].as<std::string>()};

  bool valueIsOK = false;
  std::string realValue = "";

  if ( myodim::isStringValue(strAssumedValue) ) {
    std::string value;
    if ( h5layout.isStringAttribute(attrName) ) {
      h5layout.getAttributeValue(attrName, value);
      valueIsOK =  value == strAssumedValue;
      realValue = value;
    }
    else {
      std::cout << "WARNING - the type of " << attrName << " is not string, " <<
                   "as expected from the assumed value " << std::endl;
    }
  }
  else {
    if ( myodim::hasDoublePoint(strAssumedValue) ) {
      const double assumedValue = std::stod(strAssumedValue);
      double value;
      if ( h5layout.isReal64Attribute(attrName) ) {
         h5layout.getAttributeValue(attrName, value);
         valueIsOK =  std::fabs(value - assumedValue) < 0.001;
         realValue = std::to_string(value);
      }
      else {
        std::cout << "WARNING - the type of " << attrName << " is not 64-bit real, " <<
                     "as expected from the assumed value " << std::endl;
      }
    }
    else {
      const int64_t assumedValue = std::stoi(strAssumedValue);
      int64_t value;
      if ( h5layout.isInt64Attribute(attrName) ) {
         h5layout.getAttributeValue(attrName, value);
         valueIsOK =  value == assumedValue;
         realValue = std::to_string(value);
      }
      else {
        std::cout << "WARNING - the type of " << attrName << " is not 64-bit integer, " <<
                     "as expected from the assumed value " << std::endl;
      }
    }
  }


  if ( valueIsOK ) {
    if ( myodim::printInfo ) {
      std::cout << "INFO - OK - the value of " << attrName << " is " << strAssumedValue << std::endl;
    }
    return 0;
  }
  else {
    if ( !realValue.empty() ) {
      std::cout << "WARNING - the value of " << attrName << " is not " << strAssumedValue <<
                   ", it is " << realValue << std::endl;
    }
    return -1;
  }
}


