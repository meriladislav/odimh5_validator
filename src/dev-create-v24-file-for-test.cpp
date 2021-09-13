/*
 program to create a v2.4 file with a2D attribute to use it in the testing for the 2d attribute checks
*/

#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <hdf5.h>

const std::string inFile = "./data/example/v2.4/T_PAZE50_C_LFPW_20190426132340.h5";


int main() {

  hid_t f = H5Fopen(inFile.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
  if ( f < 0 ) {
    throw std::runtime_error{"ERROR - file "+inFile+" not opened"};
  }

  hid_t how = H5Gopen2(f, "/dataset1/how", H5P_DEFAULT);

  H5Adelete(how, "zr_a_A");

  const hsize_t dims[2] = {360,22};  // 22 - because it works only for attribute size up to 512 kB
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

  std::vector<double> attrValue(360*22);
  for (int r=0; r<360; ++r) {
    for (int b=0; b<22; ++b) {
      attrValue[r*22+b] = r;
    }
  }

  H5Awrite(a, H5T_NATIVE_DOUBLE, attrValue.data());

  H5Aclose(a);

  H5Sclose(sp);

  H5Gclose(how);

  H5Fclose(f);

  return 0;
}
