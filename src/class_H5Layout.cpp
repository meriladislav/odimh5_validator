// class_H5Layout.cpp
// class to map the hdf5 file content
// Ladislav Meri, SHMU

#include "class_H5Layout.hpp"

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <limits>

namespace myodim {

static herr_t fillGroupsAndDatasets(hid_t loc_id, const char* name, 
                                    const H5O_info_t* info, void* ph5layout);
static herr_t getAttributeName(hid_t loc_id, const char* name, const H5A_info_t* ainfo, void* pNameStr);
static void splitAttributeToPathAndName(const std::string& attrName, 
                                        std::string& path, std::string& name);
static void closeAll(const std::vector<hid_t>& ids);

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

bool H5Layout::hasDataset(const std::string& dsetName) const {
  return std::find(datasets.begin(), datasets.end(), h5Entry(dsetName,false)) != datasets.end() ||
         std::find(datasets.begin(), datasets.end(), h5Entry(dsetName,true)) != datasets.end();
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
  auto status = H5Aiterate2(object, H5_INDEX_NAME, H5_ITER_INC, NULL, getAttributeName, &attrNames);
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
    closeAll({parent});
    throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
  }
  auto type = H5Aget_type(attr);
  if ( type < 0 ) {
    closeAll({attr, parent});
    throw std::runtime_error("ERROR - attribute type not found");
  }
  if ( H5Tget_class(type) != H5T_STRING ) {
    closeAll({type, attr, parent});
    throw std::runtime_error("ERROR - attribute "+attrName+" is not a STRING attribute");
  }
  if ( H5Tis_variable_str(type) ) {
    closeAll({type, attr, parent});
	  throw std::runtime_error("WARNING - NON-STANDARD DATA TYPE - attribute "+
			                   attrName+" is a variable-length string attribute,"+
			                   " which is not supported by the ODIM standard.");
  }

  auto sz = H5Tget_size(type);

  char* str;
  str = (char*)malloc(sz+1);
  str[sz] = '\0';
  auto ret  = H5Aread(attr, type, str);
  if ( ret < 0  ) {
    closeAll({type, attr, parent});
    throw std::runtime_error("ERROR - attribute "+attrName+" not read");
  }
  value = str;
  free(str);

  if ( value.length()+1 != sz ) {
    closeAll({type, attr, parent});
	  throw std::runtime_error("WARNING - STRSIZE error - attribute "+
			                   attrName+"`s size is set to "+std::to_string(sz)+", but it should be "+
                         std::to_string(value.length()+1)+ " (value.length+1) - value is now: "+value);
  }

  closeAll({type, attr, parent});
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
    H5Oclose(parent);
    throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
  }
  auto ret  = H5Aread(attr, H5T_NATIVE_DOUBLE, &value);
  if ( ret < 0  ) {
    closeAll({attr, parent});
    throw std::runtime_error("ERROR - attribute "+attrName+" not read");
  }
  closeAll({attr, parent});
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
    H5Oclose(parent);
    throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
  }
  auto ret  = H5Aread(attr, H5T_NATIVE_INT64, &value);
  if ( ret < 0  ) {
    closeAll({attr, parent});
    throw std::runtime_error("ERROR - attribute "+attrName+" not read");
  }
  closeAll({attr, parent});
}


