// class_H5Layout.hpp
// class to map the hdf5 file content
// Ladislav Meri, SHMU

#ifndef CLASS_H5LAYOUT_HPP
#define CLASS_H5LAYOUT_HPP

#include <vector>
#include <string>
#include <utility>
#include <hdf5.h>
#include <stdint.h>

namespace myodim {

struct h5Entry: public std::pair<std::string, bool>  {   // need to use to save wether it was checked or not, it speeds up the checking of extra features a lot
  public:
    h5Entry() = default;
    h5Entry(const std::string& str, const bool b): std::pair<std::string, bool>(str,b) {};
    std::string& name() {return first;};
    std::string name() const {return first;};
    bool& wasFound() {return second;};
    bool wasFound() const {return second;};
};

class H5Layout {
  public:
    std::vector<h5Entry> groups;
    std::vector<h5Entry> datasets;
    std::vector<h5Entry> attributes;
    
    H5Layout();
    H5Layout(const std::string& h5FilePath);
    ~H5Layout();
    
    void explore(const std::string& h5FilePath);
    bool hasAttribute(const std::string& attrName) const;
    bool hasGroup(const std::string& groupName) const;
    bool hasDataset(const std::string& dsetName) const;
    std::string filePath() const;
    std::vector<std::string> getAttributeNames(const std::string& objPath) const;
    void getAttributeValue(const std::string& attrName, std::string& value) const;
    void getAttributeValue(const std::string& attrName, double& value) const;
    void getAttributeValue(const std::string& attrName, int64_t& value) const;
    void getAttributeValue(const std::string& attrName, std::vector<double>& values) const;
    void getAttributeValue(const std::string& attrName, std::vector<int64_t>& values) const;
    bool isStringAttribute(const std::string& attrName) const;
    bool isFixedLenghtStringAttribute(const std::string& attrName) const;
    bool isReal64Attribute(const std::string& attrName) const;
    bool isInt64Attribute(const std::string& attrName) const;
    bool isBooleanAttribute(const std::string& attrName) const;
    bool isUcharDataset(const std::string& dsetName) const;
    bool is1DArrayAttribute(const std::string& attrName) const;
    void attributeStatistics(const std::string& attrName, double& first, double& last,
                             double& min, double& max, double& mean) const;
    void attributeStatistics(const std::string& attrName, int64_t& first, int64_t& last,
                             int64_t& min, int64_t& max, int64_t& mean) const;
    bool ucharDatasetHasImageAttributes(const std::string& dsetPath) const;
    
  private:
    std::string h5FilePath_{""};
    hid_t h5FileID_{-1};
    void checkAndOpenFile_(const std::string& h5FilePath);
    void findGroupsAndDatasets_();
    void findAttributes_();
    void reset_();
};


} // end namespace myh5

#endif // CLASS_H5LAYOUT_HPP
