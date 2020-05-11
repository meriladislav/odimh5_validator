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
static const std::string WMO_REGEX = "(WMO:[0-9]{5})|(WMO:[0-9]{7})";
static const std::string NOD_REGEX = "NOD:[\\x00-\\x7F]*";  //only ASCII
static const std::string RAD_REGEX = "RAD:.*";
static const std::string PLC_REGEX = "PLC:.*";
static const std::string ORG_REGEX = "ORG:.[0-9]*";
static const std::string CTY_REGEX = "CTY:.[0-9]*";
static const std::string CMT_REGEX = "CMT:.*";
static const std::string WIGOS_REGEX = "WIGOS:[0-9]*-[0-9]*-[0-9]*-[\\x00-\\x7F]*";


static const std::string csvDirPathEnv{"ODIMH5_VALIDATOR_CSV_DIR"};
static bool checkCompliance(myodim::H5Layout& h5layout, const OdimStandard& odimStandard,
                            const bool checkOptional);
static bool checkExtraFeatures(const myodim::H5Layout& h5layout, const OdimStandard& odimStandard);
static bool ucharDatasetHasImageAttributes(const H5Layout& h5layout,
                                           const std::string dsetPath);
static bool checkMandatoryExitenceInAll(myodim::H5Layout& h5layout, const OdimStandard& odimStandard);
static void splitNodePath(const std::string& node, std::string& parent, std::string& child);
static void addIfUnique(std::vector<std::string>& list, const std::string& str);
static bool hasIntervalSigns(const std::string& assumedValueStr);
static bool checkValueInterval(const double attrValue, const std::string& assumedValueStr,
                               std::string& errorMessage);
