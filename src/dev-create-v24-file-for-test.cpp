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

  //overwrite "/Conventions"
  {
    hid_t root = H5Gopen2(f, "/", H5P_DEFAULT);
    H5Adelete(root, "Conventions");
    auto sp  = H5Screate(H5S_SCALAR);
    auto t = H5Tcopy(H5T_C_S1);
    H5Tset_size(t, std::string("ODIM_H5/V2_4").length()+1);
    H5Tset_strpad(t, H5T_STR_NULLTERM);
    auto a = H5Acreate2(root, "Conventions", t, sp, H5P_DEFAULT, H5P_DEFAULT);
    H5Awrite(a, t, "ODIM_H5/V2_4");
    H5Sclose(sp);
    H5Tclose(t);
    H5Aclose(a);
    H5Gclose(root);
  }

  //overwrite "/what/version"
  {
    hid_t what = H5Gopen2(f, "/what", H5P_DEFAULT);
    H5Adelete(what, "version");
    auto sp  = H5Screate(H5S_SCALAR);
    auto t = H5Tcopy(H5T_C_S1);
    H5Tset_size(t, std::string("H5rad 2.4").length()+1);
    H5Tset_strpad(t, H5T_STR_NULLTERM);
    auto a = H5Acreate2(what, "version", t, sp, H5P_DEFAULT, H5P_DEFAULT);
    H5Awrite(a, t, "H5rad 2.4");
    H5Sclose(sp);
    H5Tclose(t);
    H5Aclose(a);
    H5Gclose(what);
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
