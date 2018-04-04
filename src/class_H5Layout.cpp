// class_H5Explorer.cpp
// class to explore the hdf5 file content
// Ladislav Meri, SHMU
// v_0.0, 04.2018

#include "class_H5Layout.hpp"

#include <stdexcept>
#include <hdf5.h>
#include <algorithm>

namespace myh5 {

static herr_t fillGroupsAndDatasets(hid_t loc_id, const char* name, 
                                    const H5O_info_t* info, void* ph5explorer);
static herr_t getAttribueName(hid_t loc_id, const char* name, const H5A_info_t* ainfo, void* pNameStr);
void splitAttributeToPathAndName(const std::string& attrName, 
                                 std::string& path, std::string& name);

H5Layout::H5Layout(const std::string& h5FilePath) {
  explore(h5FilePath);
}

H5Layout::~H5Layout() {
  reset_();
}

void H5Layout::explore(const std::string& h5FilePath) {
  reset_();
  checkAndOpenFile_(h5FilePath);
  findGroupsAndDatasets_();
  findAttributes_();
}

bool H5Layout::hasAttribute(const std::string& attrName) const {
  return std::find(attributes.begin(), attributes.end(), attrName) != attributes.end();
}

std::string H5Layout::filePath() const {
  return h5FilePath_;
}

void H5Layout::getAttributeValue(const std::string& attrName, std::string& value) const {
  value = "";
  if ( hasAttribute(attrName) ) {
    std::string path, name;
    splitAttributeToPathAndName(attrName, path, name);
    auto parent = H5Gopen(h5FileID_, path.c_str(), H5P_DEFAULT);
    auto attr = H5Aopen(parent, name.c_str(), H5P_DEFAULT);
    auto type = H5Aget_type(attr);
    if ( H5Tget_class(type) != H5T_STRING ) throw -1;
    char str[1024] = {'\0'};
    auto ret  = H5Aread(attr, type, str);
    ret =  H5Aclose(attr);
    value = str;
    H5Gclose(parent);
  }
}

void H5Layout::checkAndOpenFile_(const std::string& h5FilePath) {
  if ( H5Fis_hdf5(h5FilePath.c_str()) <= 0 ) {
    throw std::runtime_error{"ERROR - file "+h5FilePath+" is not a HDF5 file"};
  }
  
  h5FileID_ = H5Fopen(h5FilePath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);    
  if ( h5FileID_ < 0 ) {
    throw std::runtime_error{"ERROR - file "+h5FilePath+" not opened"};
  }
  h5FilePath_ = h5FilePath;
}

void H5Layout::findGroupsAndDatasets_() {
  herr_t status = H5Ovisit(h5FileID_, H5_INDEX_NAME, H5_ITER_NATIVE, fillGroupsAndDatasets, this);
  if ( status < 0 ) {
    throw std::runtime_error{"ERROR - error while iterating objects in h5FileID_ "+h5FilePath_};
  }
}

void H5Layout::findAttributes_() {
  herr_t status;
  std::vector<std::string> attrNames;
  for (auto group : groups) {
    attrNames.clear();
    hid_t g = H5Gopen2(h5FileID_, group.c_str(), H5P_DEFAULT);
    if ( g < 0 ) throw std::runtime_error{"ERROR - group "+group+" not opened"};
    status = H5Aiterate2(g, H5_INDEX_NAME, H5_ITER_INC, NULL, getAttribueName, &attrNames);
    if ( status < 0 ) throw std::runtime_error{"ERROR - error while iterating attributes in group "+group};
    status = H5Gclose(g);
    if ( status < 0 ) throw std::runtime_error{"ERROR - error while closing group "+group};
    if ( group.back() != '/' ) group += "/";
    for (const auto& attrName : attrNames ) attributes.push_back(group+attrName);
  }
  
  for (auto dataset : datasets) {
    attrNames.clear();
    hid_t d = H5Dopen2(h5FileID_, dataset.c_str(), H5P_DEFAULT);
    if ( d < 0 ) throw std::runtime_error{"ERROR - dataset "+dataset+" not opened"};
    status = H5Aiterate2(d, H5_INDEX_NAME, H5_ITER_INC, NULL, getAttribueName, &attrNames);
    if ( status < 0 ) throw std::runtime_error{"ERROR - error while iterating attributes in dataset "+dataset};
    status = H5Dclose(d);
    if ( status < 0 ) throw std::runtime_error{"ERROR - error while closing dataset "+dataset};
    if ( dataset.back() != '/' ) dataset += "/";
    for (const auto& attrName : attrNames ) attributes.push_back(dataset+attrName);
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


herr_t fillGroupsAndDatasets(hid_t loc_id, const char* name, 
                             const H5O_info_t* info, void* ph5explorer) {
  H5Layout* h5exp = static_cast<H5Layout*>(ph5explorer);
  std::string objName{"/"};
  if (name[0] == '.')  {       /* Root group */
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

herr_t getAttribueName(hid_t loc_id, const char* name, const H5A_info_t* ainfo, void* pNames){
  std::vector<std::string>* names = static_cast<std::vector<std::string>*>(pNames);
  names->emplace_back(std::string{name});
  return 0;
}

void splitAttributeToPathAndName(const std::string& attrName, 
                                 std::string& path, std::string& name) {
  auto found = attrName.find_last_of('/');
  path = attrName.substr(0,found+1);
  name = attrName.substr(found+1);
}


} // end namespace myh5



