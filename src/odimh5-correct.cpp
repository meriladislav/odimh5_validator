#include <iostream>
#include <string>
#include "cxxopts.hpp"
#include "class_H5Layout.hpp"
#include "class_OdimStandard.hpp"
#include "module_Compare.hpp"
#include "module_Correct.hpp"

int main(int argc, const char* argv[]) {

  //check and parse arguments
  cxxopts::Options options(argv[0], " Program to repair, change or extend ODIM-H5 files");
  options.add_options("Mandatory")
    ("i,input", "input ODIM-H5 file to change", cxxopts::value<std::string>())
    ("o,output", "output - the changed ODIM-H5 file to save", cxxopts::value<std::string>())
    ("c,correctionTable", "the csv table to listing the problematic entries - the format is as in the standard-definition .csv table", cxxopts::value<std::string>());

  options.add_options("Optional")
    ("h,help", "print this help message")
    ("noInfo", "don`t print INFO messages, only WARNINGs and ERRORs, default is False",
        cxxopts::value<bool>()->default_value("false"));


  auto cmdLineOptions = options.parse(argc, argv);
  if ( cmdLineOptions.count("input") != 1 ||
       cmdLineOptions.count("output") != 1 ||
       cmdLineOptions.count("correctionTable") != 1 ||
       cmdLineOptions.count("help") > 0 ) {
    std::cout << options.help({"Mandatory", "Optional"}) << std::endl;
    return -1;
  }

  myodim::printInfo = !(cmdLineOptions["noInfo"].as<bool>());

  std::string inH5File(cmdLineOptions["input"].as<std::string>());
  std::string outH5File(cmdLineOptions["output"].as<std::string>());
  std::string csvFile = cmdLineOptions["correctionTable"].as<std::string>();

  const myodim::OdimStandard toCorrect(csvFile);

  myodim::correct(inH5File, outH5File, toCorrect);

  return 0;
}
