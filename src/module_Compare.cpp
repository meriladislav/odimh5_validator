// module_Compare.cpp
// functions to compare the hdf5 layout with the ODIM specification
// Ladislav Meri, SHMU
// v_0.0, 04.2018

#include <cstdlib>  // getenv
#include <stdexcept>
#include <algorithm> // replace
#include <regex>
#include <iostream>
#include "module_Compare.hpp"

namespace myodim {

const std::string csvDirPathEnv{"ODIMH5_VALIDATOR_CSV_DIR"};
static bool checkCompliance(const myh5::H5Layout& h5layout, const OdimStandard& odimStandard);
bool checkExtraFeatures(const myh5::H5Layout& h5layout, const OdimStandard& odimStandard);

std::string getCsvFileNameFrom(const myh5::H5Layout& h5layout) {
  std::string csvFileName;
  const char* csvDir = std::getenv(csvDirPathEnv.c_str());
  if ( csvDir ) {
    csvFileName = csvDir;
  }
  else {
    throw std::runtime_error{"ERROR - environment variable "+csvDirPathEnv+" not found. "+
                             "Please specify it: export "+csvDirPathEnv+"=your_csv_data_directory_path"};
  }
  if ( csvFileName.back() != '/' ) csvFileName += "/";
  
  std::string conventions;
  h5layout.getAttributeValue("/Conventions", conventions);
  if ( conventions.empty() ) {
    throw std::runtime_error{"ERROR - file "+h5layout.filePath()+
                             " has no /Conventions attribute, probably not an ODIM_H5 file."};
  }
  std::replace(conventions.begin(), conventions.end(), '/', '_');
  
  std::string object;
  h5layout.getAttributeValue("/what/object", object);
  if ( object.empty() ) {
    throw std::runtime_error{"ERROR - file "+h5layout.filePath()+
                             " has no /what/object attribute, probably not an ODIM_H5 file."};
  }
  
  csvFileName += conventions+"_"+object+".csv";
   
  return csvFileName;
}

bool compare(const myh5::H5Layout& h5layout, const OdimStandard& odimStandard) {
  
  bool isCompliant = checkCompliance(h5layout, odimStandard);
  
  bool extrasPresent = checkExtraFeatures(h5layout, odimStandard);
  
  return isCompliant;
}

bool checkCompliance(const myh5::H5Layout& h5layout, const OdimStandard& odimStandard) {
  bool isCompliant{true};
  
  for (const auto& entry : odimStandard.entries) {
    
    bool entryExists{false};
    bool hasProperDatatype{true};
    bool hasProperValue{true};
    
    std::regex nodeRegex{entry.node};
    std::string failedValueMessage;
    //std::cout << "dbg - checking " << entry.node << std::endl; 
    switch (entry.category) {
      case OdimEntry::Group :
        for (const auto& g : h5layout.groups) {
          if ( std::regex_match(g, nodeRegex) ) {
            entryExists = true;
            break;
          }
        }
        break;
      case OdimEntry::Dataset :
        for (const auto& d : h5layout.datasets) {
          if ( std::regex_match(d, nodeRegex) ) {
            entryExists = true;
            break;
          }
        }
        break;
      case OdimEntry::Attribute :
        for (const auto& a : h5layout.attributes) {
          if ( std::regex_match(a, nodeRegex) ) {
            entryExists = true;
            switch (entry.type) {
              case OdimEntry::string :
                hasProperDatatype = h5layout.isStringAttribute(a);
                if ( !entry.possibleValues.empty() ) {
                  std::regex valueRegex{entry.possibleValues};
                  std::string value;
                  h5layout.getAttributeValue(a, value);
                  hasProperValue = std::regex_match(value, valueRegex);
                  if ( !hasProperValue ) {
                    failedValueMessage = "with value \"" + value + "\" doesn`t match the " + 
                                         entry.possibleValues + " assumed value";
                  }
                }
                if ( !hasProperDatatype || !hasProperValue ) goto end_attribute_loop;
                break;
              case OdimEntry::real :
                hasProperDatatype = h5layout.isReal64Attribute(a);
                break;
              case OdimEntry::integer :
                hasProperDatatype = h5layout.isInt64Attribute(a);
                break;
              default :
                break;
            }
            break;
          }
        }
        end_attribute_loop:
        break;
      default :
        break;
    }
    
    if ( !entryExists ) {
      if ( entry.isMandatory) {
        isCompliant = false;
        std::cout << "WARNING - mandatory entry \"" << entry.node << "\" doesn`t exist in the file." << std::endl;
      }
      else {
        std::cout << "INFO - optional entry \"" << entry.node << "\" doesn`t exist in the file." << std::endl;
      }
    }
    
    if ( !hasProperDatatype ) {
      std::string message;
      if ( entry.isMandatory) {
        isCompliant = false;
        message = "WARNING - mandatory ";
      }
      else {
        message = "INFO - optional ";
      }
      switch (entry.type) {
        case OdimEntry::string :
          message += "entry \"" + entry.node + "\" has non-standard datatype - it`s supposed to be a string, but isn`t.";
          break;
        case OdimEntry::real :
          message += "entry \"" + entry.node + "\" has non-standard datatype - it`s supposed to be a 64-bit real, but isn`t.";
          break;
        case OdimEntry::integer :
          message += "entry \"" + entry.node + "\" has non-standard datatype - it`s supposed to be a 64-bit integer, but isn`t.";
          break;
        default :
          break;
      }
      std::cout << message << std::endl;
    }
    
    if ( !hasProperValue ) {
      std::string message;
      if ( entry.isMandatory) {
        isCompliant = false;
        message = "WARNING - mandatory ";
      }
      else {
        message = "INFO - optional ";
      }
      message += "entry \"" + entry.node + "\" " + failedValueMessage + ".";
      std::cout << message << std::endl;
    }
    
  }
  
  return isCompliant;
}

bool checkExtraFeatures(const myh5::H5Layout& h5layout, const OdimStandard& odimStandard) {
  bool extrasPresent{false};
  
  for (const auto& group : h5layout.groups) {
    bool isExtra{true};
    for (const auto& entry : odimStandard.entries) {
      if ( entry.category != OdimEntry::Group ) continue;
      std::regex nodeRegex{entry.node};
      if ( std::regex_match(group, nodeRegex) ) isExtra = false;
    }
    if ( isExtra ) {
      std::cout << "INFO - extra feature - entry \"" + group + "\" is not mentioned in the standard." << std::endl;
      extrasPresent = true;
    }
  }
  
  for (const auto& dataset : h5layout.datasets) {
    bool isExtra{true};
    for (const auto& entry : odimStandard.entries) {
      if ( entry.category != OdimEntry::Dataset ) continue;
      std::regex nodeRegex{entry.node};
      if ( std::regex_match(dataset, nodeRegex) ) isExtra = false;
    }
    if ( isExtra ) {
      std::cout << "INFO - extra feature - entry \"" + dataset + "\" is not mentioned in the standard." << std::endl;
      extrasPresent = true;
    }
  }
  
  for (const auto& attribute : h5layout.attributes) {
    bool isExtra{true};
    for (const auto& entry : odimStandard.entries) {
      if ( entry.category != OdimEntry::Attribute ) continue;
      std::regex nodeRegex{entry.node};
      if ( std::regex_match(attribute, nodeRegex) ) isExtra = false;
    }
    if ( isExtra ) {
      std::cout << "INFO - extra feature - entry \"" + attribute + "\" is not mentioned in the standard." << std::endl;
      extrasPresent = true;
    }
    if ( !(h5layout.isInt64Attribute(attribute) || 
           h5layout.isReal64Attribute(attribute) || 
           h5layout.isStringAttribute(attribute)) ) {
      std::cout << "INFO - extra feature - entry \"" + attribute + "\" has non-standard datatype." << std::endl;
    }
  }
  
  return extrasPresent;
}


}
