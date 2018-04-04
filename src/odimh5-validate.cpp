#include <cstdio> 
#include <iostream>
#include <string>
#include "class_H5Layout.hpp"
#include "class_OdimStandard.hpp"

const std::string USAGE{"usage: odimh5-validate h5File standardDefinition.csv"};
const int MIN_ARG_NUM{2};

int main(int argc, char* argv[]) {
  setbuf(stdout, NULL);
  
  if ( argc < MIN_ARG_NUM ) {
    std::cout << USAGE << std::endl;
    return -1;
  }
  
  std::string h5File{argv[1]};
  std::string csvFile{argc > MIN_ARG_NUM ? argv[2] : ""};
  
  myh5::H5Layout h5layout(h5File);
  std::cout << "dbg - " << h5layout.groups.size() << " groups, " << h5layout.datasets.size() <<
               " datasets and " << h5layout.attributes.size() << " attributes found" << std::endl;
  
  myodim::OdimStandard odimStandard(csvFile);
  std::cout << "dbg - " << odimStandard.entries.size() << " ODIM standard entries loaded" << std::endl;
  
  return 0;
}
