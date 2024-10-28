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

std::string OdimEntry::categoryToString() const {
  return category == OdimEntry::Category::Group ? "Group" :
         (category == OdimEntry::Category::Dataset ? "Dataset" : "Attribute");
}

std::string OdimEntry::typeToString() const {
  switch ( type ) {
    case OdimEntry::Type::String :
      return "string";
      break;
    case OdimEntry::Type::Real :
      return "real";
      break;
    case OdimEntry::Type::Integer :
      return "integer";
      break;
    case OdimEntry::Type::StringArray :
      return "string array";
      break;
    case OdimEntry::Type::RealArray :
      return "real array";
      break;
    case OdimEntry::Type::IntegerArray :
      return "integer array";
      break;
    case OdimEntry::Type::StringArray2D :
      return "string array 2d";
      break;
    case OdimEntry::Type::RealArray2D :
      return "real array 2d";
      break;
    case OdimEntry::Type::IntegerArray2D :
      return "integer array 2d";
      break;
    case OdimEntry::Type::Link :
      return "link";
      break;
    case OdimEntry::Type::Undefined :
      return "";
      break;
    default :
      throw std::runtime_error("ERROR - unknown OdimEntry type");
      break;
  }
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
  if ( containsSubString(lowerTypeStr, "string") ) {
    if ( containsSubString(lowerTypeStr, "array") )
      type = containsSubString(lowerTypeStr, "2d") ? Type::StringArray2D : Type::StringArray;
    else
      type = Type::String;
  }
  else if ( containsSubString(lowerTypeStr, "real") ) {
    if ( containsSubString(lowerTypeStr, "array") )
      type = containsSubString(lowerTypeStr, "2d") ? Type::RealArray2D : Type::RealArray;
    else
      type = Type::Real;
  }
  else if ( containsSubString(lowerTypeStr, "integer") ) {
    if ( containsSubString(lowerTypeStr, "array") )
      type = containsSubString(lowerTypeStr, "2d") ? Type::IntegerArray2D : Type::IntegerArray;
    else
      type = Type::Integer;
  }
  else if ( containsSubString(lowerTypeStr, "link") ) {
    type = Type::Link;
  }
  else {
    throw std::invalid_argument{"Unknown type - "+typeStr};
  }
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
