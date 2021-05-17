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
static const double MAX_DOUBLE_DIFF = 0.0001;
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
                            const bool checkOptional, OdimStandard* failedEntries=nullptr);
static bool checkExtraFeatures(const myodim::H5Layout& h5layout, const OdimStandard& odimStandard);
static bool checkMandatoryExitenceInAll(myodim::H5Layout& h5layout, const OdimStandard& odimStandard,
                                        OdimStandard* failedEntries=nullptr);
static void splitNodePath(const std::string& node, std::string& parent, std::string& child);
static void addIfUnique(std::vector<std::string>& list, const std::string& str);
static bool hasIntervalSigns(const std::string& assumedValueStr);
static bool checkValueInterval(const double attrValue, const std::string& assumedValueStr);
static bool checkWhatSourceParts(const std::string& whatSource, std::string& errorMessage);
static std::vector<std::string> splitString(std::string str, const std::string& delimiter);
static void printWrongTypeMessage(const OdimEntry& entry, const h5Entry& attr);
static void printIncorrectValueMessage(const OdimEntry& entry, const h5Entry& attr,
                                       const std::string& failedValueMessage);
static void printWrongImageAttributes(const OdimEntry& entry);
static void parseAssumedValueStr(const std::string& assumedValueStr,
                                 std::vector<std::string>& comparisons,
                                 std::vector<std::string>& operators);
static void getArrayStatistics(const std::vector<double>& values, double& first, double& last,
                               double& min, double& max, double& mean);
static double valueFromStatistics(std::string& comparison,
                                  double first, double last, double min, double max, double mean);
static void splitPlusMinus(std::string pmString, double& center, double& interval);

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
             const bool checkOptional, const bool checkExtras,
             OdimStandard* failedEntries) {
  
  bool isCompliant = checkCompliance(h5layout, odimStandard, checkOptional, failedEntries) ;
  bool mandatoryExistsInAll = checkMandatoryExitenceInAll(h5layout, odimStandard, failedEntries);
  isCompliant = isCompliant && mandatoryExistsInAll;

  if ( checkExtras ) checkExtraFeatures(h5layout, odimStandard);
  
  return isCompliant;
}

