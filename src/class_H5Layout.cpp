// class_H5Layout.cpp
// class to map the hdf5 file content
// Ladislav Meri, SHMU

#include "class_H5Layout.hpp"

#include <stdexcept>
#include <algorithm>

namespace myodim {

static herr_t fillGroupsAndDatasets(hid_t loc_id, const char* name, 
                                    const H5O_info_t* info, void* ph5layout);
static herr_t getAttribueName(hid_t loc_id, const char* name, const H5A_info_t* ainfo, void* pNameStr);
void splitAttributeToPathAndName(const std::string& attrName, 
                                 std::string& path, std::string& name);

H5Layout::H5Layout() {
  H5Eset_auto( H5E_DEFAULT, NULL, NULL ); //Turn off error handling permanently
}

H5Layout::H5Layout(const std::string& h5FilePath) {
  H5Eset_auto( H5E_DEFAULT, NULL, NULL ); //Turn off error handling permanently 
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
  return std::find(attributes.begin(), attributes.end(), h5Entry(attrName,false)) != attributes.end() ||
         std::find(attributes.begin(), attributes.end(), h5Entry(attrName,true)) != attributes.end();
}

bool H5Layout::hasGroup(const std::string& groupName) const {
  return std::find(groups.begin(), groups.end(), h5Entry(groupName,false)) != groups.end() ||
         std::find(groups.begin(), groups.end(), h5Entry(groupName,true)) != groups.end();
}

std::string H5Layout::filePath() const {
  return h5FilePath_;
}

std::vector<std::string> H5Layout::getAttributeNames(const std::string& objPath) const {
  bool isGroup = true;
  hid_t object = H5Gopen2(h5FileID_, objPath.c_str(), H5P_DEFAULT);
  if ( object < 0 ) {
    object = H5Dopen2(h5FileID_, objPath.c_str(), H5P_DEFAULT);
    if ( object < 0 ) {
      throw std::runtime_error{"ERROR - object "+objPath+" not opened"};
    }
    isGroup = false;
  }
  std::vector<std::string> attrNames;
  auto status = H5Aiterate2(object, H5_INDEX_NAME, H5_ITER_INC, NULL, getAttribueName, &attrNames);
  if ( status < 0 ) throw std::runtime_error{"ERROR - error while iterating attributes in object "+objPath};
  if ( isGroup ) status = H5Gclose(object);
  else status = H5Dclose(object);
  if ( status < 0 ) throw std::runtime_error{"ERROR - error while closing object "+objPath};
  return attrNames;
}

void H5Layout::getAttributeValue(const std::string& attrName, std::string& value) const {
  value = "";
  std::string path, name;
  splitAttributeToPathAndName(attrName, path, name);
  auto parent = H5Oopen(h5FileID_, path.c_str(), H5P_DEFAULT);
  if ( parent < 0  ) {
    throw std::runtime_error("ERROR - node "+path+" not opened");
  }
  auto attr = H5Aopen(parent, name.c_str(), H5P_DEFAULT);
  if ( attr < 0  ) {
    throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
  }
  auto type = H5Aget_type(attr);
  if ( H5Tget_class(type) != H5T_STRING ) {
    throw std::runtime_error("ERROR - attribute "+attrName+" is not a STRING attribute");
  }
  char str[1024] = {'\0'};
  auto ret  = H5Aread(attr, type, str);
  if ( ret < 0  ) {
    throw std::runtime_error("ERROR - attribute "+attrName+" not read");
  }
  H5Aclose(attr);
  value = str;
  H5Oclose(parent);
}

void H5Layout::getAttributeValue(const std::string& attrName, double& value) const {
  std::string path, name;
  splitAttributeToPathAndName(attrName, path, name);
  auto parent = H5Oopen(h5FileID_, path.c_str(), H5P_DEFAULT);
  if ( parent < 0  ) {
    throw std::runtime_error("ERROR - node "+path+" not opened");
  }
  auto attr = H5Aopen(parent, name.c_str(), H5P_DEFAULT);
  if ( attr < 0  ) {
    throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
  }
  auto ret  = H5Aread(attr, H5T_NATIVE_DOUBLE, &value);
  if ( ret < 0  ) {
    throw std::runtime_error("ERROR - attribute "+attrName+" not read");
  }
  H5Aclose(attr);
  H5Oclose(parent);
}

void H5Layout::getAttributeValue(const std::string& attrName, int64_t& value) const {
  std::string path, name;
  splitAttributeToPathAndName(attrName, path, name);
  auto parent = H5Oopen(h5FileID_, path.c_str(), H5P_DEFAULT);
  if ( parent < 0  ) {
    throw std::runtime_error("ERROR - node "+path+" not opened");
  }
  auto attr = H5Aopen(parent, name.c_str(), H5P_DEFAULT);
  if ( attr < 0  ) {
    throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
  }
  auto ret  = H5Aread(attr, H5T_NATIVE_INT64, &value);
  if ( ret < 0  ) {
    throw std::runtime_error("ERROR - attribute "+attrName+" not read");
  }
  H5Aclose(attr);
  H5Oclose(parent);
}

bool H5Layout::isStringAttribute(const std::string& attrName) const {
  std::string path, name;
  splitAttributeToPathAndName(attrName, path, name);
  auto parent = H5Oopen(h5FileID_, path.c_str(), H5P_DEFAULT);
  if ( parent < 0  ) {
    throw std::runtime_error("ERROR - node "+path+" not opened");
  }
  auto attr = H5Aopen(parent, name.c_str(), H5P_DEFAULT);
  if ( attr < 0  ) {
    throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
  }
  auto type = H5Aget_type(attr);
  bool isString{H5Tget_class(type) == H5T_STRING};
  H5Tclose(type);
  H5Aclose(attr);
  H5Oclose(parent);
  return isString;
}

bool H5Layout::isReal64Attribute(const std::string& attrName) const {
  std::string path, name;
  splitAttributeToPathAndName(attrName, path, name);
  auto parent = H5Oopen(h5FileID_, path.c_str(), H5P_DEFAULT);
  if ( parent < 0  ) {
    throw std::runtime_error("ERROR - node "+path+" not opened");
  }
  auto attr = H5Aopen(parent, name.c_str(), H5P_DEFAULT);
  if ( attr < 0  ) {
    throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
  }
  auto type = H5Aget_type(attr);
  bool isReal64{H5Tget_class(type) == H5T_FLOAT && H5Tget_precision(type) == 64};
  H5Tclose(type);
  H5Aclose(attr);
  H5Oclose(parent);
  return isReal64;
}

bool H5Layout::isInt64Attribute(const std::string& attrName) const {
  std::string path, name;
  splitAttributeToPathAndName(attrName, path, name);
  auto parent = H5Oopen(h5FileID_, path.c_str(), H5P_DEFAULT);
  if ( parent < 0  ) {
    throw std::runtime_error("ERROR - node "+path+" not opened");
  }
  auto attr = H5Aopen(parent, name.c_str(), H5P_DEFAULT);
  if ( attr < 0  ) {
    throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
  }
  auto type = H5Aget_type(attr);
  bool isInt64{H5Tget_class(type) == H5T_INTEGER && H5Tget_precision(type) == 64};
  H5Tclose(type);
  H5Aclose(attr);
  H5Oclose(parent);
  return isInt64;
}

bool H5Layout::isBooleanAttribute(const std::string& attrName) const {
  std::string path, name;
  splitAttributeToPathAndName(attrName, path, name);
  auto parent = H5Oopen(h5FileID_, path.c_str(), H5P_DEFAULT);
  auto attr = H5Aopen(parent, name.c_str(), H5P_DEFAULT);
  auto type = H5Aget_type(attr);
  bool isBool{H5Tget_class(type) == H5T_NATIVE_HBOOL};
  H5Tclose(type);
  H5Aclose(attr);
  H5Oclose(parent);
  return isBool;
}

bool H5Layout::isUcharDataset(const std::string& dsetName) const {
  bool isUchar = false;
  auto dset = H5Dopen(h5FileID_, dsetName.c_str(), H5P_DEFAULT);
  auto type = H5Dget_type(dset);
  isUchar = H5Tget_class(type) == H5T_INTEGER &&
		    H5Tget_precision(type) == 8 &&
			H5Tget_sign(type) == H5T_SGN_NONE;
  H5Tclose(type);
  H5Dclose(dset);
  return isUchar;
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
  std::vector<std::string> attrNames;
  for (auto gr : groups) {
    auto& group = gr.name();
    attrNames.clear();
    attrNames = getAttributeNames(group);
    if ( group.back() != '/' ) group += "/";
    for (const auto& attrName : attrNames ) attributes.push_back(h5Entry(group+attrName, false));
  }
  
  for (auto dset : datasets) {
    auto& dataset = dset.name();
    attrNames.clear();
    attrNames = getAttributeNames(dataset);
    if ( dataset.back() != '/' ) dataset += "/";
    for (const auto& attrName : attrNames ) attributes.push_back(h5Entry(dataset+attrName, false));
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
                             const H5O_info_t* info, void* ph5layout) {
  if ( loc_id < 0 ) return -1;
  H5Layout* h5lay = static_cast<H5Layout*>(ph5layout);
  std::string objName{"/"};
  if (name[0] == '.')  {       /* Root group */
    h5lay->groups.push_back(h5Entry(objName, false));
  }
  else {
    objName += std::string{name};
    switch (info->type) {
      case H5O_TYPE_GROUP:
        h5lay->groups.push_back(h5Entry(objName, false));
        break;
      case H5O_TYPE_DATASET:
        h5lay->datasets.push_back(h5Entry(objName, false));
        break;
      case H5O_TYPE_NAMED_DATATYPE:
        break;
      default:
        break;
    }
  } 
  return 0;
}

herr_t getAttribueName(hid_t loc_id, const char* name, const H5A_info_t* ainfo, void* pNames) {
  if ( loc_id < 0 || ainfo->data_size <= 0 ) return -1;
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



