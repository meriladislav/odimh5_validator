// module_Correctcpp
// functions to correct, fix or change the ODIM-H5 files
// Ladislav Meri, SHMU
// v_0.0, 01.2020

#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <cstdint>
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
static void splitAttributeToPathAndName_(const std::string& attrName,
                                         std::string& path, std::string& name);

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

  auto f = openH5File_(targetFile, H5F_ACC_RDWR);

  for (const auto& entry : toCorrect.entries) {
    if ( entry.category == OdimEntry::Category::Attribute ) {

      switch (entry.type) {
        case OdimEntry::Type::Real :
          if ( source.hasAttribute(entry.node) ){
            if ( !source.isReal64Attribute(entry.node) ) {
              replaceAsReal64Attribute_(f, source, entry.node);
            }
          }
          else {
            saveAsReal64Attribute_(f, entry.node, std::stod(entry.possibleValues));
          }
          break;

        case OdimEntry::Type::Integer :
          if ( source.hasAttribute(entry.node) ){
            if ( !source.isInt64Attribute(entry.node) ) {
              replaceAsInt64Attribute_(f, source, entry.node);
            }
          }
          else {
            saveAsInt64Attribute_(f, entry.node, std::stoi(entry.possibleValues));
          }
          break;

        case OdimEntry::Type::String :
          if ( source.hasAttribute(entry.node) ){
            if ( !source.isFixedLenghtStringAttribute(entry.node) ) {
              replaceAsFixedLenghtStringAttribute_(f, source, entry.node);
            }
          }
          else {
            saveAsFixedLenghtStringAttribute_(f, entry.node, entry.possibleValues);
          }
          break;

        default :
          std::cout << "WARNING - the attribute data type is " << entry.typeToString() << std::endl;
          throw std::runtime_error("ERROR - only Real, Integer or String type attribute correction is implemented yet");
          break;
      }
    }
    else {
     throw std::runtime_error("ERROR - only Attribute correction implemented yet");
    }
  }

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
  auto a = H5Acreate2(parent, name.c_str(), H5T_STD_I64LE, sp, H5P_DEFAULT, H5P_DEFAULT);
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
  auto a = H5Acreate2(parent, attrName.c_str(), t, sp, H5P_DEFAULT, H5P_DEFAULT);
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

void splitAttributeToPathAndName_(const std::string& attrName,
                                  std::string& path, std::string& name) {
  auto found = attrName.find_last_of('/');
  path = attrName.substr(0,found+1);
  name = attrName.substr(found+1);
}


} //end namespace myodim