bool checkCompliance(myodim::H5Layout& h5layout, const OdimStandard& odimStandard,
                     const bool checkOptional, OdimStandard* failedEntries) {
  bool isCompliant{true};
  
  if ( failedEntries ) failedEntries->entries.clear();

  for (const auto& entry : odimStandard.entries) {
    
    if ( !checkOptional && !entry.isMandatory ) continue;
    
    bool entryExists{false};
    
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
              if ( !h5layout.ucharDatasetHasImageAttributes(d.name()) ) {
                isCompliant = false;
                printWrongImageAttributes(entry);
                if ( failedEntries ) {
                  failedEntries->entries.push_back(
                    OdimEntry(d.name()+"/CLASS", "Attribute", "String", "True",
                             "IMAGE", "Section 5 in all ODIM-H5 version documents"));
                  failedEntries->entries.push_back(
                    OdimEntry(d.name()+"/IMAGE_VERSION", "Attribute", "String", "True",
                             "1.2", "Section 5 in all ODIM-H5 version documents"));
                }
              }
            }
          }
        }
        break;
      case OdimEntry::Attribute :
        for (auto& a : h5layout.attributes) {
          if ( std::regex_match(a.name(), nodeRegex) ) {
            bool hasProperDatatype, hasProperValue;
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
                	  std::string message(e.what());
                	  if ( failedEntries ) {
                      OdimEntry eFailed = entry;
                      eFailed.node = a.name();
                      failedEntries->entries.push_back(eFailed);
                    }
                	  if ( message.find("WARNING") != std::string::npos ) {
                	    if ( !entry.reference.empty() ) message += "see "+entry.reference;
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
                      if ( !hasProperValue ) {
                        isCompliant = false;
                        printIncorrectValueMessage(entry, a, failedValueMessage);
                        if ( failedEntries ) {
                          OdimEntry eFailed = entry;
                          eFailed.node = a.name();
                          failedEntries->entries.push_back(eFailed);
                        }
                      }
                    }
                  }
                  else {
                    isCompliant = false;
                    printWrongTypeMessage(entry, a);
                    if ( failedEntries ) {
                      OdimEntry eFailed = entry;
                      eFailed.node = a.name();
                      failedEntries->entries.push_back(eFailed);
                    }
                  }
                }
                break;
              case OdimEntry::Real :
                hasProperDatatype = h5layout.isReal64Attribute(a.name()) &&
                                   !h5layout.is1DArrayAttribute(a.name());
                if ( !hasProperDatatype ) {
                  isCompliant = false;
                  printWrongTypeMessage(entry, a);
                  if ( failedEntries ) {
                    OdimEntry eFailed = entry;
                    eFailed.node = a.name();
                    failedEntries->entries.push_back(eFailed);
                  }
                }
                if ( !entry.possibleValues.empty() ) {
                  double value=0.0;
                  h5layout.getAttributeValue(a.name(), value);
                  const bool isReal = true;
                  hasProperValue = checkValue(value, entry.possibleValues, failedValueMessage, isReal);
                  if ( !hasProperValue ) {
                    isCompliant = false;
                    printIncorrectValueMessage(entry, a, failedValueMessage);
                    if ( failedEntries ) {
                     OdimEntry eFailed = entry;
                     eFailed.node = a.name();
                     failedEntries->entries.push_back(eFailed);
                    }
                  }
                }
                break;
              case OdimEntry::RealArray :
                hasProperDatatype = h5layout.isReal64Attribute(a.name()) &&
                                    h5layout.is1DArrayAttribute(a.name());
                if ( !hasProperDatatype ) {
                  isCompliant = false;
                  printWrongTypeMessage(entry, a);
                  if ( failedEntries ) {
                    OdimEntry eFailed = entry;
                    eFailed.node = a.name();
                    failedEntries->entries.push_back(eFailed);
                  }
                }
                if ( !entry.possibleValues.empty() ) {
                  std::vector<double> values;
                  h5layout.getAttributeValue(a.name(), values);
                  hasProperValue = checkValue(values, entry.possibleValues, failedValueMessage);
                  if ( !hasProperValue ) {
                    isCompliant = false;
                    printIncorrectValueMessage(entry, a, failedValueMessage);
                    if ( failedEntries ) {
                      OdimEntry eFailed = entry;
                      eFailed.node = a.name();
                      failedEntries->entries.push_back(eFailed);
                    }
                  }
                }
                break;
              case OdimEntry::Integer :
                hasProperDatatype = h5layout.isInt64Attribute(a.name()) &&
                                   !h5layout.is1DArrayAttribute(a.name());
                if ( !hasProperDatatype ) {
                  isCompliant = false;
                  printWrongTypeMessage(entry, a);
                  if ( failedEntries ) {
                    OdimEntry eFailed = entry;
                    eFailed.node = a.name();
                    failedEntries->entries.push_back(eFailed);
                  }
                }
                if ( !entry.possibleValues.empty() ) {
                  int64_t value=0;
                  h5layout.getAttributeValue(a.name(), value);
                  hasProperValue = checkValue(value, entry.possibleValues, failedValueMessage);
                  if ( !hasProperValue ) {
                    isCompliant = false;
                    printIncorrectValueMessage(entry, a, failedValueMessage);
                    if ( failedEntries ) {
                      OdimEntry eFailed = entry;
                      eFailed.node = a.name();
                      failedEntries->entries.push_back(eFailed);
                    }
                  }
                }
                break;
              case OdimEntry::IntegerArray :
                hasProperDatatype = h5layout.isInt64Attribute(a.name()) &&
                                    h5layout.is1DArrayAttribute(a.name());
                if ( !hasProperDatatype ) {
                  isCompliant = false;
                  printWrongTypeMessage(entry, a);
                  if ( failedEntries ) {
                    OdimEntry eFailed = entry;
                    eFailed.node = a.name();
                    failedEntries->entries.push_back(eFailed);
                  }
                }
                if ( !entry.possibleValues.empty() ) {
                  std::vector<int64_t> values;
                  h5layout.getAttributeValue(a.name(), values);
                  std::vector<double> dValues(values.size());
                  for (int i=0, n=dValues.size(); i<n; ++i) dValues[i] = values[i];
                  hasProperValue = checkValue(dValues, entry.possibleValues, failedValueMessage);
                  if ( !hasProperValue ) {
                    isCompliant = false;
                    printIncorrectValueMessage(entry, a, failedValueMessage);
                    if ( failedEntries ) {
                      OdimEntry eFailed = entry;
                      eFailed.node = a.name();
                      failedEntries->entries.push_back(eFailed);
                    }
                  }
                }
                break;
              default :
                break;
            }
          }
        }
        break;
      default :
        break;
    }
    
    if ( !entryExists ) {
      if ( entry.isMandatory) {
        isCompliant = false;
        std::cout << "WARNING - MISSING ENTRY - mandatory entry \"" << entry.node <<
                     "\" doesn`t exist in the file.";
        if ( !entry.reference.empty() ) std::cout << " See " << entry.reference;
        std::cout << std::endl;
        if ( failedEntries ) {
          OdimEntry eFailed = entry;
          failedEntries->entries.push_back(eFailed);
        }
      }
      else {
        if ( printInfo ) {
          std::cout << "INFO - optional entry \"" << entry.node <<
                       "\" doesn`t exist in the file.";
          if ( !entry.reference.empty() ) std::cout << " See " << entry.reference;
          std::cout << std::endl;
        }
      }
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

bool checkMandatoryExitenceInAll(myodim::H5Layout& h5layout, const OdimStandard& odimStandard,
                                 OdimStandard* failedEntries) {
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
                           entry.node << "\" not found in " << parents[i] << ".";
              if ( !entry.reference.empty() ) std::cout << " See " << entry.reference;
              std::cout << std::endl;
              if ( failedEntries ) {
                std::string p, c;
                splitNodePath(entry.node, p, c);
                OdimEntry failedEntry = entry;
                failedEntry.node = parents[i]+"/"+c;
                failedEntries->entries.push_back(failedEntry);
              }
            }
          }
        }
        else {
          std::cout << "WARNING - MISSING ENTRY - mandatory entry \"" <<
                           entry.node << "\" not found in any its parents" << ".";
          if ( !entry.reference.empty() ) std::cout << " See " << entry.reference;
          std::cout << std::endl;
          if ( failedEntries ) failedEntries->entries.push_back(entry);
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
  errorMessage.clear();
  bool hasProperValue;
  if ( hasIntervalSigns(assumedValueStr) ) {

    std::vector<std::string> comparisons;
    std::vector<std::string> operators;
    parseAssumedValueStr(assumedValueStr, comparisons, operators);
    for (const auto& comp : comparisons) std::cout << "dbg - " << comp << std::endl;

    hasProperValue = checkValueInterval(attrValue, comparisons[0]);
    for (int i=0, n=operators.size(); i<n; ++i) {
      bool nextHasProperValue = checkValueInterval(attrValue, comparisons[i+1]);
      if ( operators[i] == "&&" ) {
        hasProperValue = hasProperValue && nextHasProperValue;
      }
      else {
        hasProperValue = hasProperValue || nextHasProperValue;
      }
    }

  }
  else {

    const double assumedValue = std::stod(assumedValueStr);
    hasProperValue = std::fabs(attrValue - assumedValue) < MAX_DOUBLE_DIFF;

  }

  if ( !hasProperValue ) {
    const std::string attrValueStr = hasDoublePoint(assumedValueStr) || isReal ?
                                     std::to_string(attrValue) :
                                     std::to_string((int)attrValue);
    errorMessage = "with value \"" + attrValueStr +
                   "\" doesn`t match the \"" + assumedValueStr + "\" assumed value";
  }

  return hasProperValue;
}