void H5Layout::getAttributeValue(const std::string& attrName, std::vector<double>& values) const {
  if ( is1DArrayAttribute(attrName) ) {
    std::string path, name;
    splitAttributeToPathAndName(attrName, path, name);
    auto parent = H5Oopen(h5FileID_, path.c_str(), H5P_DEFAULT);
    if ( parent < 0  ) {
      throw std::runtime_error("ERROR - node "+path+" not opened");
    }
    auto attr = H5Aopen(parent, name.c_str(), H5P_DEFAULT);
    if ( attr < 0  ) {
      H5Oclose(parent);
      throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
    }
    auto space = H5Aget_space(attr);
    hsize_t size=0;
    H5Sget_simple_extent_dims(space, &size, NULL);
    values.resize(size, 0.0);
    auto ret = H5Aread(attr, H5T_NATIVE_DOUBLE, values.data());
    if ( ret < 0  ) {
      closeAll({space, attr, parent});
      throw std::runtime_error("ERROR - attribute "+attrName+" not read");
    }
    closeAll({space, attr, parent});
  }
  else if ( is2DArrayAttribute(attrName) ) {
    std::string path, name;
    splitAttributeToPathAndName(attrName, path, name);
    auto parent = H5Oopen(h5FileID_, path.c_str(), H5P_DEFAULT);
    if ( parent < 0  ) {
      throw std::runtime_error("ERROR - node "+path+" not opened");
    }
    auto attr = H5Aopen(parent, name.c_str(), H5P_DEFAULT);
    if ( attr < 0  ) {
      H5Oclose(parent);
      throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
    }
    auto space = H5Aget_space(attr);

    hsize_t sizes[2];
    H5Sget_simple_extent_dims(space, sizes, NULL);

    values.resize(sizes[0]*sizes[1], 0.0);
    auto ret = H5Aread(attr, H5T_NATIVE_DOUBLE, values.data());
    if ( ret < 0  ) {
      closeAll({space, attr, parent});
      throw std::runtime_error("ERROR - attribute "+attrName+" not read");
    }
    closeAll({space, attr, parent});
  }
  else {
    values.resize(1);
    getAttributeValue(attrName, values[0]);
  }
}

void H5Layout::getAttributeValue(const std::string& attrName, std::vector<int64_t>& values) const {
  if ( is1DArrayAttribute(attrName) ) {
    std::string path, name;
    splitAttributeToPathAndName(attrName, path, name);
    auto parent = H5Oopen(h5FileID_, path.c_str(), H5P_DEFAULT);
    if ( parent < 0  ) {
      throw std::runtime_error("ERROR - node "+path+" not opened");
    }
    auto attr = H5Aopen(parent, name.c_str(), H5P_DEFAULT);
    if ( attr < 0  ) {
      H5Oclose(parent);
      throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
    }
    auto space = H5Aget_space(attr);
    hsize_t size=0;
    H5Sget_simple_extent_dims(space, &size, NULL);
    values.resize(size, 0.0);
    auto ret = H5Aread(attr, H5T_NATIVE_INT64, values.data());
    if ( ret < 0  ) {
      closeAll({space, attr, parent});
      throw std::runtime_error("ERROR - attribute "+attrName+" not read");
    }
    closeAll({space, attr, parent});
  }
  else if ( is2DArrayAttribute(attrName) ) {
    std::string path, name;
    splitAttributeToPathAndName(attrName, path, name);
    auto parent = H5Oopen(h5FileID_, path.c_str(), H5P_DEFAULT);
    if ( parent < 0  ) {
      throw std::runtime_error("ERROR - node "+path+" not opened");
    }
    auto attr = H5Aopen(parent, name.c_str(), H5P_DEFAULT);
    if ( attr < 0  ) {
      H5Oclose(parent);
      throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
    }
    auto space = H5Aget_space(attr);
    hsize_t sizes[2];
    H5Sget_simple_extent_dims(space, sizes, NULL);
    values.resize(sizes[0]*sizes[1], 0.0);
    auto ret = H5Aread(attr, H5T_NATIVE_INT64, values.data());
    if ( ret < 0  ) {
      closeAll({space, attr, parent});
      throw std::runtime_error("ERROR - attribute "+attrName+" not read");
    }
    closeAll({space, attr, parent});
  }
  else {
    values.resize(1);
    getAttributeValue(attrName, values[0]);
  }
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
    H5Oclose(parent);
    throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
  }
  auto type = H5Aget_type(attr);
  const bool isString = H5Tget_class(type) == H5T_STRING;
  closeAll({type, attr, parent});
  return isString;
}

