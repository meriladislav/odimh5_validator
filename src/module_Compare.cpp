// module_Compare.cpp
// functions to compare the hdf5 layout with the ODIM specification
// Ladislav Meri, SHMU
// v_0.0, 04.2018

#include <cstdlib>  // getenv
#include <stdexcept>
#include <algorithm> // replace, count
#include <regex>
#include <iostream>
#include <cmath> //fabs
#include "module_Compare.hpp"

namespace myodim {

bool printInfo{true};
static const double MAX_DOUBLE_DIFF = 0.001;

static const std::string csvDirPathEnv{"ODIMH5_VALIDATOR_CSV_DIR"};
static bool checkCompliance(myodim::H5Layout& h5layout, const OdimStandard& odimStandard,
                            const bool checkOptional);
static bool checkExtraFeatures(const myodim::H5Layout& h5layout, const OdimStandard& odimStandard);
static bool ucharDatasetHasImageAttributes(const H5Layout& h5layout,
                                           const std::string dsetPath);
static bool checkMandatoryExitenceInAll(myodim::H5Layout& h5layout, const OdimStandard& odimStandard);
static void splitNodePath(const std::string& node, std::string& parent, std::string& child);
static void addIfUnique(std::vector<std::string>& list, const std::string& str);

std::string getCsvFileNameFrom(const myodim::H5Layout& h5layout) {
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

std::string getCsvFileNameFrom(const myodim::H5Layout& h5layout, std::string version) { 
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
  
  std::replace(version.begin(), version.end(), '.', '_');
  csvFileName += "ODIM_H5_V" + version;
  
  std::string object;
  h5layout.getAttributeValue("/what/object", object);
  if ( object.empty() ) {
    throw std::runtime_error{"ERROR - file "+h5layout.filePath()+
                             " has no /what/object attribute, probably not an ODIM_H5 file."};
  }
  
  csvFileName += "_"+object+".csv";
  return csvFileName;
}

bool compare(myodim::H5Layout& h5layout, const OdimStandard& odimStandard, 
             const bool checkOptional, const bool checkExtras) {
  
  bool isCompliant = checkCompliance(h5layout, odimStandard, checkOptional) ;
  bool mandatoryExistsInAll = checkMandatoryExitenceInAll(h5layout, odimStandard);
  isCompliant = isCompliant && mandatoryExistsInAll;

  if ( checkExtras ) checkExtraFeatures(h5layout, odimStandard);
  
  return isCompliant;
}

bool checkCompliance(myodim::H5Layout& h5layout, const OdimStandard& odimStandard,
                     const bool checkOptional) {
  bool isCompliant{true};
  
  for (const auto& entry : odimStandard.entries) {
    
    if ( !checkOptional && !entry.isMandatory ) continue;
    
    bool entryExists{false};
    bool hasProperDatatype{true};
    bool hasProperValue{true};
    bool ucharDatasetHasProperAttributes{true};
    
    std::regex nodeRegex{entry.node};
    std::string failedValueMessage;
    
    switch (entry.category) {
      case OdimEntry::Group :
        for (auto& g : h5layout.groups) {
          if ( std::regex_match(g.name(), nodeRegex) ) {
            entryExists = true;
            g.wasFound() = true;
          }
        }
        break;
      case OdimEntry::Dataset :
        for (auto& d : h5layout.datasets) {
          if ( std::regex_match(d.name(), nodeRegex) ) {
            entryExists = true;
            d.wasFound() = true;
            if ( h5layout.isUcharDataset(d.name()) ) {
              ucharDatasetHasProperAttributes = ucharDatasetHasImageAttributes(h5layout, d.name());
            }
          }
        }
        break;
      case OdimEntry::Attribute :
        for (auto& a : h5layout.attributes) {
          if ( std::regex_match(a.name(), nodeRegex) ) {
            entryExists = true;
            a.wasFound() = true;
            switch (entry.type) {
              case OdimEntry::string :
                hasProperDatatype = h5layout.isStringAttribute(a.name());
                if ( !entry.possibleValues.empty() ) {
                  std::string value;
                  h5layout.getAttributeValue(a.name(), value);
                  std::regex valueRegex{entry.possibleValues};
                  hasProperValue = std::regex_match(value, valueRegex);
                  if ( !hasProperValue ) {
                    failedValueMessage = "with value \"" + value + "\" doesn`t match the \"" +
                                         entry.possibleValues + "\" assumed value";
                  }
                }
                if ( !hasProperDatatype || !hasProperValue ) goto end_attribute_loop;
                break;
              case OdimEntry::real :
                hasProperDatatype = h5layout.isReal64Attribute(a.name());
                if ( !entry.possibleValues.empty() ) {
                  double value=0.0;
                  h5layout.getAttributeValue(a.name(), value);
                  const double assumedValue = std::stod(entry.possibleValues);
                  hasProperValue = std::fabs(value - assumedValue) < MAX_DOUBLE_DIFF;
                  if ( !hasProperValue ) {
                    failedValueMessage = "with value \"" + std::to_string(value) +
                                         "\" doesn`t match the \"" +
                                         entry.possibleValues + "\" assumed value";
                  }
                }
                break;
              case OdimEntry::integer :
                hasProperDatatype = h5layout.isInt64Attribute(a.name());
                if ( !entry.possibleValues.empty() ) {
                  int64_t value=0;
                  h5layout.getAttributeValue(a.name(), value);
                  const int64_t assumedValue = std::stoi(entry.possibleValues);
                  hasProperValue = value == assumedValue;
                  if ( !hasProperValue ) {
                    failedValueMessage = "with value \"" + std::to_string(value) +
                                         "\" doesn`t match the \"" +
                                         entry.possibleValues + "\" assumed value";
                  }
                }
                break;
              default :
                break;
            }
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
        std::cout << "WARNING - MISSING ENTRY - mandatory entry \"" << entry.node <<
                     "\" doesn`t exist in the file." << std::endl;
      }
      else {
        if ( printInfo ) 
          std::cout << "INFO - optional entry \"" << entry.node <<
                       "\" doesn`t exist in the file." << std::endl;
      }
    }
    
    if ( !hasProperDatatype ) {
      std::string message;
      if ( entry.isMandatory) {
        isCompliant = false;
        message = "WARNING - NON-STANDARD DATA TYPE - mandatory ";
      }
      else {
        isCompliant = false;
        message = "WARNING - NON-STANDARD DATA TYPE - optional ";
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
        message = "WARNING - INCORRECT VALUE - mandatory ";
      }
      else {
        isCompliant = false;
        message = "WARNING - INCORRECT VALUE - optional ";
      }
      message += "entry \"" + entry.node + "\" " + failedValueMessage + ".";
      std::cout << message << std::endl;
    }
    
    if ( !ucharDatasetHasProperAttributes ) {
      isCompliant = false;
      std::string message = "WARNING -  dataset \"" + entry.node + "\" " +
    		                " is 8-bit unsigned int - it should have attributes CLASS=\"IMAGE\" and IMAGE_VERSION=\"1.2\"";
      std::cout << message << std::endl;
    }

  }
  
  return isCompliant;
}

bool checkExtraFeatures(const myodim::H5Layout& h5layout, const OdimStandard& odimStandard) {
  bool extrasPresent{false};
  
  for (const auto& group : h5layout.groups) {
    if ( group.wasFound() ) continue;
    bool isExtra{true};
    for (const auto& entry : odimStandard.entries) {
      if ( entry.category != OdimEntry::Group ) continue;
      std::regex nodeRegex{entry.node};
      if ( std::regex_match(group.name(), nodeRegex) ) isExtra = false;
    }
    if ( isExtra ) {
      if ( printInfo) std::cout << "INFO - extra feature - entry \"" + group.name() + "\" is not mentioned in the standard." << std::endl;
      extrasPresent = true;
    }
  }
  
  for (const auto& dataset : h5layout.datasets) {
    if ( dataset.wasFound() ) continue;
    bool isExtra{true};
    for (const auto& entry : odimStandard.entries) {
      if ( entry.category != OdimEntry::Dataset ) continue;
      std::regex nodeRegex{entry.node};
      if ( std::regex_match(dataset.name(), nodeRegex) ) isExtra = false;
    }
    if ( isExtra ) {
      if ( printInfo) std::cout << "INFO - extra feature - entry \"" + dataset.name() + "\" is not mentioned in the standard." << std::endl;
      extrasPresent = true;
    }
  }
  
  for (const auto& attribute : h5layout.attributes) {
    if ( attribute.wasFound() ) continue;
    bool isExtra{true};
    for (const auto& entry : odimStandard.entries) {
      if ( entry.category != OdimEntry::Attribute ) continue;
      std::regex nodeRegex{entry.node};
      if ( std::regex_match(attribute.name(), nodeRegex) ) isExtra = false;
    }
    if ( isExtra ) {
      if ( printInfo) std::cout << "INFO - extra feature - entry \"" + attribute.name() + "\" is not mentioned in the standard." << std::endl;
      extrasPresent = true;
      if ( !(h5layout.isInt64Attribute(attribute.name()) || 
             h5layout.isReal64Attribute(attribute.name()) || 
             h5layout.isStringAttribute(attribute.name())) ) {
        if ( printInfo) std::cout << "INFO - extra feature - entry \"" + attribute.name() + "\" has non-standard datatype." << std::endl;
      }
    }
  }
  
  return extrasPresent;
}

bool ucharDatasetHasImageAttributes(const H5Layout& h5layout,
		                            const std::string dsetPath) {
  if ( !h5layout.hasAttribute(dsetPath+"/CLASS") ) return false;
  if ( !h5layout.hasAttribute(dsetPath+"/IMAGE_VERSION") ) return false;
  std::string attrValue;
  h5layout.getAttributeValue(dsetPath+"/CLASS", attrValue);
  if ( attrValue != "IMAGE" ) return false;
  h5layout.getAttributeValue(dsetPath+"/IMAGE_VERSION", attrValue);
  if ( attrValue != "1.2" ) return false;
  return true;
}

bool checkMandatoryExitenceInAll(myodim::H5Layout& h5layout, const OdimStandard& odimStandard) {
  bool isCompliant = true;

  for (const auto& entry : odimStandard.entries) {
    if ( !entry.isMandatory ) continue;

    //if node contains some wildcard, check ALL h5layout elements which fulfill the given regex
    if ( entry.node.find('*') != std::string::npos ||
         entry.node.find('[') != std::string::npos ||
         entry.node.find('?') != std::string::npos ) {

      std::string parent{""}, child{""};
      splitNodePath(entry.node, parent, child);
      if ( parent.empty() ) continue;

      std::regex nodeRegex{entry.node};
      std::string parentEnxtended = parent + "/[0-z]*";
      std::regex parentRegex{parentEnxtended};

      std::vector<std::string> parents;
      std::vector<std::string> entries;
      switch (entry.category) {
        case OdimEntry::Group :
          for (auto& g : h5layout.groups) {
            if ( std::regex_match(g.name(), parentRegex)  ) {
              std::string p, c;
              splitNodePath(g.name(), p, c);
              addIfUnique(parents, p);
            }
            if ( std::regex_match(g.name(), nodeRegex)  ) {
              std::string p, c;
              splitNodePath(g.name(), p, c);
              addIfUnique(entries, p);
            }
          }
          break;
        case OdimEntry::Dataset :
          for (auto& d : h5layout.datasets) {
            if ( std::regex_match(d.name(), parentRegex) ) {
              std::string p, c;
              splitNodePath(d.name(), p, c);
              addIfUnique(parents, p);
            }
            if ( std::regex_match(d.name(), nodeRegex)  ) {
              std::string p, c;
              splitNodePath(d.name(), p, c);
              addIfUnique(entries, p);
            }
          }
          break;
        case OdimEntry::Attribute :
          for (auto& a : h5layout.attributes) {
            if ( std::regex_match(a.name(), parentRegex) ) {
              std::string p, c;
              splitNodePath(a.name(), p, c);
              addIfUnique(parents, p);
            }
            if ( std::regex_match(a.name(), nodeRegex)  ) {
              std::string p, c;
              splitNodePath(a.name(), p, c);
              addIfUnique(entries, p);
            }
          }
          break;
        default:
          break;
      }

      if ( parents.size() != entries.size() ) {
        if ( entries.size() > 0 ) {
          for (int i=0, j=0, n=parents.size(); i<n; ++i) {
            if ( entries[j].find(parents[i]) != std::string::npos ) {
              j++;
              continue;
            }
            else {
              std::cout << "WARNING - MISSING ENTRY - mandatory entry \"" <<
                           entry.node << "\" not found in " << parents[i] << std::endl;
            }
          }
        }
        else {
          std::cout << "WARNING - MISSING ENTRY - mandatory entry \"" <<
                           entry.node << "\" not found in any its parents" << std::endl;
        }
        isCompliant = false;
      }

    }
  }

  return isCompliant;
}

void splitNodePath(const std::string& node, std::string& parent, std::string& child) {
  const auto splitPosi = node.rfind('/');
  if ( splitPosi == std::string::npos ) return;
  parent = node.substr(0,splitPosi);
  child = node.substr(splitPosi+1);
}

void addIfUnique(std::vector<std::string>& list, const std::string& str) {
  for (const auto& element : list) {
    if ( element == str ) return;
  }
  list.push_back(str);
}

}
