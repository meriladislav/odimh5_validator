// class_H5Explorer.cpp
// class to explore the hdf5 file content
// Ladislav Meri, SHMU
// v_0.0, 04.2018

#include "class_H5Layout.hpp"

#include <stdexcept>
#include <hdf5.h>

namespace myh5 {

static herr_t findGroupsAndDatasets(hid_t loc_id, const char* name, 
                                    const H5O_info_t* info, void* ph5explorer);
static herr_t getAttribueName(hid_t loc_id, const char* name, const H5A_info_t* ainfo, void* pNameStr);

H5Layout::H5Layout(const std::string& h5FilePath) {
  explore(h5FilePath);
}

H5Layout::~H5Layout() {
  reset_();
}

void H5Layout::explore(const std::string& h5FilePath) {
  reset_();
  
  h5FileID_ = H5Fopen(h5FilePath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);    
  if ( h5FileID_ < 0 ) {
    throw std::runtime_error{"ERROR - h5FileID_ "+h5FilePath+" not opened"};
  }
  herr_t status;
  
  status = H5Ovisit(h5FileID_, H5_INDEX_NAME, H5_ITER_NATIVE, findGroupsAndDatasets, this);
  if ( status < 0 ) {
    throw std::runtime_error{"ERROR - error while iterating objects in h5FileID_ "+h5FilePath};
  }
  
  findAttributes_();
}

void H5Layout::findAttributes_() {
  herr_t status;
  std::string attrName;
  for (const auto& group : groups) {
    hid_t g = H5Gopen2(h5FileID_, group.c_str(), H5P_DEFAULT);
    if ( g < 0 ) throw std::runtime_error{"ERROR - group "+group+" not opened"};
    status = H5Aiterate2(g, H5_INDEX_NAME, H5_ITER_INC, NULL, getAttribueName, &attrName);
    if ( status < 0 ) throw std::runtime_error{"ERROR - error while iterating attributes in group "+group};
    status = H5Gclose(g);
    if ( status < 0 ) throw std::runtime_error{"ERROR - error while closing group "+group};
    attributes.push_back(group+"/"+attrName);
  }
  
  for (const auto& dataset : datasets) {
    hid_t d = H5Dopen2(h5FileID_, dataset.c_str(), H5P_DEFAULT);
    if ( d < 0 ) throw std::runtime_error{"ERROR - dataset "+dataset+" not opened"};
    status = H5Aiterate2(d, H5_INDEX_NAME, H5_ITER_INC, NULL, getAttribueName, &attrName);
    if ( status < 0 ) throw std::runtime_error{"ERROR - error while iterating attributes in dataset "+dataset};
    status = H5Dclose(d);
    if ( status < 0 ) throw std::runtime_error{"ERROR - error while closing dataset "+dataset};
    attributes.push_back(dataset+"/"+attrName);
  }
}

void H5Layout::reset_() {
  if ( h5FileID_ > 0 ) {
    H5Fclose(h5FileID_);
  }
  h5FileID_ = -1;
  h5FilePath_ = "";
  groups.clear();
  datasets.clear();
  attributes.clear();
}


herr_t findGroupsAndDatasets(hid_t loc_id, const char* name, 
                             const H5O_info_t* info, void* ph5explorer) {
  H5Layout* h5exp = static_cast<H5Layout*>(ph5explorer);
  std::string objName{"/"};
  if (name[0] == '.')  {       /* Root group, do not print '.' */
    h5exp->groups.push_back(objName);
  }
  else {
    objName += std::string{name};
    switch (info->type) {
      case H5O_TYPE_GROUP:
        h5exp->groups.push_back(objName);
        break;
      case H5O_TYPE_DATASET:
        h5exp->datasets.push_back(objName);
        break;
      case H5O_TYPE_NAMED_DATATYPE:
        break;
      default:
        break;
    }
  } 
  return 0;
}

herr_t getAttribueName(hid_t loc_id, const char* name, const H5A_info_t* ainfo, void* pNameStr){
  std::string* nameStr = static_cast<std::string*>(pNameStr);
  *nameStr = name;
  return 0;
}


} // end namespace myh5