bool H5Layout::isFixedLengthStringAttribute(const std::string& attrName, std::string& errMsg) const {
  std::string path, name;
  splitAttributeToPathAndName(attrName, path, name);
  auto parent = H5Oopen(h5FileID_, path.c_str(), H5P_DEFAULT);
  if ( parent < 0  ) {
    throw std::runtime_error("ERROR - node "+path+" not opened");
  }
  auto attr = H5Aopen(parent, name.c_str(), H5P_DEFAULT);
  if ( attr < 0  ) {
    H5Oclose(parent);
    throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
  }
  auto type = H5Aget_type(attr);
  bool isString = H5Tget_class(type) == H5T_STRING;
  if ( !isString ) {
    errMsg = "is not a string";
    closeAll({type, attr, parent});
    return isString;
  }
  isString = isString && !H5Tis_variable_str(type);
  if ( !isString ) {
    errMsg = "is not a fixed length string";
    closeAll({type, attr, parent});
    return isString;
  }
  isString = isString && H5Tget_strpad(type) == H5T_STR_NULLTERM;
  if ( !isString ) {
    errMsg = "is not a H5T_STR_NULLTERM terminated string";
    closeAll({type, attr, parent});
    return isString;
  }
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
    H5Oclose(parent);
    throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
  }
  auto type = H5Aget_type(attr);
  bool isReal64{H5Tget_class(type) == H5T_FLOAT && H5Tget_precision(type) == 64};
  closeAll({type, attr, parent});
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
    H5Oclose(parent);
    throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
  }
  auto type = H5Aget_type(attr);
  bool isInt64{H5Tget_class(type) == H5T_INTEGER && H5Tget_precision(type) == 64};
  closeAll({type, attr, parent});
  return isInt64;
}

bool H5Layout::isBooleanAttribute(const std::string& attrName) const {
  std::string path, name;
  splitAttributeToPathAndName(attrName, path, name);
  auto parent = H5Oopen(h5FileID_, path.c_str(), H5P_DEFAULT);
  if ( parent < 0  ) {
    throw std::runtime_error("ERROR - node "+path+" not opened");
  }
  auto attr = H5Aopen(parent, name.c_str(), H5P_DEFAULT);
  if ( attr < 0  ) {
    H5Oclose(parent);
    throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
  }
  auto type = H5Aget_type(attr);
  bool isBool{H5Tget_class(type) == H5T_NATIVE_HBOOL};
  closeAll({type, attr, parent});
  return isBool;
}

bool H5Layout::isUcharDataset(const std::string& dsetName) const {
  bool isUchar = false;
  auto dset = H5Dopen(h5FileID_, dsetName.c_str(), H5P_DEFAULT);
  if ( dset < 0  ) {
    throw std::runtime_error("ERROR - node "+dsetName+" not opened");
  }
  auto type = H5Dget_type(dset);
  isUchar = H5Tget_class(type) == H5T_INTEGER &&
		        H5Tget_precision(type) == 8 &&
			      H5Tget_sign(type) == H5T_SGN_NONE;
  closeAll({type, dset});
  return isUchar;
}

bool H5Layout::is1DArrayAttribute(const std::string& attrName) const {
  std::string path, name;
  splitAttributeToPathAndName(attrName, path, name);
  auto parent = H5Oopen(h5FileID_, path.c_str(), H5P_DEFAULT);
  if ( parent < 0  ) {
    throw std::runtime_error("ERROR - node "+path+" not opened");
  }
  auto attr = H5Aopen(parent, name.c_str(), H5P_DEFAULT);
  if ( attr < 0  ) {
    H5Oclose(parent);
    throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
  }
  auto space = H5Aget_space(attr);
  auto rank = H5Sget_simple_extent_ndims(space);
  bool isArray = rank == 1;
  closeAll({space, attr, parent});
  return isArray;
}

