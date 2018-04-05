#include <cstdio> 
#include <iostream>
#include <string>
#include "class_H5Layout.hpp"
#include "class_OdimStandard.hpp"
#include "module_Compare.hpp"

const std::string USAGE{"usage: odimh5-validate h5File [standardDefinition.csv]"};
const int MIN_ARG_NUM{2};

int main(int argc, char* argv[]) {
  setbuf(stdout, NULL);
  
  //check and parse arguments
  if ( argc < MIN_ARG_NUM ) {
    std::cout << USAGE << std::endl;
    return -1;
  }
  std::string h5File{argv[1]};
  std::string csvFile{argc > MIN_ARG_NUM ? argv[2] : ""};
  
  //load the hdf5 input file layout
  myh5::H5Layout h5layout(h5File);
  std::cout << "dbg - " << h5layout.groups.size() << " groups, " << h5layout.datasets.size() <<
               " datasets and " << h5layout.attributes.size() << " attributes found" << std::endl;
  //for (const auto& g : h5layout.groups) std::cout << g << std::endl;
  //for (const auto& a : h5layout.attributes) std::cout << a << std::endl;
  
  //load the ODIM_H5 standard definition 
  if ( csvFile.empty() ) {
    csvFile = myodim::getCsvFileNameFrom(h5layout);
    std::cout << "INFO - using the default " << csvFile << " standard definition table" << std::endl;
  }
  myodim::OdimStandard odimStandard(csvFile);
  std::cout << "dbg - " << odimStandard.entries.size() << " ODIM standard entries loaded" << std::endl;
  
  //compare the layout to the standard
  bool isCompliant = myodim::compare(h5layout, odimStandard);
  if ( isCompliant ) {
    return 0;
  }
  else { 
    return -1;
  }
}