bool checkValue(const std::vector<double>& attrValues, const std::string& assumedValueStr,
                std::string& errorMessage) {
  errorMessage.clear();
  bool hasProperValue = false;

  if ( hasIntervalSigns(assumedValueStr) ) {

    double first, last, min, max, mean;
    getArrayStatistics(attrValues, first, last, min ,max, mean);

    std::vector<std::string> comparisons;
    std::vector<std::string> operators;
    parseAssumedValueStr(assumedValueStr, comparisons, operators);

    double value = valueFromStatistics(comparisons[0], first, last, min ,max, mean);
    hasProperValue = checkValueInterval(value, comparisons[0]);
    for (int i=0, n=operators.size(); i<n; ++i) {
      value = valueFromStatistics(comparisons[i+1], first, last, min ,max, mean);
      bool nextHasProperValue = checkValueInterval(value, comparisons[i+1]);
      if ( operators[i] == "&&" ) {
        hasProperValue = hasProperValue && nextHasProperValue;
      }
      else {
        hasProperValue = hasProperValue || nextHasProperValue;
      }
    }

  }
  else {
    throw std::runtime_error("ERROR - wrong assumed value string \"" + assumedValueStr + "\" - " +
                             "assumed value string for array attributes should use "+
                             "logical operators and statistics (e.g. min<=1.000 or mean==5.0 or first>1.0&&last<10.0 ...)");
  }

  if ( !hasProperValue ) {
    errorMessage = "with array value \"[" + std::to_string(attrValues.front()) + ",...," +
                   std::to_string(attrValues.back()) + "]\" doesn`t match the \"" +
                   assumedValueStr + "\" assumed value";
  }

  return hasProperValue;
}

