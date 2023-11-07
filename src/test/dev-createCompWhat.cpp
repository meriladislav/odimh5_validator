/*
 program to create a v2.4 file with a2D attribute to use it in the testing for the 2d attribute checks
*/

#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <hdf5.h>
#include "../module_Compare.hpp"

using namespace myodim;

const std::string inFile = "./data/test/T_PASH21_C_EUOC_20230616041500.hdf";

static void saveAsFixedLenghtStringAttribute_(hid_t f, const std::string attrName, const std::string& attrValue);
static void deleteAttribute_(hid_t f, const std::string attrName);
static void splitAttributeToPathAndName_(const std::string& attrName, std::string& path, std::string& name);


int main() {

  //H5Eset_auto( H5E_DEFAULT, NULL, NULL ); //Turn off error handling permanently

  hid_t f = H5Fopen(inFile.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
  if ( f < 0 ) {
    throw std::runtime_error{"ERROR - file "+inFile+" not opened"};
  }

  deleteAttribute_(f, "/dataset1/what/quantity");
  saveAsFixedLenghtStringAttribute_(f, "/dataset1/data1/what/quantity", "ACRR");

  H5Fclose(f);

  //test the regex
  const std::string value = "/dataset1/what/quantity";
  const std::string regex = "/dataset[1-9][0-9]*(/data[1-9][0-9]*)?/what/quantity";
  std::string errmsg;
  std::cout << "DBG - regex test : " << checkValue(value, regex, errmsg) << std::endl;

  return 0;
}





void splitAttributeToPathAndName_(const std::string& attrName,
                                  std::string& path, std::string& name) {
  auto found = attrName.find_last_of('/');
  path = attrName.substr(0,found+1);
  name = attrName.substr(found+1);
}

void deleteAttribute_(hid_t f, const std::string attrName) {
  std::string path, name;
  splitAttributeToPathAndName_(attrName, path, name);

  auto parent = H5Oopen(f, path.c_str(), H5P_DEFAULT);
  if ( parent < 0  ) {
    throw std::runtime_error("ERROR - node "+path+" not opened");
  }
  
  if ( H5Aexists(parent, name.c_str()) ) {
    H5Adelete(parent, name.c_str());
  }
  
  H5Oclose(parent);
}

void saveAsFixedLenghtStringAttribute_(hid_t f, const std::string attrName, const std::string& attrValue) {
  std::string path, name;
  splitAttributeToPathAndName_(attrName, path, name);

  auto parent = H5Oopen(f, path.c_str(), H5P_DEFAULT);
  if ( parent < 0  ) {
    parent = H5Gcreate(f, path.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if ( parent < 0  ) {
      throw std::runtime_error("ERROR - node "+path+" not opened");
    }
  }
  if ( H5Aexists(parent, name.c_str()) ) {
    H5Adelete(parent, name.c_str());
  }

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

