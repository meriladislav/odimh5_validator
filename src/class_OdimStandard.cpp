// class_OdimStandard.cpp
// class to work with ODIM specification
// Ladislav Meri, SHMU
// v_0.0, 04.2018

#include "csv.h"
#include "class_OdimStandard.hpp"

namespace myodim {

static const int CSV_COL_NUM{5};
static const char CSV_SEPARATOR{';'};

OdimStandard::OdimStandard(const std::string& csvFilePath) {
  readFromCsv(csvFilePath);
}

void OdimStandard::readFromCsv(const std::string& csvFilePath) {
  entries.clear();
  
  io::CSVReader<CSV_COL_NUM, io::trim_chars<CSV_SEPARATOR>, 
                io::no_quote_escape<CSV_SEPARATOR>> csv(csvFilePath);
  
  csv.read_header(io::ignore_no_column, 
                  "Node", "Category", "Type", "IsMandatory", "PossibleValues");
  
  std::string node, category, type, isMandatory, possibleValues;
  while(csv.read_row(node, category, type, isMandatory, possibleValues)){
    entries.emplace_back(OdimEntry{node, category, type, isMandatory, possibleValues});
  }
}

void OdimStandard::updateWithCsv(const std::string& csvFilePath) {
  io::CSVReader<CSV_COL_NUM, io::trim_chars<CSV_SEPARATOR>,
                io::no_quote_escape<CSV_SEPARATOR>> csv(csvFilePath);

  csv.read_header(io::ignore_no_column,
                  "Node", "Category", "Type", "IsMandatory", "PossibleValues");

  std::string node, category, type, isMandatory, possibleValues;
  while(csv.read_row(node, category, type, isMandatory, possibleValues)){
    const OdimEntry e(node, category, type, isMandatory, possibleValues);
    OdimEntry* myEntry = entry_(e);
    if ( myEntry ) {
      myEntry->possibleValues = e.possibleValues;
    }
    else {
      entries.push_back(e);
    }
  }
}

OdimEntry* OdimStandard::entry_(const OdimEntry& e) {
  for (auto& en : entries) {
    if ( en.category == e.category &&
         en.type == e.type &&
         en.node == e.node ) return &en;
  }
  return nullptr;
}

} // end namespace myodim
