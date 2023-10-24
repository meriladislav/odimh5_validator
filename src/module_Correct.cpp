// module_Correctcpp
// functions to correct, fix or change the ODIM-H5 files
// Ladislav Meri, SHMU
// v_0.0, 01.2020

#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <cstdint>
#include <regex>
#include <cmath>
#include <hdf5.h>
#include "module_Correct.hpp"
#include "class_H5Layout.hpp"

namespace myodim {

static void checkH5File_(const std::string& h5FilePath);
static hid_t openH5File_(const std::string& h5FilePath, unsigned h5AccessFlag=H5F_ACC_RDONLY);
static void closeH5File_(const hid_t f);
static void saveAsReal64Attribute_(hid_t f, const std::string attrName, const double attrValue);
static void replaceAsReal64Attribute_(hid_t f, const H5Layout& source, const std::string attrName);
static void saveAsReal64ArrayAttribute_(hid_t f, const std::string attrName, const std::vector<double>& attrValue);
static void replaceAsReal64ArrayAttribute_(hid_t f, const H5Layout& source, const std::string attrName);
static void saveAsInt64Attribute_(hid_t f, const std::string attrName, const int64_t attrValue);
static void replaceAsInt64Attribute_(hid_t f, const H5Layout& source, const std::string attrName);
static void saveAsInt64ArrayAttribute_(hid_t f, const std::string attrName, const std::vector<int64_t>& attrValue);
static void replaceAsInt64ArrayAttribute_(hid_t f, const H5Layout& source, const std::string attrName);
static void saveAsFixedLengthStringAttribute_(hid_t f, const std::string attrName, const std::string& attrValue);
static void replaceAsFixedLengthStringAttribute_(hid_t f, const H5Layout& source, const std::string attrName);
static void addGroup_(hid_t f, const std::string& name);
static void splitAttributeToPathAndName_(const std::string& attrName,
                                         std::string& path, std::string& name);
static double parseRealValue_(const std::string& valStr, const std::string attrName);
static std::vector<double> parseRealArrayValue_(std::string valStr, const std::string attrName);
static int64_t parseIntValue_(const std::string& valStr, const std::string attrName);
static std::vector<int64_t> parseIntArrayValue_(std::string valStr, const std::string attrName);
static std::vector<OdimEntry> substituteWildcards_(const H5Layout& h5Layout, const OdimEntry& wildcardEntry);
static OdimStandard substituteWildcards_(const H5Layout& h5Layout, const OdimStandard& wildcardStandard);
static bool hasWildcard_(const std::string& str);
static void splitByWildcard_(const std::string& str, std::string& wildcardPart, std::string& noWildcardPart);
static std::string getMatchingPart_(const std::string str, const std::regex& r);
static void addIfUnique_(std::vector<OdimEntry>& list, const OdimEntry& e);
static void addHowMetadataChanged_(hid_t f, const H5Layout& source, const std::vector<std::string>& metadataChanged);
static bool hasIntervalSigns_(const std::string& assumedValueStr);
static void parseAssumedValueStr_(const std::string& assumedValueStr,
                                 std::vector<std::string>& comparisons,
                                 std::vector<std::string>& operators);
static void splitPlusMinus_(std::string pmString, double& center, double& interval);
static void getSignAndNumberStr_(const std::string& compareStr,
                                 std::string& signStr, std::string& numberStr, bool& hasPlusMinus);
static void parseRealFromInterval_(const std::string& valStr, const std::string attrName, double& realVal);
static void parseIntFromInterval_(const std::string& valStr, const std::string attrName, int64_t& intVal);

void copyFile(const std::string& sourceFile, const std::string& copyFile) {
  FILE* fIn = fopen(sourceFile.c_str(), "rb");
  if ( !fIn ) {
    throw std::runtime_error{"ERROR - file "+sourceFile+" not opened"};
  }
  fseek(fIn, 0L, SEEK_END);
  size_t sz = ftell(fIn);
  rewind(fIn);
  std::vector<char> buffer(sz);
  if ( fread(buffer.data(), sizeof(char), sz, fIn) != sz ) {
    throw std::runtime_error("ERROR - file "+sourceFile+" not loaded");
  }
  fclose(fIn);

  FILE* fOut = fopen(copyFile.c_str(), "wb");
  if ( !fOut ) {
    throw std::runtime_error{"ERROR - file "+copyFile+" not opened"};
  }
  if ( fwrite(buffer.data(), sizeof(char), sz, fOut) != sz ) {
    throw std::runtime_error("ERROR - file "+copyFile+" not saved");
  }
  fclose(fOut);
}

void correct(const std::string& sourceFile, const std::string& targetFile,
             const OdimStandard& toCorrect) {
  checkH5File_(sourceFile);
  copyFile(sourceFile, targetFile);
  H5Layout source(sourceFile);
  OdimStandard toCorrectWithoutWildcards = substituteWildcards_(source, toCorrect);
  std::vector<std::string> metadataChanged;
  auto f = openH5File_(targetFile, H5F_ACC_RDWR);
  for (const auto& entry : toCorrectWithoutWildcards.entries) {
    if ( entry.category == OdimEntry::Category::Attribute ) {

      switch (entry.type) {

        case OdimEntry::Type::Real :
          if ( source.hasAttribute(entry.node) ){
            if ( !source.isReal64Attribute(entry.node) ) {        //!!! TODO - what if type and also value changes
              replaceAsReal64Attribute_(f, source, entry.node);
              metadataChanged.push_back(entry.node);
            }
            else {
              if ( !entry.possibleValues.empty() ) {
                saveAsReal64Attribute_(f, entry.node, parseRealValue_(entry.possibleValues, entry.node));
                metadataChanged.push_back(entry.node);
              }
            }
          }
          else {
            saveAsReal64Attribute_(f, entry.node, parseRealValue_(entry.possibleValues, entry.node));
            metadataChanged.push_back(entry.node);
          }
          break;

        case OdimEntry::Type::Integer :
          if ( source.hasAttribute(entry.node) ){
            if ( !source.isInt64Attribute(entry.node) ) {
              replaceAsInt64Attribute_(f, source, entry.node);
              metadataChanged.push_back(entry.node);
            }
            else {
              if ( !entry.possibleValues.empty() ) {
                saveAsInt64Attribute_(f, entry.node, parseIntValue_(entry.possibleValues, entry.node));
                metadataChanged.push_back(entry.node);
              }
            }
          }
          else {
            saveAsInt64Attribute_(f, entry.node, parseIntValue_(entry.possibleValues, entry.node));
            metadataChanged.push_back(entry.node);
          }
          break;

        case OdimEntry::Type::String :
          if ( source.hasAttribute(entry.node) ){
            if ( !source.isFixedLengthStringAttribute(entry.node) ) {
              replaceAsFixedLengthStringAttribute_(f, source, entry.node);
              metadataChanged.push_back(entry.node);
            }
            else {
              if ( !entry.possibleValues.empty() ) {
                saveAsFixedLengthStringAttribute_(f, entry.node, entry.possibleValues);
                metadataChanged.push_back(entry.node);
              }
            }
          }
          else {
            saveAsFixedLengthStringAttribute_(f, entry.node, entry.possibleValues);
            metadataChanged.push_back(entry.node);
          }
          break;

        case OdimEntry::Type::RealArray :
          if ( source.hasAttribute(entry.node) ){
            if ( !source.isReal64Attribute(entry.node) || !source.is1DArrayAttribute(entry.node) ) {
              replaceAsReal64ArrayAttribute_(f, source, entry.node);
              metadataChanged.push_back(entry.node);
            }
            else {
              if ( !entry.possibleValues.empty() ) {
                saveAsReal64ArrayAttribute_(f, entry.node, parseRealArrayValue_(entry.possibleValues, entry.node));
                metadataChanged.push_back(entry.node);
              }
            }
          }
          else {
            saveAsReal64ArrayAttribute_(f, entry.node, parseRealArrayValue_(entry.possibleValues, entry.node));
            metadataChanged.push_back(entry.node);
          }
          break;

        case OdimEntry::Type::IntegerArray :
          if ( source.hasAttribute(entry.node) ){
            if ( !source.isInt64Attribute(entry.node) || !source.is1DArrayAttribute(entry.node) ) {
              replaceAsInt64ArrayAttribute_(f, source, entry.node);
              metadataChanged.push_back(entry.node);
            }
            else {
              if ( !entry.possibleValues.empty() ) {
                saveAsInt64ArrayAttribute_(f, entry.node, parseIntArrayValue_(entry.possibleValues, entry.node));
                metadataChanged.push_back(entry.node);
              }
            }
          }
          else {
            saveAsInt64ArrayAttribute_(f, entry.node, parseIntArrayValue_(entry.possibleValues, entry.node));
            metadataChanged.push_back(entry.node);
          }
          break;

        default :
          std::cout << "WARNING - the attribute data type is " << entry.typeToString() << std::endl;
          throw std::runtime_error("ERROR - only Real, Integer or String type attribute correction is implemented yet");
          break;
      }
    }
    else if ( entry.category == OdimEntry::Category::Group ) {
      addGroup_(f, entry.node);
      metadataChanged.push_back(entry.node);
    }
    else {
      throw std::runtime_error("ERROR - dataset correction not implemented yet");
    }
  }

  addHowMetadataChanged_(f, source, metadataChanged);

  closeH5File_(f);
}


//statics

void checkH5File_(const std::string& h5FilePath) {
  if ( H5Fis_hdf5(h5FilePath.c_str()) <= 0 ) {
    throw std::runtime_error{"ERROR - file "+h5FilePath+" is not a HDF5 file"};
  }
}

hid_t openH5File_(const std::string& h5FilePath, unsigned h5AccessFlag) {
  hid_t f = H5Fopen(h5FilePath.c_str(), h5AccessFlag, H5P_DEFAULT);
  if ( f < 0 ) {
    throw std::runtime_error{"ERROR - file "+h5FilePath+" not opened"};
  }
  return f;
}

void closeH5File_(const hid_t f) {
  H5Fclose(f);
}

void saveAsReal64Attribute_(hid_t f, const std::string attrName, const double attrValue) {
  std::string path, name;
  splitAttributeToPathAndName_(attrName, path, name);

  auto parent = H5Oopen(f, path.c_str(), H5P_DEFAULT);
  if ( parent < 0  ) {
    throw std::runtime_error("ERROR - node "+path+" not opened");
  }

  H5Adelete(parent, name.c_str());

  hid_t sp = H5Screate(H5S_SCALAR);
  auto a = H5Acreate2(parent, name.c_str(), H5T_NATIVE_DOUBLE, sp, H5P_DEFAULT, H5P_DEFAULT);
  if ( a < 0 ) {
    throw std::runtime_error("ERROR - attribute "+name+" not opened.");
  }

  auto ret  = H5Awrite(a, H5T_NATIVE_DOUBLE, &attrValue);
  if ( ret < 0  ) {
    H5Aclose(a);
    H5Oclose(parent);
    throw std::runtime_error("ERROR - attribute "+attrName+" not written");
  }
  H5Sclose(sp);
  H5Aclose(a);
  H5Oclose(parent);
}

void saveAsReal64ArrayAttribute_(hid_t f, const std::string attrName, const std::vector<double>& attrValue) {
  std::string path, name;
  splitAttributeToPathAndName_(attrName, path, name);

  auto parent = H5Oopen(f, path.c_str(), H5P_DEFAULT);
  if ( parent < 0  ) {
    throw std::runtime_error("ERROR - node "+path+" not opened");
  }

  H5Adelete(parent, name.c_str());

  const hsize_t dims[1] = {attrValue.size()};
  hid_t sp = H5Screate_simple (1, dims, NULL);
  auto a = H5Acreate2(parent, name.c_str(), H5T_NATIVE_DOUBLE, sp, H5P_DEFAULT, H5P_DEFAULT);
  if ( a < 0 ) {
    throw std::runtime_error("ERROR - attribute "+name+" not opened.");
  }

  auto ret  = H5Awrite(a, H5T_NATIVE_DOUBLE, attrValue.data());
  if ( ret < 0  ) {
    H5Aclose(a);
    H5Oclose(parent);
    throw std::runtime_error("ERROR - attribute "+attrName+" not written");
  }

  H5Sclose(sp);
  H5Aclose(a);
  H5Oclose(parent);
}

void replaceAsReal64Attribute_(hid_t f, const H5Layout& source, const std::string attrName) {
  double attrValue;
  source.getAttributeValue(attrName, attrValue);
  saveAsReal64Attribute_(f, attrName, attrValue);
}

void replaceAsReal64ArrayAttribute_(hid_t f, const H5Layout& source, const std::string attrName) {
  std::vector<double> attrValue;
  source.getAttributeValue(attrName, attrValue);
  saveAsReal64ArrayAttribute_(f, attrName, attrValue);
}

void saveAsInt64Attribute_(hid_t f, const std::string attrName, const int64_t attrValue) {
  std::string path, name;
  splitAttributeToPathAndName_(attrName, path, name);

  auto parent = H5Oopen(f, path.c_str(), H5P_DEFAULT);
  if ( parent < 0  ) {
    throw std::runtime_error("ERROR - node "+path+" not opened");
  }

  H5Adelete(parent, name.c_str());

  hid_t sp = H5Screate(H5S_SCALAR);
  auto a = H5Acreate2(parent, name.c_str(), H5T_NATIVE_INT64, sp, H5P_DEFAULT, H5P_DEFAULT);
  if ( a < 0 ) {
    throw std::runtime_error("ERROR - attribute "+name+" not opened.");
  }

  auto ret  = H5Awrite(a, H5T_NATIVE_INT64, &attrValue);
  if ( ret < 0  ) {
    H5Aclose(a);
    H5Oclose(parent);
    throw std::runtime_error("ERROR - attribute "+attrName+" not written");
  }

  H5Sclose(sp);
  H5Aclose(a);
  H5Oclose(parent);
}

void saveAsInt64ArrayAttribute_(hid_t f, const std::string attrName, const std::vector<int64_t>& attrValue) {
  std::string path, name;
  splitAttributeToPathAndName_(attrName, path, name);

  auto parent = H5Oopen(f, path.c_str(), H5P_DEFAULT);
  if ( parent < 0  ) {
    throw std::runtime_error("ERROR - node "+path+" not opened");
  }

  H5Adelete(parent, name.c_str());

  const hsize_t dims[1] = {attrValue.size()};
  hid_t sp = H5Screate_simple (1, dims, NULL);
  auto a = H5Acreate2(parent, name.c_str(), H5T_NATIVE_INT64, sp, H5P_DEFAULT, H5P_DEFAULT);
  if ( a < 0 ) {
    throw std::runtime_error("ERROR - attribute "+name+" not opened.");
  }

  auto ret  = H5Awrite(a, H5T_NATIVE_INT64, attrValue.data());
  if ( ret < 0  ) {
    H5Aclose(a);
    H5Oclose(parent);
    throw std::runtime_error("ERROR - attribute "+attrName+" not written");
  }
  H5Sclose(sp);
  H5Aclose(a);
  H5Oclose(parent);
}


void replaceAsInt64Attribute_(hid_t f, const H5Layout& source, const std::string attrName) {
  int64_t attrValue;
  source.getAttributeValue(attrName, attrValue);
  saveAsInt64Attribute_(f, attrName, attrValue);
}

void replaceAsInt64ArrayAttribute_(hid_t f, const H5Layout& source, const std::string attrName) {
  std::vector<int64_t> attrValue;
  source.getAttributeValue(attrName, attrValue);
  saveAsInt64ArrayAttribute_(f, attrName, attrValue);
}

void saveAsFixedLengthStringAttribute_(hid_t f, const std::string attrName, const std::string& attrValue) {
  std::string path, name;
  splitAttributeToPathAndName_(attrName, path, name);

  auto parent = H5Oopen(f, path.c_str(), H5P_DEFAULT);
  if ( parent < 0  ) {
    throw std::runtime_error("ERROR - node "+path+" not opened");
  }

  H5Adelete(parent, name.c_str());

  auto sp  = H5Screate(H5S_SCALAR);
  auto t = H5Tcopy(H5T_C_S1);
  H5Tset_size(t, attrValue.length()+1);
  H5Tset_strpad(t, H5T_STR_NULLTERM);
  auto a = H5Acreate2(parent, name.c_str(), t, sp, H5P_DEFAULT, H5P_DEFAULT);
  if ( a < 0 ) {
    H5Sclose(sp);
    H5Tclose(t);
    H5Oclose(parent);
    throw std::runtime_error("ERROR - attribute "+name+" not opened.");
  }

  auto ret  = H5Awrite(a, t, attrValue.c_str());
  if ( ret < 0  ) {
    H5Sclose(sp);
    H5Tclose(t);
    H5Aclose(a);
    H5Oclose(parent);
    throw std::runtime_error("ERROR - attribute "+attrName+" not written");
  }

  H5Sclose(sp);
  H5Tclose(t);
  H5Aclose(a);
  H5Oclose(parent);
}

void replaceAsFixedLengthStringAttribute_(hid_t f, const H5Layout& source, const std::string attrName) {
  std::string attrValue;
  source.getAttributeValue(attrName, attrValue);
  saveAsFixedLengthStringAttribute_(f, attrName, attrValue);
}

void addGroup_(hid_t f, const std::string& name) {
  auto g = H5Oopen(f, name.c_str(), H5P_DEFAULT);
  if ( g < 0 ) {
    g = H5Gcreate(f, name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if ( g < 0 ) {
      throw std::runtime_error("ERROR - group "+name+" not created");
    }
  }
  H5Oclose(g);
  return;
}

void splitAttributeToPathAndName_(const std::string& attrName,
                                  std::string& path, std::string& name) {
  auto found = attrName.find_last_of('/');
  path = attrName.substr(0,found+1);
  name = attrName.substr(found+1);
}

double parseRealValue_(const std::string& valStr, const std::string attrName) {
  double d = std::nan("");
  if ( hasIntervalSigns_(valStr) ) {
    parseRealFromInterval_(valStr, attrName, d);
  }
  else {
    try {
      d = std::stod(valStr);
   }
    catch(...) {
      throw std::invalid_argument("ERROR - the value of "+attrName+" not parsed correctly");
    }
  }

  return d;
}

std::vector<double> parseRealArrayValue_(std::string valStr, const std::string attrName) {
  std::vector<double> result;
  try {
    size_t pos = 0;
    std::string token;
    while ((pos = valStr.find(",")) != std::string::npos) {
      token = valStr.substr(0, pos);
      result.push_back(std::stod(token));
      valStr.erase(0, pos + 1);
    }
    result.push_back(std::stod(valStr));
  }
  catch(...) {
    throw std::invalid_argument("ERROR - the value of "+attrName+" not parsed correctly");
  }
  return result;
}

int64_t parseIntValue_(const std::string& valStr, const std::string attrName) {
  int64_t i;
  if ( hasIntervalSigns_(valStr) ) {
    parseIntFromInterval_(valStr, attrName, i);
  }
  else {
    try {
      i = std::stoi(valStr);
    }
    catch(...) {
      throw std::invalid_argument("ERROR - the value of "+attrName+" not parsed correctly");
    }
  }
  return i;
}

std::vector<int64_t> parseIntArrayValue_(std::string valStr, const std::string attrName) {
  std::vector<int64_t> result;
  try {
    size_t pos = 0;
    std::string token;
    while ((pos = valStr.find(",")) != std::string::npos) {
      token = valStr.substr(0, pos);
      result.push_back(std::stod(token));
      valStr.erase(0, pos + 1);
    }
    result.push_back(std::stoi(valStr));
  }
  catch(...) {
    throw std::invalid_argument("ERROR - the value of "+attrName+" not parsed correctly");
  }
  return result;
}

std::vector<OdimEntry> substituteWildcards_(const H5Layout& h5Layout, const OdimEntry& wildcardEntry) {
  std::vector<OdimEntry> resultEntries;

  if ( hasWildcard_(wildcardEntry.node) ) {
    std::string wc, other;
    splitByWildcard_(wildcardEntry.node, wc, other);
    std::regex wildcardRegex{wc};

    if ( wildcardEntry.category == OdimEntry::Category::Group ) {
      for (const auto& g : h5Layout.groups) {
        if ( std::regex_match(g.name(), wildcardRegex)  ) {
          OdimEntry e = wildcardEntry;
          std::string matchingPart = getMatchingPart_(g.name(), wildcardRegex);
          if ( matchingPart.empty() ) continue;
          e.node = matchingPart+other;
          addIfUnique_(resultEntries, e);
        }
      }
    }
    else if ( wildcardEntry.category == OdimEntry::Category::Attribute ) {
      for (const auto& a : h5Layout.attributes) {
        if ( std::regex_match(a.name(), wildcardRegex)  ) {
          OdimEntry e = wildcardEntry;
          std::string matchingPart = getMatchingPart_(a.name(), wildcardRegex);
          if ( matchingPart.empty() ) continue;
          e.node = matchingPart+other;
          addIfUnique_(resultEntries, e);
        }
      }
    }
    else {
      for (const auto& d : h5Layout.datasets) {
        if ( std::regex_match(d.name(), wildcardRegex)  ) {
          OdimEntry e = wildcardEntry;
          std::string matchingPart = getMatchingPart_(d.name(), wildcardRegex);
          if ( matchingPart.empty() ) continue;
          e.node = matchingPart+other;
          addIfUnique_(resultEntries, e);
        }
      }
    }
  }
  else {
    resultEntries.push_back(wildcardEntry);
  }

  return resultEntries;
}

OdimStandard substituteWildcards_(const H5Layout& h5Layout, const OdimStandard& wildcardStandard) {
  OdimStandard result;
  for (const auto& e : wildcardStandard.entries) {
    std::vector<OdimEntry> entries = substituteWildcards_(h5Layout, e);
    for (const auto& ee : entries) {
      result.entries.push_back(ee);
    }
  }
  return result;
}

bool hasWildcard_(const std::string& str) {
  return str.find('*') != std::string::npos ||
         str.find('[') != std::string::npos ||
         str.find('?') != std::string::npos ;
}

void splitByWildcard_(const std::string& str, std::string& wildcardPart, std::string& noWildcardPart) {
  if ( !hasWildcard_(str) ) {
    wildcardPart = str;
    noWildcardPart = "";
    return;
  }

  std::vector<size_t> tmp = {str.find_last_of("*"),
                             str.find_last_of("["),
                             str.find_last_of("]"),
                             str.find_last_of("?")};

  size_t lastWCPosi = 0;
  for (int i=0; i<4; ++i) {
    if ( tmp[i] == std::string::npos ) continue;
    if ( tmp[i] > lastWCPosi ) lastWCPosi = tmp[i];
  }
  std::string wrkStr = str.substr(lastWCPosi);
  auto p = wrkStr.find("/");
  wildcardPart = str.substr(0,lastWCPosi+p);
  noWildcardPart = str.substr(lastWCPosi+p);
}

std::string getMatchingPart_(const std::string str, const std::regex& r) {
  std::string tmp = "";
  auto strlen = str.length();
  size_t i = 0;
  while ( !std::regex_match(tmp, r) ) {
    i++;
    if ( i >= strlen ) break;
    tmp = str.substr(0, i);
  }
  auto p = str.substr(i).find("/");
  if ( p == std::string::npos ) {
    i = 0;
    p = 0;
  }
  return str.substr(0,i+p);
}

void addIfUnique_(std::vector<OdimEntry>& list, const OdimEntry& e) {
  for (const auto& element : list) {
    if ( element.node == e.node ) return;
  }
  list.push_back(e);
}

void addHowMetadataChanged_(hid_t f, const H5Layout& source,
                            const std::vector<std::string>& metadataChanged) {
  if ( metadataChanged.size() == 0 ) return;
  std::string metaBefore = "";
  if ( source.hasAttribute("/how/metadata_changed") ) {
    source.getAttributeValue("/how/metadata_changed", metaBefore);
  }

  addGroup_(f, "/how");
  std::string value = metaBefore;
  const int n = metadataChanged.size();
  for (int i=0; i<n; ++i) {
    if ( value.find(metadataChanged[i]) == std::string::npos ) {
      if ( !value.empty() && value.back() != ',' ) value += ",";
      value += metadataChanged[i]+",";
    }
  }
  if ( !value.empty() ) value.pop_back(); //remove last ','
  saveAsFixedLengthStringAttribute_(f, "/how/metadata_changed", value);
}

bool hasIntervalSigns_(const std::string& assumedValueStr) {
  return assumedValueStr.find('=') != std::string::npos ||
         assumedValueStr.find('<') != std::string::npos ||
         assumedValueStr.find('>') != std::string::npos ||
         assumedValueStr.find("+-") != std::string::npos;
}

void parseAssumedValueStr_(const std::string& assumedValueStr,
                          std::vector<std::string>& comparisons,
                          std::vector<std::string>& operators) {
  std::string avs = assumedValueStr;
  size_t pos;
  while ((pos = std::min(avs.find("&&"), avs.find("||"))) != std::string::npos) {
    const std::string comparison = avs.substr(0, pos);
    if ( hasIntervalSigns_(comparison) ) {
      comparisons.push_back(comparison);
    }
    operators.push_back(avs.substr(pos,2));
    avs.erase(0, pos + 2);
  }
  if ( hasIntervalSigns_(avs) ) {
    comparisons.push_back(avs);
  }
  if ( comparisons.size() != operators.size()+1 ) {
    throw std::runtime_error("ERROR - assumed value string \"" + assumedValueStr +
                             "\" not parsed correctly.");
  }
}

void splitPlusMinus_(std::string pmString, double& center, double& interval) {
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

void getSignAndNumberStr_(const std::string& compareStr, std::string& signStr, std::string& numberStr,
                          bool& hasPlusMinus) {
  hasPlusMinus = false;
  for (int i=0; i<(int)compareStr.length(); ++i) {
    if ( (compareStr[i] >= '0' && compareStr[i] <= '9') || compareStr[i] <= '.' ) {
      numberStr.append(1, compareStr[i]);
      if ( compareStr[i] == '-' && i > 0 && compareStr[i-1] == '+') {
        hasPlusMinus = true;
      }
    }
    else {
      signStr.append(1, compareStr[i]);
    }
  }
}

void parseRealFromInterval_(const std::string& valStr, const std::string attrName, double& realVal) {
  realVal = std::nan("");
  std::vector<std::string> comparisons;
  std::vector<std::string> operators;
  parseAssumedValueStr_(valStr, comparisons, operators);
  if ( comparisons.size() == 1 ) {
    std::string signStr, numberStr;
    bool hasPlusMinus = false;
    getSignAndNumberStr_(comparisons[0], signStr, numberStr, hasPlusMinus);
    if ( hasPlusMinus ) {
      double center, interval;
      splitPlusMinus_(numberStr, center, interval);
      realVal = center;
    }
    else {
      throw std::invalid_argument("ERROR - the value of "+attrName+" - defined as "+valStr+" - not parsed correctly - it is an open interval");
    }
  }
  else if ( comparisons.size() == 2 ) {
    if ( operators[0] != "&&" ) {
      throw std::invalid_argument("ERROR - the value of "+attrName+" - defined as "+valStr+" - not parsed correctly ");
    }
    std::vector<std::string> signStr(2), numberStr(2);
    bool hasPlusMinus[2];
    for (int i=0; i<2; ++i) {
      getSignAndNumberStr_(comparisons[i], signStr[i], numberStr[i], hasPlusMinus[i]);
    }
    if ( (signStr[0].find('<') != std::string::npos && signStr[1].find('>') != std::string::npos) ||
         (signStr[0].find('>') != std::string::npos && signStr[1].find('<') != std::string::npos) ) {
      double d1 = numberStr[0].empty() ? std::nan("") : std::stod(numberStr[0]);
      double d2 = numberStr[1].empty() ? std::nan("") : std::stod(numberStr[1]);
      realVal = (d1 + d2) / 2;
    }
    else {
      throw std::invalid_argument("ERROR - the value of "+attrName+" - defined as "+valStr+" - not parsed correctly - it is an open interval");
    }
  }
  else {
    throw std::invalid_argument("ERROR - the value of "+attrName+" - defined as "+valStr+" - not parsed correctly ");
  }
}

void parseIntFromInterval_(const std::string& valStr, const std::string attrName, int64_t& intVal) {
  intVal = 0;
  std::vector<std::string> comparisons;
  std::vector<std::string> operators;
  parseAssumedValueStr_(valStr, comparisons, operators);
  if ( comparisons.size() == 1 ) {
    std::string signStr, numberStr;
    bool hasPlusMinus = false;
    getSignAndNumberStr_(comparisons[0], signStr, numberStr, hasPlusMinus);
    if ( hasPlusMinus ) {
      double center, interval;
      splitPlusMinus_(numberStr, center, interval);
      intVal = center;
    }
    else {
      throw std::invalid_argument("ERROR - the value of "+attrName+" - defined as "+valStr+" - not parsed correctly - it is an open interval");
    }
  }
  else if ( comparisons.size() == 2 ) {
    if ( operators[0] != "&&" ) {
      throw std::invalid_argument("ERROR - the value of "+attrName+" - defined as "+valStr+" - not parsed correctly ");
    }
    std::vector<std::string> signStr(2), numberStr(2);
    bool hasPlusMinus[2];
    for (int i=0; i<2; ++i) {
      getSignAndNumberStr_(comparisons[i], signStr[i], numberStr[i], hasPlusMinus[i]);
    }
    if ( (signStr[0].find('<') != std::string::npos && signStr[1].find('>') != std::string::npos) ||
         (signStr[0].find('>') != std::string::npos && signStr[1].find('<') != std::string::npos) ) {
      double d1 = numberStr[0].empty() ? std::nan("") : std::stod(numberStr[0]);
      double d2 = numberStr[1].empty() ? std::nan("") : std::stod(numberStr[1]);
      intVal = (d1 + d2) / 2.0;
    }
    else {
      throw std::invalid_argument("ERROR - the value of "+attrName+" - defined as "+valStr+" - not parsed correctly - it is an open interval");
    }
  }
  else {
    throw std::invalid_argument("ERROR - the value of "+attrName+" - defined as "+valStr+" - not parsed correctly ");
  }
}

} //end namespace myodim
