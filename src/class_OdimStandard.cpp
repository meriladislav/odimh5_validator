// class_OdimStandard.cpp
// class to work with ODIM specification
// Ladislav Meri, SHMU
// v_0.0, 04.2018

#include <iostream>
#include <cstdio>
#include <stdexcept>
#include "csv.h"
#include "class_OdimStandard.hpp"

namespace myodim {

static const int CSV_COL_NUM{6};
static const char CSV_SEPARATOR{';'};

OdimStandard::OdimStandard(const std::string& csvFilePath) {
  readFromCsv(csvFilePath);
}

void OdimStandard::readFromCsv(const std::string& csvFilePath) {
  entries.clear();
  
  io::CSVReader<CSV_COL_NUM, io::trim_chars<CSV_SEPARATOR>,
                io::no_quote_escape<CSV_SEPARATOR>> csv(csvFilePath);

  csv.read_header(io::ignore_missing_column,
                  "Node", "Category", "Type", "IsMandatory", "PossibleValues", "Reference");
  if ( !csv.has_column("Node") || !csv.has_column("Category") || !csv.has_column("Type") ||
       !csv.has_column("IsMandatory") ) {
    throw io::error::missing_column_in_header();
  }

  std::string node, category, type, isMandatory, possibleValues="", reference="";
  while(csv.read_row(node, category, type, isMandatory, possibleValues, reference)){
    entries.emplace_back(OdimEntry{node, category, type, isMandatory, possibleValues, reference});
  }
}

void OdimStandard::updateWithCsv(const std::string& csvFilePath) {
  io::CSVReader<CSV_COL_NUM, io::trim_chars<CSV_SEPARATOR>,
                io::no_quote_escape<CSV_SEPARATOR>> csv(csvFilePath);

  csv.read_header(io::ignore_missing_column,
                  "Node", "Category", "Type", "IsMandatory", "PossibleValues", "Reference");
  if ( !csv.has_column("Node") || !csv.has_column("Category") || !csv.has_column("Type") ||
       !csv.has_column("IsMandatory") ) {
    throw io::error::missing_column_in_header();
  }

  std::string node, category, type, isMandatory, possibleValues="", reference="";
  while(csv.read_row(node, category, type, isMandatory, possibleValues, reference)){
    const OdimEntry e(node, category, type, isMandatory, possibleValues, reference);
    OdimEntry* myEntry = entry_(e);
    if ( myEntry ) {
      myEntry->possibleValues = e.possibleValues;
    }
    else {
      entries.push_back(e);
    }
  }
}

void OdimStandard::writeToCsv(const std::string& csvFilePath) {
  FILE* f = fopen(csvFilePath.c_str(), "w");
  if ( !f ) {
    throw std::runtime_error("ERROR - can not create file "+csvFilePath);
  }

  fprintf(f, "Node;Category;Type;IsMandatory;PossibleValues;Reference\n");
  for (const OdimEntry& e : entries) {
    fprintf(f, "%s;%s;%s;%s;%s;%s\n", e.node.c_str(), e.categoryToString().c_str(), e.typeToString().c_str(),
            e.isMandatory ? "TRUE" : "FALSE", e.possibleValues.c_str(), e.reference.c_str());
  }

  fclose(f);
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
