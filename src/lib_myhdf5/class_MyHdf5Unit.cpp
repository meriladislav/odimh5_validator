// class_MyHdf5Unit.cpp
// class to work with the hdf5 files, groups and datasets (by hidding the hdf5 API)
// Ladislav Meri, SHMU
// v_0.0, 01.2014

#include <iostream>
#include <stdint.h>
#include <cstring>
#include <stdexcept>

#include "class_MyHdf5Unit.hpp"

char *get_variable_string_attribute(hid_t attrID);

MyHdf5Unit::MyHdf5Unit():
id_(0), type_(UNKNOWN_) {
  /*turn out the error messages*/
  H5Eset_auto( H5E_DEFAULT, NULL, NULL );
}

MyHdf5Unit::~MyHdf5Unit() {
  close();
}

unsigned int MyHdf5Unit::id() const {
  return static_cast<unsigned int>(id_);
}

Hdf5UnitType MyHdf5Unit::type() const {
  return type_;
}

void MyHdf5Unit::checkId_() {
  if ( id_ < 0 ) {
    id_ = 0;
    throw std::runtime_error{"hdf5 ID is negative"};
  }
}

void MyHdf5Unit::reset_() {
  id_ = 0;
  type_ = UNKNOWN_;
}

bool MyHdf5Unit::isOpen() const {
  return id_>0 && type_ != UNKNOWN_;
}

