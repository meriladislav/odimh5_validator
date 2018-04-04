#include <cstdio> 
#include <iostream>
#include <string>
#include <stdexcept>
#include "csv.h"
#include "class_H5Layout.hpp"
#include "class_OdimEntry.hpp"

const std::string USAGE{"usage: odimh5-validate h5File standardDefinition.csv"};
const int MIN_ARG_NUM{3};

const int CSV_COL_NUM{5};
const char CSV_SEPARATOR{';'};



int main(int argc, char* argv[]) {
  setbuf(stdout, NULL);
  
  if ( argc < MIN_ARG_NUM ) {
    std::cout << USAGE << std::endl;
    return -1;
  }
  
  std::string h5File{argv[1]};
  std::string csvFile{argv[2]};
  
  myh5::H5Layout h5layout(h5File);
  std::cout << "dbg - " << h5layout.groups.size() << " groups, " << h5layout.datasets.size() <<
               " datasets and " << h5layout.attributes.size() << " attributes found" << std::endl;
  for (const auto& g : h5layout.groups) std::cout << g << " - group" << std::endl;
  for (const auto& d : h5layout.datasets) std::cout << d << " - dataset" << std::endl;
  for (const auto& a : h5layout.attributes) std::cout << a << " - attribute" << std::endl;
  
  
  io::CSVReader<CSV_COL_NUM, io::trim_chars<CSV_SEPARATOR>, io::no_quote_escape<CSV_SEPARATOR>> csv(csvFile);
  csv.read_header(io::ignore_no_column, 
                  "Node", "Category", "Type", "IsMandatory", "PossibleValues");
  myodim::OdimEntry entry;
  std::string node, category, type, isMandatory, possibleValues;
  while(csv.read_row(node, category, type, isMandatory, possibleValues)){
    entry.set(node, category, type, isMandatory, possibleValues);
    std::cout << "dbg - " << entry.node << std::endl;
  }
  
  return 0;
}
