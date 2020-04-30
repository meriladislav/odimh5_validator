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
    ("t,type", "specify the type of te attribute - possible values: string, int, real",
     cxxopts::value<std::string>())
    ("noInfo", "don`t print INFO messages, only WARNINGs and ERRORs, default is False",
     cxxopts::value<bool>()->default_value("false"));

  auto cmdLineOptions = options.parse(argc, argv);
  if ( cmdLineOptions.count("input") != 1 ||
       cmdLineOptions.count("attribute") != 1 ||
       cmdLineOptions.count("value") != 1 ||
       cmdLineOptions.count("help") > 0 ) {
    std::cout << options.help({"Mandatory", "Optional"}) << std::endl;
    return -1;
  }

  myodim::printInfo = !(cmdLineOptions["noInfo"].as<bool>());

  const std::string h5File = cmdLineOptions["input"].as<std::string>();

  myodim::H5Layout h5layout(h5File);

  const std::string attrName = cmdLineOptions["attribute"].as<std::string>();
  const std::string strAssumedValue = cmdLineOptions["value"].as<std::string>();
  const std::string strAssumedType = cmdLineOptions.count("type") > 0 ?
                                     cmdLineOptions["type"].as<std::string>() :
                                     "";

  if ( !strAssumedType.empty() &&
       (strAssumedType != "string" && strAssumedType != "real" && strAssumedType != "int") ) {
    std::cout << "ERROR - the value of the -t or --type option should be \"string\" \"real\" or \"int\" - see the usage" << std::endl;
    return -1;
  }

  bool valueIsOK = false;
  std::string actualValueStr = "";
  std::string errorMessage="";
  try {
  if ( myodim::isStringValue(strAssumedValue) || strAssumedType == "string" ) {
    if ( h5layout.isStringAttribute(attrName) ) {
      std::string attrValue = "";
      h5layout.getAttributeValue(attrName, attrValue);
      valueIsOK = myodim::checkValue(attrValue, strAssumedValue, errorMessage);
      actualValueStr = attrValue;
    }
    else {
      std::cout << "WARNING - NON-STANDARD DATA TYPE - the type of " << attrName << " is not string, " <<
                   "as expected from the assumed value - try to use the -t option to specify the type " << std::endl;
    }
  }
  else {
    if ( myodim::hasDoublePoint(strAssumedValue) || strAssumedType == "real" ) {
      double value;
      if ( h5layout.isReal64Attribute(attrName) ) {
         h5layout.getAttributeValue(attrName, value);
         const bool isReal = true;
         valueIsOK =  myodim::checkValue(value, strAssumedValue, errorMessage, isReal);
         actualValueStr = std::to_string(value);
      }
      else {
        std::cout << "WARNING - NON-STANDARD DATA TYPE - the type of " << attrName << " is not 64-bit real, " <<
                     "as expected from the assumed value - try to use the -t option to specify the type " << std::endl;
      }
    }
    else {
      int64_t value;
      if ( h5layout.isInt64Attribute(attrName) ) {
         h5layout.getAttributeValue(attrName, value);
         const bool isNoReal = false;
         valueIsOK =  myodim::checkValue(value, strAssumedValue, errorMessage, isNoReal);
         actualValueStr = std::to_string(value);
      }
      else {
        std::cout << "WARNING - NON-STANDARD DATA TYPE - the type of " << attrName << " is not 64-bit integer, " <<
                     "as expected from the assumed value - try to use the -t option to specify the type " << std::endl;
      }
    }
  }
  }
  catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
  }


  if ( valueIsOK ) {
    if ( myodim::printInfo ) {
      std::cout << "INFO - OK - the value of " << attrName << " is " << actualValueStr <<
                   ", which matches the " << strAssumedValue << " assumed value" << std::endl;
    }
    return 0;
  }
  else {
    std::cout << "WARNING - INCORRECT VALUE - the value of " << attrName << " " << errorMessage << std::endl;
    return -1;
  }
}


