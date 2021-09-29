/*
 program to create a v2.4 file with a2D attribute to use it in the testing for the 2d attribute checks
*/

#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <hdf5.h>

const std::string inFile = "./data/test/T_PAZE50_C_LFPW_20190426132340.h5";


static void saveAsFixedLenghtStringAttribute_(hid_t f, const std::string attrName, const std::string& attrValue);
static void saveAsReal64Attribute_(hid_t f, const std::string attrName, const double attrValue);
static void saveAsInt64Attribute_(hid_t f, const std::string attrName, const int64_t attrValue);
static void splitAttributeToPathAndName_(const std::string& attrName, std::string& path, std::string& name);


int main() {

  //H5Eset_auto( H5E_DEFAULT, NULL, NULL ); //Turn off error handling permanently

  hid_t f = H5Fopen(inFile.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
  if ( f < 0 ) {
    throw std::runtime_error{"ERROR - file "+inFile+" not opened"};
  }

  //overwrite "/Conventions" and "/what/version"
  saveAsFixedLenghtStringAttribute_(f, "/Conventions", "ODIM_H5/V2_4");
  saveAsFixedLenghtStringAttribute_(f, "/what/version", "H5rad 2.4");

  //add missing mandatory attributes
  saveAsReal64Attribute_(f, "/how/frequency", 5656461.4717);
  saveAsReal64Attribute_(f, "/how/RXlossH", 2.591);
  saveAsReal64Attribute_(f, "/how/RXlossV", 2.591);
  saveAsReal64Attribute_(f, "/how/antgainH", 45.0);
  saveAsReal64Attribute_(f, "/how/antgainV", 45.0);
  saveAsReal64Attribute_(f, "/how/beamwH", 0.92);
  saveAsReal64Attribute_(f, "/how/beamwV", 0.92);
  saveAsInt64Attribute_(f, "/how/scan_count", 1);
  saveAsReal64Attribute_(f, "/how/pulsewidth", 0.000002);
  saveAsFixedLenghtStringAttribute_(f, "/how/simulated", "False");


  saveAsInt64Attribute_(f, "/dataset1/how/scan_index", 1);

  //hid_t propAcc = H5Pcreate(H5P_FILE);
  //H5Pset_libver_bounds(propAcc, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
  //std::cout << "dbg - prop = " << propAcc << std::endl;
  //H5Pset_attr_phase_change(propAcc, 0, 0);
  //std::cout << "dbg - prop = " << propAcc << std::endl;
  hid_t how = H5Gopen2(f, "/dataset1/how", H5P_DEFAULT);

  //H5Adelete(how, "zr_a_A");

  /*
  const hsize_t dims[2] = {360,22};  // 22 - because it works only for attribute size up to 64 kB
                                     // To create a bigger attribute one need to use the "Large Attributes Stored in Dense Attribute Storage"
                                     // To use dense attribute storage to store large attributes, set the number of attributes that will be
                                     // stored in compact storage to 0 with the H5Pset_attr_phase_change function. This will force all
                                     // attributes to be put into dense attribute storage and will avoid the 64KB size limitation for a
                                     // single attribute in compact attribute storage.
  hid_t sp = H5Screate_simple (2, dims, NULL);



  auto a = H5Acreate2(how, "zr_a_A", H5T_NATIVE_DOUBLE, sp, H5P_DEFAULT, H5P_DEFAULT);
  if ( a < 0 ) {
    throw std::runtime_error("ERROR - attribute zr_a_A not opened.");
  }

  std::vector<double> attrValue(360*22, 200.0);


  H5Awrite(a, H5T_NATIVE_DOUBLE, attrValue.data());

  H5Aclose(a);

  H5Sclose(sp);
  */

  H5Gclose(how);

  H5Fclose(f);

  return 0;
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

void splitAttributeToPathAndName_(const std::string& attrName,
                                  std::string& path, std::string& name) {
  auto found = attrName.find_last_of('/');
  path = attrName.substr(0,found+1);
  name = attrName.substr(found+1);
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
