// module_Correctcpp
// functions to correct, fix or change the ODIM-H5 files
// Ladislav Meri, SHMU
// v_0.0, 01.2020

#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <cstdint>
#include <regex>
#include <hdf5.h>
#include "module_Correct.hpp"
#include "class_H5Layout.hpp"

namespace myodim {

static void checkH5File_(const std::string& h5FilePath);
static hid_t openH5File_(const std::string& h5FilePath, unsigned h5AccessFlag=H5F_ACC_RDONLY);
static void closeH5File_(const hid_t f);
static void saveAsReal64Attribute_(hid_t f, const std::string attrName, const double attrValue);
static void replaceAsReal64Attribute_(hid_t f, const H5Layout& source, const std::string attrName);
static void saveAsInt64Attribute_(hid_t f, const std::string attrName, const int64_t attrValue);
static void replaceAsInt64Attribute_(hid_t f, const H5Layout& source, const std::string attrName);
static void saveAsFixedLenghtStringAttribute_(hid_t f, const std::string attrName, const std::string& attrValue);
static void replaceAsFixedLenghtStringAttribute_(hid_t f, const H5Layout& source, const std::string attrName);
static void addGroup_(hid_t f, const std::string& name);
static void splitAttributeToPathAndName_(const std::string& attrName,
                                         std::string& path, std::string& name);
static double parseRealValue_(const std::string& valStr, const std::string attrName);
static int64_t parseIntValue_(const std::string& valStr, const std::string attrName);
static std::vector<OdimEntry> subsituteWildcards_(const H5Layout& h5Layout, const OdimEntry& wildcardEntry);
static OdimStandard subsituteWildcards_(const H5Layout& h5Layout, const OdimStandard& wildcardStandard);
static bool hasWildcard_(const std::string& str);
static void splitByWildcard_(const std::string& str, std::string& wildcardPart, std::string& noWildcardPart);
static std::string getMatchingPart_(const std::string str, const std::regex& r);
static void addIfUnique_(std::vector<OdimEntry>& list, const OdimEntry& e);
static void addHowMetadataChanged_(hid_t f, const std::vector<std::string>& metadataChanged);

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

  OdimStandard toCorrectWithoutWildcards = subsituteWildcards_(source, toCorrect);

  std::vector<std::string> metadataChanged;

  auto f = openH5File_(targetFile, H5F_ACC_RDWR);

  for (const auto& entry : toCorrectWithoutWildcards.entries) {
    if ( entry.category == OdimEntry::Category::Attribute ) {

      switch (entry.type) {

        case OdimEntry::Type::Real :
          if ( source.hasAttribute(entry.node) ){
            if ( !source.isReal64Attribute(entry.node) ) {
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
            if ( !source.isFixedLenghtStringAttribute(entry.node) ) {
              replaceAsFixedLenghtStringAttribute_(f, source, entry.node);
              metadataChanged.push_back(entry.node);
            }
            else {
              if ( !entry.possibleValues.empty() ) {
                saveAsFixedLenghtStringAttribute_(f, entry.node, entry.possibleValues);
                metadataChanged.push_back(entry.node);
              }
            }
          }
          else {
            saveAsFixedLenghtStringAttribute_(f, entry.node, entry.possibleValues);
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

  addHowMetadataChanged_(f, metadataChanged);

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

  H5Aclose(a);
  H5Oclose(parent);
}

void replaceAsReal64Attribute_(hid_t f, const H5Layout& source, const std::string attrName) {
  double attrValue;
  source.getAttributeValue(attrName, attrValue);
  saveAsReal64Attribute_(f, attrName, attrValue);
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

  H5Aclose(a);
  H5Oclose(parent);
}

void replaceAsInt64Attribute_(hid_t f, const H5Layout& source, const std::string attrName) {
  int64_t attrValue;
  source.getAttributeValue(attrName, attrValue);
  saveAsInt64Attribute_(f, attrName, attrValue);
}

void saveAsFixedLenghtStringAttribute_(hid_t f, const std::string attrName, const std::string& attrValue) {
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

void replaceAsFixedLenghtStringAttribute_(hid_t f, const H5Layout& source, const std::string attrName) {
  std::string attrValue;
  source.getAttributeValue(attrName, attrValue);
  saveAsFixedLenghtStringAttribute_(f, attrName, attrValue);
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
  double d;
  try {
    d = std::stod(valStr);
  }
  catch(...) {
    throw std::invalid_argument("ERROR - the value of "+attrName+" not parsed correctly");
  }
  return d;
}

int64_t parseIntValue_(const std::string& valStr, const std::string attrName) {
  int64_t i;
  try {
    i = std::stoi(valStr);
  }
  catch(...) {
    throw std::invalid_argument("ERROR - the value of "+attrName+" not parsed correctly");
  }
  return i;
}

std::vector<OdimEntry> subsituteWildcards_(const H5Layout& h5Layout, const OdimEntry& wildcardEntry) {
  std::vector<OdimEntry> resultEntries;

  if ( hasWildcard_(wildcardEntry.node) ) {
    std::string wc, other;
    splitByWildcard_(wildcardEntry.node, wc, other);
    std::regex wildcardRegex{wc};
    //std::cout << "dbg - 2 - " << wc << std::endl;

    if ( wildcardEntry.category == OdimEntry::Category::Group ) {
      for (const auto& g : h5Layout.groups) {
        if ( std::regex_match(g.name(), wildcardRegex)  ) {
          OdimEntry e = wildcardEntry;
          //std::cout << "dbg - getmatchingpart = " << getMatchingPart_(g.name(), wildcardRegex) << std::endl;
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

OdimStandard subsituteWildcards_(const H5Layout& h5Layout, const OdimStandard& wildcardStandard) {
  OdimStandard result;
  for (const auto& e : wildcardStandard.entries) {
    //std::cout << "dbg - 4 - " << e.node << std::endl;
    std::vector<OdimEntry> entries = subsituteWildcards_(h5Layout, e);
    for (const auto& ee : entries) {
      //std::cout << "dbg - 3 - " << ee.node << std::endl;
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

void addHowMetadataChanged_(hid_t f, const std::vector<std::string>& metadataChanged) {
  addGroup_(f, "/how");
  std::string value = "";
  const int n = metadataChanged.size();
  for (int i=0; i<n-1; ++i) {
    value += metadataChanged[i]+",";
  }
  value += metadataChanged[n-1];
  saveAsFixedLenghtStringAttribute_(f, "/how/metadata_changed", value);
}

} //end namespace myodim
