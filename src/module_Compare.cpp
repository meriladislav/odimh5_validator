// module_Compare.cpp
// functions to compare the hdf5 layout with the ODIM specification
// Ladislav Meri, SHMU
// v_0.0, 04.2018

#include <cstdlib>  // getenv
#include <stdexcept>
#include <algorithm>
#include "module_Compare.hpp"

namespace myodim {

const std::string csvDirPathEnv{"ODIMH5_VALIDATOR_CSV_DIR"};

std::string getCsvFileNameFrom(const myh5::H5Layout& h5layout) {
  std::string result;
  const char* csvDir = std::getenv(csvDirPathEnv.c_str());
  if ( csvDir ) {
    result = csvDir;
  }
  else {
    throw std::runtime_error{"ERROR - environment variable "+csvDirPathEnv+" not found. "+
                             "Please specify it: export "+csvDirPathEnv+"=your_csv_data_directory_path"};
  }
  if ( result.back() != '/' ) result += "/";
  
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
  
  result += conventions+"_"+object+".csv";
   
  return result;
}

bool compare(const myh5::H5Layout& h5layout, const OdimStandard& odimStandard) {
  return false;
}

}
