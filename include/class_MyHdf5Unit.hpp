// class_MyHdf5Unit.hpp
// class to work with the hdf5 files, groups and datasets (by hidding the hdf5 API)
// Ladislav Meri, SHMU
// v_0.0, 01.2014

#ifndef CLASS_MYHDF5UNIT_HPP
#define CLASS_MYHDF5UNIT_HPP

#include <string>
#include "hdf5.h"

using namespace std;

enum Hdf5UnitType {
  UNKNOWN_,
  FILE_,
  GROUP_,
  DATASET_
};

enum Hdf5UnitAccess {
  READ_,
  WRITE_,
  READWRITE_
};

//TODO:: replace char * with sdt::string&
class MyHdf5Unit {
 public:
  MyHdf5Unit();
  ~MyHdf5Unit();
  
  unsigned int id() const;
  Hdf5UnitType type() const;
  bool isOpen() const;
  void openFile(const std::string& fileName, Hdf5UnitAccess access=READ_);
  void closeFile();
  void openGroup(const MyHdf5Unit &sourceUnit, const std::string& groupName);
  void closeGroup();
  void openDataset(const MyHdf5Unit &sourceUnit, const std::string& datasetName);
  void closeDataset();
  void open(const std::string& fileName, Hdf5UnitAccess access=READ_);
  void open(const MyHdf5Unit &sourceUnit, const std::string& name);
  void close();
  int numberOfLinksInGroup();
  int linkNameSizeByIndex(int index);
  std::string linkNameByIndex(int index, int &nameSize);
  int numberOfAttributes();
  bool hasAttribute(const std::string& name);
  void getAttribute(const std::string& name, char *&value, int &valueSize);
  void getAttribute(const std::string& name, int &value);
  void getAttribute(const std::string& name, double &value);
  void getAttribute(const std::string& name, float*& buffer, int& bufferSize);
  void getDatasetDims(int &numberOfDims, int *&dimSize);
  void getDataset(unsigned char *&buffer, int &bufferSize);
  void getDataset(const std::string& name, unsigned char *&buffer, int &bufferSize);
  void getDataset(unsigned short *&buffer, int &bufferSize);
  void getDataset(const std::string& name, unsigned short *&buffer, int &bufferSize);
  void getDataset(float*& buffer, int& bufferSize);
  void getDataset(const std::string& name, float*& buffer, int& bufferSize);
  void getDataset(double*& buffer, int& bufferSize);
  void getDataset(const std::string& name, double*& buffer, int& bufferSize);
  void create(const std::string& fileName);
  void createGroup(MyHdf5Unit &group, const std::string& groupName);
  void setDataset(const std::string& datasetName, const unsigned char *buffer, 
                  const int numberOfDims, const int *dimSize, const int compression=6);
  void setDataset(const std::string& datasetName, const double* buffer, 
                  const int numberOfDims, const int *dimSize, const int compression=6);
  // TODO:: steDataset for unsigned short, ...
  void setAttribute(const std::string& attrName, const double value);
  void setAttribute(const std::string& attrName, const int value);
  void setAttribute(const std::string& attrName, const long long value);
  void setAttribute(const std::string& attrName, const std::string& value, const int size);
  void getName(char *&name, size_t &nameSize);
  bool isDouble() const;
                            
 private:
  hid_t id_;
  Hdf5UnitType type_;
  void checkId_();
  void reset_();
  
};

#endif
