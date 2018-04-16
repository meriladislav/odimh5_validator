#include <cstdio> 
#include <iostream>
#include <string>
#include "class_H5Layout.hpp"
#include "class_OdimStandard.hpp"
#include "module_Compare.hpp"
#include "cxxopts.hpp"


const std::string USAGE{"usage: odimh5-validate h5File [standardDefinition.csv]"};
const int MIN_ARG_NUM{2};

int main(int argc, const char* argv[]) {
  setbuf(stdout, NULL);
  
  //check and parse arguments
  cxxopts::Options options(argv[0], " Program to analyse the ODIM-H5 standard-conformance");
  options.add_options("Mandatory")
    ("i,input", "input ODIM-H5 file to analyse", cxxopts::value<std::string>());
  
  options.add_options("Optional")
    ("h,help", "print this help message")
    ("c,csv", "standard-definition .csv table, e.g. your_path/your_table.csv", cxxopts::value<std::string>())
    ("v,version", "standard version to use, e.g. 2.1", cxxopts::value<std::string>())
    ("checkOptional", "check the presence of the optional ODIM entries, default is False",  
        cxxopts::value<bool>()->default_value("false"))
    ("checkExtras", "check the presence of extra entries, not mentioned in the standard, default is False",  
        cxxopts::value<bool>()->default_value("false"))
    ("noInfo", "don`t print INFO messages, only WARNINGs and ERRORs, default is False",  
        cxxopts::value<bool>()->default_value("false"));
  
  auto cmdLineOptions = options.parse(argc, argv);
  if ( cmdLineOptions.count("input") != 1 || cmdLineOptions.count("help") > 0 ) {
    std::cout << options.help({"Mandatory", "Optional"}) << std::endl;
    return -1;
  }
  
  myodim::printInfo = !(cmdLineOptions["noInfo"].as<bool>());
  
  //load the hdf5 input file layout
  std::string h5File{cmdLineOptions["input"].as<std::string>()};
  myh5::H5Layout h5layout(h5File);
  
  std::string csvFile{""};
  if ( cmdLineOptions.count("csv") == 1 ) {
    csvFile = cmdLineOptions["csv"].as<std::string>();
  }
  else {
    if ( cmdLineOptions.count("version") == 1 ) {
      csvFile = myodim::getCsvFileNameFrom(h5layout, cmdLineOptions["version"].as<std::string>());
    }
    else {
      csvFile = myodim::getCsvFileNameFrom(h5layout);
    }
  }
  if ( myodim::printInfo ) {
    std::cout << "INFO - using the " << csvFile << " standard definition table" << std::endl;
  }
  
  myodim::OdimStandard odimStandard(csvFile);
  
  const bool checkOptional{cmdLineOptions["checkOptional"].as<bool>()};
  const bool checkExtras{cmdLineOptions["checkExtras"].as<bool>()};
  
  //compare the layout to the standard
  bool isCompliant = myodim::compare(h5layout, odimStandard, checkOptional, checkExtras);
  if ( isCompliant ) {
    if ( myodim::printInfo ) {
      std::string message = "INFO - file " + h5File + " is a standard-compliant ODIM-H5 file";
      if ( !checkOptional ) {
        if ( !checkExtras ) {
          message += ", but the optional and extra entries were not checked.";
        }
        else {
          message += ", but the optional entries were not checked.";
        }
      }
      else {
        if ( !checkExtras ) {
          message += ", but the extra entries were not checked.";
        }
      }
      std::cout << message << std::endl;
    }
    return 0;
  }
  else { 
    std::cout << "WARNING - file " << h5File << " IS NOT a standard-compliant ODIM-H5 file - see the previous WARNING messages" << std::endl;
    return -1;
  }
}
