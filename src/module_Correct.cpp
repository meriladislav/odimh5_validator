// module_Correctcpp
// functions to correct, fix or change the ODIM-H5 files
// Ladislav Meri, SHMU
// v_0.0, 01.2020

#include <stdexcept>
#include <cstdio>
#include <hdf5.h>
#include "module_Correct.hpp"

namespace myodim {

//static void checkH5File_(const std::string& h5FilePath);
//static hid_t openH5File_(const std::string& h5FilePath, unsigned h5AccessFlag=H5F_ACC_RDONLY);

void copyFile(const std::string& sourceFile, const std::string& copyFile) {
  FILE* fIn = fopen(sourceFile.c_str(), "rb");
  if ( !fIn ) {
    throw std::runtime_error{"ERROR - file "+sourceFile+" not opened"};
  }
  fseek(fIn, 0L, SEEK_END);
  auto sz = ftell(fIn);
  rewind(fIn);
  std::vector<char> buffer(sz);
  fread(buffer.data(), sizeof(char), sz, fIn);
  fclose(fIn);

  FILE* fOut = fopen(copyFile.c_str(), "wb");
  if ( !fOut ) {
    throw std::runtime_error{"ERROR - file "+copyFile+" not opened"};
  }
  fwrite(buffer.data(), sizeof(char), sz, fOut);
  fclose(fOut);
}


//statics

/*
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
*/


} //end namespace myodim
