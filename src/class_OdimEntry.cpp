// class_OdimEntry.cpp
// class to work with ODIM specification entries
// Ladislav Meri, SHMU
// v_0.0, 04.2018

#include <stdexcept>
#include <algorithm>  //used in toLower
#include <cctype>     //used in toLower
#include "class_OdimEntry.hpp"

namespace myodim {

static std::string toLower(std::string str);
static bool containsSubString(const std::string& theString, const std::string& subString);

OdimEntry::OdimEntry(const std::string nodeStr, const std::string categoryStr, 
                     const std::string typeStr, const std::string isMandatoryStr, 
                     const std::string possibleValuesStr, const std::string referenceStr){
  set(nodeStr, categoryStr, typeStr, isMandatoryStr, possibleValuesStr, referenceStr);
}
    
void OdimEntry::set(const std::string nodeStr, const std::string categoryStr, 
                    const std::string typeStr, const std::string isMandatoryStr, 
                    const std::string possibleValuesStr, const std::string referenceStr) {
  node = nodeStr;
  parseCategory_(categoryStr);
  parseType_(typeStr);
  parseIsMandatory_(isMandatoryStr);
  possibleValues = possibleValuesStr;
  reference = referenceStr;
}

void OdimEntry::parseCategory_(const std::string categoryStr) {
  const std::string lowerCategoryStr = toLower(categoryStr);
  if ( lowerCategoryStr == "group" ) category = Category::Group;
  else if ( lowerCategoryStr == "attribute" ) category = Category::Attribute;
  else if ( lowerCategoryStr == "dataset" ) category = Category::Dataset;
  else throw std::invalid_argument{"Unknown category - "+categoryStr};
}
    
void OdimEntry::parseType_(const std::string typeStr) {
  if ( isGroup() || isDataset() ) {
    type = Undefined;
    return;
  }
  const std::string lowerTypeStr = toLower(typeStr);
  if ( containsSubString(lowerTypeStr, "string") )
    type = containsSubString(lowerTypeStr, "array") ? Type::StringArray : Type::String;
  else if ( containsSubString(lowerTypeStr, "real") )
    type = containsSubString(lowerTypeStr, "array") ? Type::RealArray : Type::Real;
  else if ( containsSubString(lowerTypeStr, "integer") )
    type = containsSubString(lowerTypeStr, "array") ? Type::IntegerArray : Type::Integer;
  else if ( containsSubString(lowerTypeStr, "boolean") )
    type = containsSubString(lowerTypeStr, "array") ? Type::BooleanArray : Type::Boolean;
  else throw std::invalid_argument{"Unknown type - "+typeStr};
}
    
void OdimEntry::parseIsMandatory_(const std::string isMandatoryStr) {
  isMandatory = toLower(isMandatoryStr) == "true" ? true : false;
}


// statics

std::string toLower(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(),
      [](unsigned char c){ return std::tolower(c); });
  return str;
}

bool containsSubString(const std::string& theString, const std::string& subString) {
  return theString.find(subString) != std::string::npos;
}
    
} //end namespace myodim