bool H5Layout::is2DArrayAttribute(const std::string& attrName) const {
  std::string path, name;
  splitAttributeToPathAndName(attrName, path, name);
  auto parent = H5Oopen(h5FileID_, path.c_str(), H5P_DEFAULT);
  if ( parent < 0  ) {
    throw std::runtime_error("ERROR - node "+path+" not opened");
  }
  auto attr = H5Aopen(parent, name.c_str(), H5P_DEFAULT);
  if ( attr < 0  ) {
    H5Oclose(parent);
    throw std::runtime_error("ERROR - attribute "+attrName+" not opened");
  }
  auto space = H5Aget_space(attr);
  auto rank = H5Sget_simple_extent_ndims(space);
  bool is2DArray = rank == 2;
  closeAll({space, attr, parent});
  return is2DArray;
}

void H5Layout::attributeStatistics(const std::string& attrName, double& first, double& last,
                                   double& min, double& max, double& mean) const {
  if ( is1DArrayAttribute(attrName) || is2DArrayAttribute(attrName) ) {
    std::vector<double> values;
    getAttributeValue(attrName, values);
    first = values.front();
    last = values.back();
    min = std::numeric_limits<double>::max();
    max = std::numeric_limits<double>::lowest();
    mean = 0.0;
    for (const double v : values) {
      if ( v < min ) min = v;
      if ( v > max ) max = v;
      mean += v;
    }
    mean /= values.size();
  }
  else {
    getAttributeValue(attrName, first);
    last = first; min = first; max = first; mean = first;
  }
}

void H5Layout::attributeStatistics(const std::string& attrName, int64_t& first, int64_t& last,
                                   int64_t& min, int64_t& max, int64_t& mean) const {
  if ( is1DArrayAttribute(attrName) ) {
    std::vector<int64_t> values;
    getAttributeValue(attrName, values);
    first = values.front();
    last = values.back();
    min = std::numeric_limits<int64_t>::max();
    max = std::numeric_limits<int64_t>::lowest();
    mean = 0.0;
    for (const int64_t v : values) {
      if ( v < min ) min = v;
      if ( v > max ) max = v;
      mean += v;
    }
    mean /= values.size();
  }
  else {
    getAttributeValue(attrName, first);
    last = first; min = first; max = first; mean = first;
  }
}

bool H5Layout::ucharDatasetHasImageAttributes(const std::string& dsetPath) const {
  if ( !hasAttribute(dsetPath+"/CLASS") ) return false;
  if ( !hasAttribute(dsetPath+"/IMAGE_VERSION") ) return false;
  std::string attrValue;
  getAttributeValue(dsetPath+"/CLASS", attrValue);
  if ( attrValue != "IMAGE" ) return false;
  getAttributeValue(dsetPath+"/IMAGE_VERSION", attrValue);
  if ( attrValue != "1.2" ) return false;
  return true;
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
  herr_t status = H5Ovisit(h5FileID_, H5_INDEX_NAME, H5_ITER_NATIVE, fillGroupsAndDatasets, this); //doesn`t work with hdf5 v1.12, but works with -DH5_USE_18_API
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
    objName += std::string(name);
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

herr_t getAttributeName(hid_t loc_id, const char* name, const H5A_info_t* ainfo, void* pNames) {
  if ( loc_id < 0 || ainfo->data_size <= 0 ) return -1;
  std::vector<std::string>* names = static_cast<std::vector<std::string>*>(pNames);
  names->emplace_back(std::string(name));
  return 0;
}

void splitAttributeToPathAndName(const std::string& attrName, 
                                 std::string& path, std::string& name) {
  auto found = attrName.find_last_of('/');
  path = attrName.substr(0,found+1);
  name = attrName.substr(found+1);
}

void closeAll(const std::vector<hid_t>& ids) {
  for (size_t i=0; i<ids.size(); ++i) {
    H5Oclose(ids[i]);
  }
}

} // end namespace myh5