static bool checkWhatSourceParts(const std::string& whatSource, std::string& errorMessage);
static std::vector<std::string> splitString(std::string str, const std::string& delimiter);


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
              case OdimEntry::String :
                hasProperDatatype = h5layout.isStringAttribute(a.name());
                if ( !entry.possibleValues.empty() ) {
                  std::string value;
                  try {
                    h5layout.getAttributeValue(a.name(), value);
                  }
                  catch (const std::exception& e) {
                	hasProperDatatype = false;
                	const std::string message(e.what());
                	if ( message.find("WARNING") != std::string::npos ) {
                	  std::cout << message << std::endl;
                	}
                	else {
                      throw(e);
                	}
                  }
                  if ( hasProperDatatype ) {
                    std::regex valueRegex{entry.possibleValues};
                    hasProperValue = checkValue(value, entry.possibleValues, failedValueMessage);
                    if ( a.name() == "/what/source" && hasProperValue ) {
                      hasProperValue = checkWhatSource(value, entry.possibleValues, failedValueMessage);
                    }
                  }
                }
                if ( !hasProperDatatype || !hasProperValue ) goto end_attribute_loop;
                break;
              case OdimEntry::Real :
                hasProperDatatype = h5layout.isReal64Attribute(a.name());
                if ( !entry.possibleValues.empty() ) {
                  double value=0.0;
                  h5layout.getAttributeValue(a.name(), value);
                  hasProperValue = checkValue(value, entry.possibleValues, failedValueMessage);
                }
                break;
              case OdimEntry::Integer :
                hasProperDatatype = h5layout.isInt64Attribute(a.name());
                if ( !entry.possibleValues.empty() ) {
                  int64_t value=0;
                  h5layout.getAttributeValue(a.name(), value);
                  hasProperValue = checkValue(value, entry.possibleValues, failedValueMessage);
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
        case OdimEntry::String :
          message += "entry \"" + entry.node + "\" has non-standard datatype - it`s supposed to be a fixed-length string, but isn`t.";
          break;
        case OdimEntry::Real :
          message += "entry \"" + entry.node + "\" has non-standard datatype - it`s supposed to be a 64-bit real, but isn`t.";
          break;
        case OdimEntry::Integer :
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

bool isStringValue(const std::string& value) {
  return std::find_if(value.begin(), value.end(), isalpha) != value.end();
}

bool hasDoublePoint(const std::string& value) {
  return std::find(value.begin(), value.end(), '.') != value.end();
}

bool checkValue(const std::string& attrValue, const std::string& assumedValueStr,
                std::string& errorMessage) {
  std::regex valueRegex{assumedValueStr};
  bool hasProperValue = std::regex_match(attrValue, valueRegex);
  if ( !hasProperValue ) {
    errorMessage = "with value \"" + attrValue + "\" doesn`t match the \"" +
                   assumedValueStr + "\" assumed value";
  }
  return hasProperValue;
}

bool checkValue(const double attrValue, const std::string& assumedValueStr,
                std::string& errorMessage, const bool isReal) {
  bool hasProperValue = true;
  if ( hasIntervalSigns(assumedValueStr) ) {

    hasProperValue = checkValueInterval(attrValue, assumedValueStr, errorMessage);

  }
  else {

    const double assumedValue = std::stod(assumedValueStr);
    hasProperValue = std::fabs(attrValue - assumedValue) < MAX_DOUBLE_DIFF;
    if ( !hasProperValue ) {
      const std::string attrValueStr = hasDoublePoint(assumedValueStr) || isReal ?
                                       std::to_string(attrValue) :
                                       std::to_string((int)attrValue);
      errorMessage = "with value \"" + attrValueStr +
                     "\" doesn`t match the \"" + assumedValueStr + "\" assumed value";
    }

  }
  return hasProperValue;
}

bool hasIntervalSigns(const std::string& assumedValueStr) {
  return assumedValueStr.find('=') != std::string::npos ||
         assumedValueStr.find('<') != std::string::npos ||
         assumedValueStr.find('>') != std::string::npos;
}

bool checkValueInterval(const double attrValue, const std::string& assumedValueStr,
                        std::string& errorMessage) {
  //split interval description
  const std::string andDelimiter = "&&";
  bool isAnd = false;
  const std::string orDelimiter = "||";
  bool isOr = false;
  auto splitPosi = assumedValueStr.find(andDelimiter);
  if ( splitPosi==std::string::npos ) {
    splitPosi = assumedValueStr.find(orDelimiter);
    isOr = splitPosi!=std::string::npos;
  }
  else {
    isAnd = true;
  }
  std::string first = assumedValueStr.substr(0,splitPosi);
  std::string second = splitPosi!=std::string::npos ?
                       assumedValueStr.substr(splitPosi+andDelimiter.length()) : "";
  //std::cout << "dbg - first = " << first << ", second = " << second << std::endl;

  //get first sign and number
  std::string firstSign, firstNumberStr;
  for (int i=0; i<(int)first.length(); ++i) {
    if ( (first[i] >= '0' && first[i] <= '9') || first[i] <= '.' ) {
      firstNumberStr.append(1, first[i]);
    }
    else {
      firstSign.append(1, first[i]);
    }
  }
  const double firstNumber = firstNumberStr.empty() ? std::nan("") : std::stod(firstNumberStr);

  //get second sign and number
  std::string secondSign, secondNumberStr;
  for (int i=0; i<(int)second.length(); ++i) {
    if ( (second[i] >= '0' && second[i] <= '9') || second[i] <= '.' ) {
      secondNumberStr.append(1, second[i]);
    }
    else {
      secondSign.append(1, second[i]);
    }
  }
  const double secondNumber = secondNumberStr.empty() ? std::nan("") : std::stod(secondNumberStr);
  //std::cout << "dbg - firstSign = " << firstSign << ", firstNumber = " << firstNumber <<
  //             ", secondSign = " << secondSign << ", secondNumber = " << secondNumber << std::endl;

  bool result = false;
  if ( firstSign == "=" || firstSign == "==" ) {
    result = std::fabs(attrValue - firstNumber) < MAX_DOUBLE_DIFF;
  }
  else if ( firstSign == "<=" ) {
    result = attrValue <= firstNumber;
  }
  else if ( firstSign == "<" ) {
    result = attrValue < firstNumber;
  }
  else if ( firstSign == ">=" ) {
    result = attrValue >= firstNumber;
  }
  else if ( firstSign == ">" ) {
    result = attrValue > firstNumber;
  }

  if ( !std::isnan(secondNumber) ) {
    if ( isAnd ) {
      if ( secondSign == "=" || secondSign == "==" ) {
        result = result && std::fabs(attrValue - secondNumber) < MAX_DOUBLE_DIFF;
      }
      else if ( secondSign == "<=" ) {
        result = result && attrValue <= secondNumber;
      }
      else if ( secondSign == "<" ) {
        result = result && attrValue < secondNumber;
      }
      else if ( secondSign == ">=" ) {
        result = result && attrValue >= secondNumber;
      }
      else if ( secondSign == ">" ) {
        result = result && attrValue > secondNumber;
      }
    }
    else if ( isOr ) {
      if ( secondSign == "=" || secondSign == "==" ) {
        result = result || std::fabs(attrValue - secondNumber) < MAX_DOUBLE_DIFF;
      }
      else if ( secondSign == "<=" ) {
        result = result || attrValue <= secondNumber;
      }
      else if ( secondSign == "<" ) {
        result = result || attrValue < secondNumber;
      }
      else if ( secondSign == ">=" ) {
        result = result || attrValue >= secondNumber;
      }
      else if ( secondSign == ">" ) {
        result = result || attrValue > secondNumber;
      }
    }
    else {
      throw std::runtime_error("ERROR - unknown sign in the "+
                               assumedValueStr+" PossibleValues string");
    }
  }

  //std::cout << "dbg - result = " << result << std::endl;
  if ( result ) {
    errorMessage = "";
  }
  else {
    const std::string attrValueStr = hasDoublePoint(assumedValueStr) ?
                                     std::to_string(attrValue) :
                                     std::to_string((int)attrValue);
    errorMessage = "with value \"" + attrValueStr +
                   "\" doesn`t match the \"" + assumedValueStr + "\" assumed value";
  }

  return result;
}

bool checkWhatSource(const std::string& whatSource, const std::string& basicRegex,
                     std::string& errorMessage) {
  bool result = checkValue(whatSource, basicRegex, errorMessage);
  result = result && checkWhatSourceParts(whatSource, errorMessage);
  return result;
}

bool checkWhatSourceParts(const std::string& whatSource, std::string& errorMessage) {
  bool result = false;

  std::vector<std::string> whatSourceParts = splitString(whatSource, ",");
  result = !whatSourceParts.empty();
  for (const auto& id : whatSourceParts) {
    std::string idErrorMessage = "";
    if ( id.find("WMO") != std::string::npos ) {
      result = result && checkValue(id, WMO_REGEX, idErrorMessage);
    }
    else if ( id.find("NOD") != std::string::npos ) {
      result = result && checkValue(id, NOD_REGEX, idErrorMessage);
    }
    else if ( id.find("RAD") != std::string::npos ) {
      result = result && checkValue(id, RAD_REGEX, idErrorMessage);
    }
    else if ( id.find("PLC") != std::string::npos ) {
      result = result && checkValue(id, PLC_REGEX, idErrorMessage);
    }
    else if ( id.find("ORG") != std::string::npos ) {
      result = result && checkValue(id, ORG_REGEX, idErrorMessage);
    }
    else if ( id.find("CTY") != std::string::npos ) {
      result = result && checkValue(id, CTY_REGEX, idErrorMessage);
    }
    else if ( id.find("CMT") != std::string::npos ) {
      result = result && checkValue(id, CMT_REGEX, idErrorMessage);
    }
    else if ( id.find("WIGOS") != std::string::npos ) {
      result = result && checkValue(id, WIGOS_REGEX, idErrorMessage);
    }
    else {
      idErrorMessage = "source type in /what/source - " + id + " - not defined by the ODIM standard";
      result = result && false;
    }

    if ( !idErrorMessage.empty() ) errorMessage += idErrorMessage;
  }
  return result;
}

std::vector<std::string> splitString(std::string str, const std::string& delimiter) {
  std::vector<std::string> result;
  if ( str.find(delimiter) == std::string::npos ) {
    result.push_back(str);
  }
  else {
    size_t pos = 0;
    std::string token;
    while ((pos = str.find(delimiter)) != std::string::npos) {
      token = str.substr(0, pos);
      result.push_back(token);
      str.erase(0, pos + delimiter.length());
    }
    result.push_back(str);
  }
  return result;
}

}
