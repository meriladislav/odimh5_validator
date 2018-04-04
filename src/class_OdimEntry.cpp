// class_odimEntry.cpp
// class to work with ODIM specification entries
// Ladislav Meri, SHMU
// v_0.0, 04.2018

#include <stdexcept>
#include "class_OdimEntry.hpp"

namespace myodim {

OdimEntry::OdimEntry(const std::string nodeStr, const std::string categoryStr, 
                     const std::string typeStr, const std::string isMandatoryStr, 
                     const std::string possibleValuesStr){ 
  set(nodeStr, categoryStr, typeStr, isMandatoryStr, possibleValuesStr);
}
    
void OdimEntry::set(const std::string nodeStr, const std::string categoryStr, 
                    const std::string typeStr, const std::string isMandatoryStr, 
                    const std::string possibleValuesStr) {
  node = nodeStr;
  parseCategory_(categoryStr);
  parseType_(typeStr);
  parseIsMandatory_(isMandatoryStr);
  possibleValues = possibleValuesStr;
}

void OdimEntry::parseCategory_(const std::string categoryStr) {
  if ( categoryStr == "Group" ) category = Category::Group;
  else if ( categoryStr == "Attribute" ) category = Category::Attribute;
  else if ( categoryStr == "Dataset" ) category = Category::Dataset;
  else throw std::invalid_argument{"Unknown category - "+categoryStr};
}
    
void OdimEntry::parseType_(const std::string typeStr) {
  if ( isGroup() || isDataset() ) {
    type = undefined;
    return;
  }
  if ( typeStr == "string" ) type = Type::string;
  else if ( typeStr == "real" ) type = Type::real;
  else if ( typeStr == "integer" ) type = Type::integer;
  else throw std::invalid_argument{"Unknown type - "+typeStr};
}
    
void OdimEntry::parseIsMandatory_(const std::string isMandatoryStr) {
  isMandatory = isMandatoryStr == "TRUE" ? true : false;
}
    
} //end namespace myodim