bool hasIntervalSigns(const std::string& assumedValueStr) {
  return assumedValueStr.find('=') != std::string::npos ||
         assumedValueStr.find('<') != std::string::npos ||
         assumedValueStr.find('>') != std::string::npos ||
         assumedValueStr.find("+-") != std::string::npos;
}

bool checkValueInterval(const double attrValue, const std::string& assumedValueStr) {
  //get first sign and number
  std::string signStr, numberStr;
  bool hasPlusMinus = false;
  for (int i=0; i<(int)assumedValueStr.length(); ++i) {
    if ( (assumedValueStr[i] >= '0' && assumedValueStr[i] <= '9') || assumedValueStr[i] <= '.' ) {
      numberStr.append(1, assumedValueStr[i]);
      if ( assumedValueStr[i] == '-' && i > 0 && assumedValueStr[i-1] == '+') {
        hasPlusMinus = true;
      }
    }
    else {
      signStr.append(1, assumedValueStr[i]);
    }
  }

  bool result = false;

  if ( hasPlusMinus ) {
    double center, interval;
    splitPlusMinus(numberStr, center, interval);

    result = attrValue >= center-interval && attrValue <= center+interval;
  }
  else {
    const double theNumber = numberStr.empty() ? std::nan("") : std::stod(numberStr);

    if ( signStr == "=" || signStr == "==" ) {
      result = std::fabs(attrValue - theNumber) < MAX_DOUBLE_DIFF;
    }
    else if ( signStr == "<=" ) {
      result = attrValue <= theNumber;
    }
    else if ( signStr == "<" ) {
      result = attrValue < theNumber;
    }
    else if ( signStr == ">=" ) {
      result = attrValue >= theNumber;
    }
    else if ( signStr == ">" ) {
      result = attrValue > theNumber;
    }
    else {
      throw std::runtime_error("ERROR - unknown sign in the "+
                               assumedValueStr+" PossibleValues string");
    }
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

void printWrongTypeMessage(const OdimEntry& entry, const h5Entry& attr) {
  std::string message = "WARNING - NON-STANDARD DATA TYPE - ";
  message += entry.isMandatory ? "mandatory" : "optional";
  switch (entry.type) {
    case OdimEntry::String :
      message += " entry \"" + attr.name() + "\" has non-standard datatype - " +
                 "it`s supposed to be a fixed-length string, but isn`t. See section 3.1 in v2.1 (or higher) ODIM-H5 documetaton.";
      break;
    case OdimEntry::Real :
      message += " entry \"" + attr.name() + "\" has non-standard datatype - " +
                 "it`s supposed to be a 64-bit real scalar, but isn`t. See section 3.1 in v2.1 (or higher) ODIM-H5 documetaton.";
      break;
    case OdimEntry::RealArray :
      message += " entry \"" + attr.name() + "\" has non-standard datatype - " +
                 "it`s supposed to be a 64-bit real array, but isn`t. See section 3.1 in v2.1 (or higher) ODIM-H5 documetaton.";
      break;
    case OdimEntry::Integer :
      message += " entry \"" + attr.name() + "\" has non-standard datatype - " +
                 "it`s supposed to be a 64-bit integer scalar, but isn`t. See section 3.1 in v2.1 (or higher) ODIM-H5 documetaton.";
      break;
    case OdimEntry::IntegerArray :
      message += " entry \"" + attr.name() + "\" has non-standard datatype - " +
                 "it`s supposed to be a 64-bit integer array, but isn`t. See section 3.1 in v2.1 (or higher) ODIM-H5 documetaton.";
      break;
    default :
      break;
  }
  std::cout << message << std::endl;
}

void printIncorrectValueMessage(const OdimEntry& entry, const h5Entry& attr,
                                const std::string& failedValueMessage) {
  std::string message = "WARNING - INCORRECT VALUE - ";
  message += entry.isMandatory ? "mandatory" : "optional";
  message += " entry \"" + attr.name() + "\" " + failedValueMessage + ".";
  std::cout << message << std::endl;
}

void printWrongImageAttributes(const OdimEntry& entry) {
  std::string message = "WARNING -  dataset \"" + entry.node + "\"  is 8-bit unsigned int - " +
                        "it should have attributes CLASS=\"IMAGE\" and IMAGE_VERSION=\"1.2\"";
  std::cout << message << std::endl;
}

void parseAssumedValueStr(const std::string& assumedValueStr,
                          std::vector<std::string>& comparisons,
                          std::vector<std::string>& operators) {
  std::string avs = assumedValueStr;
  size_t pos;
  while ((pos = std::min(avs.find("&&"), avs.find("||"))) != std::string::npos) {
    const std::string comparison = avs.substr(0, pos);
    if ( hasIntervalSigns(comparison) ) {
      comparisons.push_back(comparison);
    }
    operators.push_back(avs.substr(pos,2));
    avs.erase(0, pos + 2);
  }
  if ( hasIntervalSigns(avs) ) {
    comparisons.push_back(avs);
  }
  if ( comparisons.size() != operators.size()+1 ) {
    throw std::runtime_error("ERROR - assumed value string \"" + assumedValueStr +
                             "\" not parsed correctly.");
  }
}

void getArrayStatistics(const std::vector<double>& values, double& first, double& last,
                        double& min, double& max, double& mean) {
  first = values.front();
  last = values.back();
  min = std::numeric_limits<double>::max();
  max = std::numeric_limits<double>::lowest();
  mean = 0.0;
  for (const double v : values) {
    if ( v < min ) min = v;
    if ( v > max ) max = v;
    mean += v;
  }
  mean /= values.size();
}

double valueFromStatistics(std::string& comparison,
                           double first, double last, double min, double max, double mean) {
  size_t pos;
  if ( (pos = comparison.find("first")) != std::string::npos ) {
    comparison.erase(pos, 5);
    return first;
  }
  else if ( (pos = comparison.find("last")) != std::string::npos ) {
    comparison.erase(pos, 4);
    return last;
  }
  else if ( (pos = comparison.find("min")) != std::string::npos ) {
    comparison.erase(pos, 3);
    return min;
  }
  else if ( (pos = comparison.find("max")) != std::string::npos ) {
    comparison.erase(pos, 3);
    return max;
  }
  else if ( (pos = comparison.find("mean")) != std::string::npos ) {
    comparison.erase(pos, 4);
    return mean;
  }
  else {
    throw std::runtime_error("ERROR - proper statistics not found in the \"" +
                             comparison + "\" comparison");
  }
}

void splitPlusMinus(std::string pmString, double& center, double& interval) {
  const std::string delimiter = "+-";
  size_t pos = 0;
  std::string centerStr;
  if ((pos = pmString.find(delimiter)) != std::string::npos) {
    centerStr = pmString.substr(0, pos);
    center = std::stod(centerStr);
    pmString.erase(0, pos + delimiter.length());
    interval = std::stod(pmString);
  }
  else {
    center = std::nan("");
    interval = std::nan("");
  }
}

}