void MyHdf5Unit::openFile(const std::string& fileName, Hdf5UnitAccess access) {
  if ( isOpen() ) close();
  
  switch (access) {
    case READ_: {
      if ( !H5Fis_hdf5(fileName.c_str()) ) {
        throw std::runtime_error{"File "+fileName+" is not a hdf5 file"};
      }
      FILE* f = fopen(fileName.c_str(), "rb");
      if ( !f ) throw std::runtime_error{"File "+fileName+" is not opened"};;
      fclose(f);
      //break missing to skip to case READWRITE_
    }
    case READWRITE_: {
      if ( !H5Fis_hdf5(fileName.c_str()) ) {
        throw std::runtime_error{"File "+fileName+" is not a hdf5 file"};
      }
      unsigned int flag = H5F_ACC_RDONLY;
      if ( access == READWRITE_ ) flag = H5F_ACC_RDWR;
      id_ = H5Fopen(fileName.c_str(), flag, H5P_DEFAULT);
      checkId_();
      type_ = FILE_;  
      break;
    }
    case WRITE_:
      id_ = H5Fcreate(fileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
      checkId_();
      type_ = FILE_;  
      break;
    default:
      throw std::runtime_error{"Unknown Hdf5UnitAccess"};
      break;
  }
}

void MyHdf5Unit::closeFile() {
  if ( isOpen() ) {
    if (H5Fclose(id_)<0) throw std::runtime_error{"File not closed"};
    reset_();
  }
}

void MyHdf5Unit::openGroup(const MyHdf5Unit &sourceUnit, const std::string& groupName) {
  if ( isOpen() ) close();
  if ( !sourceUnit.isOpen() ) throw std::runtime_error{"SourceUnit is not opened"};
  id_ = H5Gopen( sourceUnit.id(), groupName.c_str(), H5P_DEFAULT );
  checkId_();
  type_ = GROUP_;
}

void MyHdf5Unit::closeGroup() {
  if ( isOpen() ) {
    if (H5Gclose(id_)<0) throw std::runtime_error{"Group not closed"};
    reset_();
  }
}

void MyHdf5Unit::openDataset(const MyHdf5Unit &sourceUnit, const std::string& datasetName) {
  if ( isOpen() ) close();
  if ( !sourceUnit.isOpen() ) throw std::runtime_error{"SourceUnit is not opened"};
  id_ = H5Dopen( sourceUnit.id(), datasetName.c_str(), H5P_DEFAULT );
  checkId_();
  type_ = DATASET_;
}

void MyHdf5Unit::closeDataset() {
  if ( isOpen() ) {
    if (H5Dclose(id_)<0) throw std::runtime_error{"Dataset not closed"};
    reset_();
  }
}

void MyHdf5Unit::open(const std::string& fileName, Hdf5UnitAccess access) {
  if ( isOpen() ) close();
  openFile(fileName, access);
}

void MyHdf5Unit::open(const MyHdf5Unit &sourceUnit, const std::string& name) {
  if ( isOpen() ) close();
  H5O_info_t info;
  if ( H5Oget_info_by_name(sourceUnit.id(), name.c_str(), &info, H5P_DEFAULT ) < 0 ) throw -1;
  switch (info.type) {
    case H5O_TYPE_GROUP:
      openGroup(sourceUnit, name);
      break;
    case H5O_TYPE_DATASET:
      openDataset(sourceUnit, name);
      break;
    default:
      throw -1;
      break;
  }
}

void MyHdf5Unit::close() {
  if ( !isOpen() ) return;
  switch (type_) {
    case FILE_:
      closeFile();
      break;
    case GROUP_:
      closeGroup();
      break;
    case DATASET_:
      closeDataset();
      break;
    default:
      break;
  }
}

int MyHdf5Unit::numberOfLinksInGroup() {
  if ( !isOpen() || type_ != GROUP_ ) throw -1;
  H5G_info_t GrInfo;
  if ( H5Gget_info (id_, &GrInfo) < 0 ) throw -1;
  return GrInfo.nlinks;
}

int MyHdf5Unit::linkNameSizeByIndex(int index) {
  if ( index < 0 || index >= numberOfLinksInGroup() ) throw -1;
  int nameSize = H5Lget_name_by_idx (id_, ".", H5_INDEX_NAME, H5_ITER_INC, index, NULL, 0, H5P_DEFAULT);
  if ( nameSize < 0 ) throw -1;
  nameSize++;
  return nameSize;
}

std::string MyHdf5Unit::linkNameByIndex(int index, int &nameSize) {
  nameSize = linkNameSizeByIndex(index);
  char* linkName = new char[nameSize];
  if ( !linkName ) throw -1;
  for (int i=0; i<nameSize; i++) linkName[i] = '\0';
  H5Lget_name_by_idx (id_, ".", H5_INDEX_NAME, H5_ITER_INC, index, linkName, (size_t) nameSize, H5P_DEFAULT);
  std::string result{linkName};
  delete[] linkName;
  return result;
}

int MyHdf5Unit::numberOfAttributes() {
  H5O_info_t info;
  if ( H5Oget_info(id_, &info) < 0 ) throw -1;
  return info.num_attrs;
}

bool MyHdf5Unit::hasAttribute(const std::string& name) {
  htri_t exist;
  exist = H5Aexists(id_, name.c_str());
  if ( exist < 0 ) throw -1;
  return static_cast<bool>(exist);
}

void MyHdf5Unit::getAttribute(const std::string& name, char *&value, int &valueSize) {
  hid_t attr = H5Aopen(id_, name.c_str() , H5P_DEFAULT);
  if ( attr < 0 ) throw -1;
  hid_t type = H5Aget_type(attr);
  if ( H5Tget_class(type) != H5T_STRING ) throw -1;
  
  if ( H5Tis_variable_str(type) ) {
    value = get_variable_string_attribute(attr);
    valueSize = strlen(value)+1;
  }
  else {
    valueSize = H5Aget_storage_size(attr);
    hid_t nativeType = H5Tget_native_type(type, H5T_DIR_ASCEND);
  
    if ( value ) {
      delete[] value;
      value = 0;
    }
    value = new char[valueSize];
    if ( !value ) throw -1;
    for (int i=0; i<valueSize; i++) value[i] = '\0';
  
    if ( H5Aread(attr, nativeType, value) < 0 ) throw -1;
    H5Tclose(nativeType);
  }
  
  H5Tclose(type);
  H5Aclose(attr);
}

void MyHdf5Unit::getAttribute(const std::string& name, int &value) {
  hid_t attr = H5Aopen(id_, name.c_str(), H5P_DEFAULT);
  if ( attr < 0 ) throw -1;
  hid_t type = H5Aget_type(attr);
  if ( H5Tget_class(type) != H5T_INTEGER ) throw -1;
  hid_t nativeType = H5Tget_native_type(type, H5T_DIR_ASCEND);
  hsize_t attrSize = H5Tget_size(type);
  switch (attrSize) {
    case(sizeof(int8_t)): {
      int8_t i=0;
      if ( H5Aread(attr, nativeType, &i ) < 0 ) throw -1;
      value = static_cast<int>(i);
      break;
    }
    case(sizeof(int16_t)): {
      int16_t i=0;
      if ( H5Aread(attr, nativeType, &i ) < 0 ) throw -1;
      value = static_cast<int>(i);
      break;
    }
    case(sizeof(int32_t)): {
      int32_t i=0;
      if ( H5Aread(attr, nativeType, &i ) < 0 ) throw -1;
      value = static_cast<int>(i);
      break;
    }
    case(sizeof(int64_t)): {
      int64_t i=0;
      if ( H5Aread(attr, nativeType, &i ) < 0 ) throw -1;
      value = static_cast<int>(i);
      break;
    }
    default:
      throw -1;
      break;
  }
  H5Tclose(nativeType);
  H5Tclose(type);
  H5Aclose(attr);
}

void MyHdf5Unit::getAttribute(const std::string& name, double &value) {
  hid_t attr = H5Aopen(id_, name.c_str(), H5P_DEFAULT);
  if ( attr < 0 ) throw -1;
  hid_t type = H5Aget_type(attr);
  if ( H5Tget_class(type) != H5T_FLOAT && H5Tget_class(type) != H5T_INTEGER ) throw -1;
  hid_t nativeType = H5Tget_native_type(type, H5T_DIR_ASCEND);
  hsize_t attrSize = H5Tget_size(type);
  if ( H5Tget_class(type) == H5T_FLOAT ) {
    switch (attrSize) {
      case(sizeof(float)): {
        float d=0.0;
        if ( H5Aread(attr, nativeType, &d ) < 0 ) throw -1;
        value = static_cast<double>(d);
        break;
      }
      case(sizeof(double)): {
        double d=0.0;
        if ( H5Aread(attr, nativeType, &d ) < 0 ) throw -1;
        value = static_cast<double>(d);
        break;
      }
      default:
        throw -1;
        break;
    }
  }
  else {
    switch (attrSize) {
      case(sizeof(int8_t)): {
        int8_t i=0;
        if ( H5Aread(attr, nativeType, &i ) < 0 ) throw -1;
        value = static_cast<double>(i);
        break;
      }
      case(sizeof(int16_t)): {
        int16_t i=0;
        if ( H5Aread(attr, nativeType, &i ) < 0 ) throw -1;
        value = static_cast<double>(i);
        break;
      }
      case(sizeof(int32_t)): {
        int32_t i=0;
        if ( H5Aread(attr, nativeType, &i ) < 0 ) throw -1;
        value = static_cast<double>(i);
        break;
      }
      case(sizeof(int64_t)): {
        int64_t i=0;
        if ( H5Aread(attr, nativeType, &i ) < 0 ) throw -1;
        value = static_cast<double>(i);
        break;
      }
      default:
        throw -1;
        break;
    }
  }
  H5Tclose(nativeType);
  H5Tclose(type);
  H5Aclose(attr);
}

void MyHdf5Unit::getAttribute(const std::string& name, float*& buffer, int& bufferSize) {
  hid_t attr = H5Aopen(id_, name.c_str(), H5P_DEFAULT);
  if ( attr < 0 ) throw -1;
  hid_t type = H5Aget_type(attr);
  if ( H5Tget_class(type) != H5T_FLOAT ) throw -1;
  hid_t space = H5Aget_space(attr);
  int ndims = H5Sget_simple_extent_ndims(space);
  hsize_t* dSize = new hsize_t[ndims];
  H5Sget_simple_extent_dims(space, dSize, NULL);
  bufferSize = 0;
  for (int i=0; i<ndims; i++) {
    bufferSize += static_cast<int>(dSize[i]);
  }
  delete[] dSize;
  
  buffer = 0;
  buffer = new float[bufferSize];
  if ( !buffer ) throw -1;
  
  if ( H5Aread(attr, H5T_NATIVE_FLOAT, buffer) < 0 ) throw -1;
  
  H5Sclose(space);
  H5Tclose(type);
  H5Aclose(attr);
}

void MyHdf5Unit::getDatasetDims(int &numberOfDims, int *&dimSize) {
  if ( type_ != DATASET_ ) throw -1;
  if ( dimSize ) {
    delete[] dimSize;
    dimSize = 0;
  }
  hid_t space = H5Dget_space(id_);
  int ndims = H5Sget_simple_extent_ndims(space);
  if ( ndims <= 0 ) throw -1;
  hsize_t *dSize = new hsize_t[ndims];
  H5Sget_simple_extent_dims(space, dSize, NULL);
  H5Sclose(space);
  numberOfDims = ndims;
  dimSize = new int[numberOfDims];
  if ( !dimSize ) throw -1;
  for (int i=0; i<ndims; i++) {
    dimSize[i] = static_cast<int>(dSize[i]);
  }
  delete[] dSize;
}

void MyHdf5Unit::getDataset(unsigned char *&buffer, int &bufferSize) {
  if ( type_ != DATASET_ ) throw -1;
  hid_t type = H5Dget_type(id_); 
  if ( type < 0 ) throw -1;
  if ( H5Tget_class(type) != H5T_INTEGER ) throw -1;
  hid_t space = H5Dget_space(id_);
  int ndims = 0;
  int *dimSize = 0;
  getDatasetDims(ndims, dimSize);
  bufferSize = 1;
  for (int i=0; i<ndims; i++) {
    bufferSize = bufferSize * dimSize[i];
  }
  delete[] dimSize;
  hsize_t size = H5Tget_size(type);
  unsigned char *ubuf = 0;
  ubuf = new unsigned char[bufferSize];
  if ( !ubuf ) throw -1;
  switch (size) {
    case(sizeof(uint8_t)): {
      uint8_t *buf = 0;
      buf = new uint8_t[bufferSize];
      if ( !buf ) throw -1;
      if ( H5Dread(id_, type, space, space, H5P_DEFAULT, buf) < 0 ) throw -1;
      for (int i=0; i<bufferSize; i++) {
        ubuf[i] = static_cast<unsigned char>(buf[i]);
      }
      delete[] buf;
      buf = 0;
      break;
    }
    case(sizeof(uint16_t)): {
      uint16_t *buf = 0;
      buf = new uint16_t[bufferSize];
      if ( !buf ) throw -1;
      if ( H5Dread(id_, type, space, space, H5P_DEFAULT, buf) < 0 ) throw -1;
      for (int i=0; i<bufferSize; i++) {
        ubuf[i] = static_cast<unsigned char>(buf[i]);
      }
      delete[] buf;
      buf = 0;
      break;
    }
    case(sizeof(uint32_t)): {
      uint32_t *buf = 0;
      buf = new uint32_t[bufferSize];
      if ( !buf ) throw -1;
      if ( H5Dread(id_, type, space, space, H5P_DEFAULT, buf) < 0 ) throw -1;
      for (int i=0; i<bufferSize; i++) {
        ubuf[i] = static_cast<unsigned char>(buf[i]);
      }
      delete[] buf;
      buf = 0;
      break;
    }
    case(sizeof(uint64_t)): {
      uint64_t *buf = 0;
      buf = new uint64_t[bufferSize];
      if ( !buf ) throw -1;
      if ( H5Dread(id_, type, space, space, H5P_DEFAULT, buf) < 0 ) throw -1;
      for (int i=0; i<bufferSize; i++) {
        ubuf[i] = static_cast<unsigned char>(buf[i]);
      }
      delete[] buf;
      buf = 0;
      break;
    }
    default:
      throw -1;
      break;
  }
  if ( buffer ) {
    delete[] buffer;
    buffer = 0;
  }
  buffer = ubuf;
  H5Tclose(type);
  H5Sclose(space);
}

void MyHdf5Unit::getDataset(const std::string& name, unsigned char *&buffer, int &bufferSize) {
  MyHdf5Unit dataset;
  dataset.open(*this, name);
  dataset.getDataset(buffer, bufferSize);
  dataset.close();
}

void MyHdf5Unit::getDataset(unsigned short *&buffer, int &bufferSize) {
  if ( type_ != DATASET_ ) throw -1;
  hid_t type = H5Dget_type(id_); 
  if ( type < 0 ) throw -1;
  if ( H5Tget_class(type) != H5T_INTEGER ) throw -1;
  hid_t space = H5Dget_space(id_);
  int ndims = 0;
  int *dimSize = 0;
  getDatasetDims(ndims, dimSize);
  bufferSize = 1;
  for (int i=0; i<ndims; i++) {
    bufferSize = bufferSize * dimSize[i];
  }
  delete[] dimSize;
  hsize_t size = H5Tget_size(type);
  unsigned short *ubuf = 0;
  ubuf = new unsigned short[bufferSize];
  if ( !ubuf ) throw -1;
  switch (size) {
    case(sizeof(uint8_t)): {
      uint8_t *buf = 0;
      buf = new uint8_t[bufferSize];
      if ( !buf ) throw -1;
      if ( H5Dread(id_, type, space, space, H5P_DEFAULT, buf) < 0 ) {
        printf("dbg - H5Dread error \n");
        throw -1;
      }
      for (int i=0; i<bufferSize; i++) {
        ubuf[i] = static_cast<unsigned short>(buf[i]);
      }
      delete[] buf;
      buf = 0;
      break;
    }
    case(sizeof(uint16_t)): {
      uint16_t *buf = 0;
      buf = new uint16_t[bufferSize];
      if ( !buf ) throw -1;
      if ( H5Dread(id_, type, space, space, H5P_DEFAULT, buf) < 0 ) throw -1;
      for (int i=0; i<bufferSize; i++) {
        ubuf[i] = static_cast<unsigned short>(buf[i]);
      }
      delete[] buf;
      buf = 0;
      break;
    }
    case(sizeof(uint32_t)): {
      uint32_t *buf = 0;
      buf = new uint32_t[bufferSize];
      if ( !buf ) throw -1;
      if ( H5Dread(id_, type, space, space, H5P_DEFAULT, buf) < 0 ) throw -1;
      for (int i=0; i<bufferSize; i++) {
        ubuf[i] = static_cast<unsigned short>(buf[i]);
      }
      delete[] buf;
      buf = 0;
      break;
    }
    case(sizeof(uint64_t)): {
      uint64_t *buf = 0;
      buf = new uint64_t[bufferSize];
      if ( !buf ) throw -1;
      if ( H5Dread(id_, type, space, space, H5P_DEFAULT, buf) < 0 ) throw -1;
      for (int i=0; i<bufferSize; i++) {
        ubuf[i] = static_cast<unsigned short>(buf[i]);
      }
      delete[] buf;
      buf = 0;
      break;
    }
    default:
      throw -1;
      break;
  }
  if ( buffer ) {
    delete[] buffer;
    buffer = 0;
  }
  buffer = ubuf;
  H5Tclose(type);
  H5Sclose(space);
}

void MyHdf5Unit::getDataset(const std::string& name, unsigned short *&buffer, int &bufferSize) {
  printf("dbg - getDataset\n");
  MyHdf5Unit dataset;
  printf("dbg - open\n");
  dataset.open(*this, name);
  dataset.getDataset(buffer, bufferSize);
  dataset.close();
}

void MyHdf5Unit::getDataset(float *&buffer, int &bufferSize) {
  if ( type_ != DATASET_ ) throw -1;
  hid_t type = H5Dget_type(id_); 
  if ( type < 0 ) throw -1;
  if ( H5Tget_class(type) != H5T_FLOAT ) throw -1;
  hid_t space = H5Dget_space(id_);
  int ndims = 0;
  int *dimSize = 0;
  getDatasetDims(ndims, dimSize);
  bufferSize = 1;
  for (int i=0; i<ndims; i++) {
    bufferSize = bufferSize * dimSize[i];
  }
  delete[] dimSize;
  hsize_t size = H5Tget_size(type);
  float* floatBuf = 0;
  floatBuf = new float[bufferSize];
  if ( !floatBuf ) throw -1;
  
  switch (size) {
    case(sizeof(float)): {
      float *buf = 0;
      buf = new float[bufferSize];
      if ( !buf ) throw -1;
      if ( H5Dread(id_, H5T_NATIVE_FLOAT, space, space, H5P_DEFAULT, buf) < 0 ) {
        throw -1;
      }
      for (int i=0; i<bufferSize; i++) {
        floatBuf[i] = static_cast<float>(buf[i]);
      }
      delete[] buf;
      buf = 0;
      break;
    }
    case(sizeof(double)): {
      double *buf = 0;
      buf = new double[bufferSize];
      if ( !buf ) throw -1;
      if ( H5Dread(id_, type, space, space, H5P_DEFAULT, buf) < 0 ) {
        throw -1;
      }
      for (int i=0; i<bufferSize; i++) {
        floatBuf[i] = static_cast<float>(buf[i]);
      }
      delete[] buf;
      buf = 0;
      break;
    }
    default:
      throw -1;
      break;
  }
  
  //TODO:: is it OK?
  //if ( buffer ) {
  //  delete[] buffer;
  //  buffer = 0;
  //}
  buffer = floatBuf;
  H5Tclose(type);
  H5Sclose(space);
}

void MyHdf5Unit::getDataset(const std::string& name, float*& buffer, int& bufferSize) {
  MyHdf5Unit dataset;
  dataset.open(*this, name);
  dataset.getDataset(buffer, bufferSize);
  dataset.close();
}

void MyHdf5Unit::getDataset(double *&buffer, int &bufferSize) {
  if ( type_ != DATASET_ ) throw -1;
  hid_t type = H5Dget_type(id_); 
  if ( type < 0 ) throw -1;
  if ( H5Tget_class(type) != H5T_FLOAT ) throw -1;
  hid_t space = H5Dget_space(id_);
  int ndims = 0;
  int *dimSize = 0;
  getDatasetDims(ndims, dimSize);
  bufferSize = 1;
  for (int i=0; i<ndims; i++) {
    bufferSize = bufferSize * dimSize[i];
  }
  delete[] dimSize;
  hsize_t size = H5Tget_size(type);
  double* dBuf = 0;
  dBuf = new double[bufferSize];
  if ( !dBuf ) throw -1;
  
  switch (size) {
    case(sizeof(float)): {
      float *buf = 0;
      buf = new float[bufferSize];
      if ( !buf ) throw -1;
      if ( H5Dread(id_, H5T_NATIVE_FLOAT, space, space, H5P_DEFAULT, buf) < 0 ) {
        throw -1;
      }
      for (int i=0; i<bufferSize; i++) {
        dBuf[i] = static_cast<double>(buf[i]);
      }
      delete[] buf;
      buf = 0;
      break;
    }
    case(sizeof(double)): {
      double *buf = 0;
      buf = new double[bufferSize];
      if ( !buf ) throw -1;
      if ( H5Dread(id_, H5T_NATIVE_DOUBLE, space, space, H5P_DEFAULT, buf) < 0 ) {
        throw -1;
      }
      for (int i=0; i<bufferSize; i++) {
        dBuf[i] = buf[i];
      }
      delete[] buf;
      buf = 0;
      break;
    }
    default:
      throw -1;
      break;
  }
  
  //TODO:: is it OK?
  //if ( buffer ) {
  //  delete[] buffer;
  //  buffer = 0;
  //}
  buffer = dBuf;
  H5Tclose(type);
  H5Sclose(space);
}

void MyHdf5Unit::getDataset(const std::string& name, double*& buffer, int& bufferSize) {
  MyHdf5Unit dataset;
  dataset.open(*this, name);
  dataset.getDataset(buffer, bufferSize);
  dataset.close();
}

void MyHdf5Unit::create(const std::string& fileName) {
  openFile(fileName, WRITE_);
}

void MyHdf5Unit::createGroup(MyHdf5Unit &group, const std::string& groupName) {
  if ( !isOpen() ) throw -1;
  group.close();
  group.id_ = H5Gcreate2(id_, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  group.checkId_();
  group.type_ = GROUP_;
}

void MyHdf5Unit::setDataset(const std::string& datasetName, const unsigned char *buffer, 
                            const int numberOfDims, const int *dimSize, const int compression) {
  if ( !isOpen() ) throw -1; 
  hid_t datatype = H5Tcopy(H5T_STD_U8LE);  
  hsize_t *dimSizeLocal = new hsize_t[numberOfDims];
  if ( !dimSizeLocal ) throw -1;
  for (int i=0; i<numberOfDims; i++) {
    dimSizeLocal[i] = static_cast<hsize_t>(dimSize[i]);
  }
  const hsize_t *maxDimSize = NULL;
  hid_t dataspace = H5Screate_simple(numberOfDims, dimSizeLocal, maxDimSize);
  hid_t dataprop = H5Pcreate(H5P_DATASET_CREATE);
  H5Pset_chunk(dataprop, numberOfDims, dimSizeLocal);
  H5Pset_deflate(dataprop, compression);
  delete[] dimSizeLocal;
  hid_t dataset = H5Dcreate2(id_, datasetName.c_str(), datatype, dataspace, 
                             H5P_DEFAULT, dataprop, H5P_DEFAULT);
  H5Dwrite(dataset, H5T_STD_U8LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer);
  
  H5Tclose(datatype);
  H5Sclose(dataspace);
  H5Dclose(dataset);
}

void MyHdf5Unit::setDataset(const std::string& datasetName, const double* buffer, 
                            const int numberOfDims, const int *dimSize, const int compression) {
  if ( !isOpen() ) throw -1; 
  hid_t datatype = H5Tcopy(H5T_NATIVE_DOUBLE);  
  hsize_t *dimSizeLocal = new hsize_t[numberOfDims];
  if ( !dimSizeLocal ) throw -1;
  for (int i=0; i<numberOfDims; i++) {
    dimSizeLocal[i] = static_cast<hsize_t>(dimSize[i]);
  }
  const hsize_t *maxDimSize = NULL;
  hid_t dataspace = H5Screate_simple(numberOfDims, dimSizeLocal, maxDimSize);
  hid_t dataprop = H5Pcreate(H5P_DATASET_CREATE);
  H5Pset_chunk(dataprop, numberOfDims, dimSizeLocal);
  H5Pset_deflate(dataprop, compression);
  delete[] dimSizeLocal;
  hid_t dataset = H5Dcreate2(id_, datasetName.c_str(), datatype, dataspace, 
                             H5P_DEFAULT, dataprop, H5P_DEFAULT);
  H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer);
  
  H5Tclose(datatype);
  H5Sclose(dataspace);
  H5Dclose(dataset);
}

void MyHdf5Unit::setAttribute(const std::string& attrName, const double value) {
  if ( !isOpen() ) throw -1;
  hid_t       attrspace = H5Screate(H5S_SCALAR);
  hid_t       attr = H5Acreate2(id_, attrName.c_str(), H5T_IEEE_F64LE, attrspace, 
                                H5P_DEFAULT, H5P_DEFAULT);

  H5Awrite(attr, H5T_NATIVE_DOUBLE, &value);

  H5Sclose(attrspace);
  H5Aclose(attr);
}

void MyHdf5Unit::setAttribute(const std::string& attrName, const int value) {
  if ( !isOpen() ) throw -1;
  hid_t       attrspace = H5Screate(H5S_SCALAR);
  hid_t       attr = H5Acreate2(id_, attrName.c_str(), H5T_STD_I32LE, attrspace,
                                H5P_DEFAULT, H5P_DEFAULT);

  H5Awrite(attr, H5T_NATIVE_INT, &value);

  H5Sclose(attrspace);
  H5Aclose(attr);
}

void MyHdf5Unit::setAttribute(const std::string& attrName, const long long value) {
  if ( !isOpen() ) throw -1;
  hid_t       attrspace = H5Screate(H5S_SCALAR);
  hid_t       attr = H5Acreate2(id_, attrName.c_str(), H5T_STD_I64LE, attrspace, 
                                H5P_DEFAULT, H5P_DEFAULT);

  H5Awrite(attr, H5T_NATIVE_LLONG, &value);

  H5Sclose(attrspace);
  H5Aclose(attr);
}

void MyHdf5Unit::setAttribute(const std::string& attrName, const std::string& value, const int size) {
  if ( !isOpen() ) throw -1;
  hid_t attrspace  = H5Screate(H5S_SCALAR);
  hid_t attrtype = H5Tcopy(H5T_C_S1);
  H5Tset_size(attrtype, size);
  H5Tset_strpad(attrtype,H5T_STR_NULLTERM);
  hid_t attr = H5Acreate2(id_, attrName.c_str(), attrtype, attrspace, 
                          H5P_DEFAULT, H5P_DEFAULT);

  H5Awrite(attr, attrtype, value.c_str());
  
  H5Sclose(attrspace);
  H5Tclose(attrtype);
  H5Aclose(attr);
}

void MyHdf5Unit::getName(char *&name, size_t &nameSize) {
  if ( name ) {
    delete[] name;
    name = 0;
  }
  nameSize = H5Iget_name(id_, name, 0) + 1;
  name = new char[nameSize];
  H5Iget_name(id_, name, nameSize);
}

bool MyHdf5Unit::isDouble() const {
  if ( type_ != DATASET_ ) return false;
  hid_t type = H5Dget_type(id_); 
  if ( type < 0 ) throw -1;
  if ( H5Tget_class(type) != H5T_FLOAT ) return false;
  else return true;
}

char *get_variable_string_attribute(hid_t attrID) {
  hid_t       attrtype, type;
  char *string_attr;
  type = H5Tcopy (H5T_C_S1);
  H5Tset_size(type, H5T_VARIABLE);
  attrtype = H5Aget_type(attrID);
  type = H5Tget_native_type(attrtype, H5T_DIR_ASCEND);
  
  if ( H5Aread( attrID, type, &string_attr ) < 0 ) throw -1;
  
  return string_attr;
}


