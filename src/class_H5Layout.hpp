// class_H5Explorer.hpp
// class to explore the hdf5 file content
// Ladislav Meri, SHMU
// v_0.0, 04.2018

#ifndef CLASS_H5EXPLORER_HPP
#define CLASS_H5EXPLORER_HPP

#include <vector>
#include <string>

namespace myh5 {

class H5Layout {
  public:
    std::vector<std::string> groups;
    std::vector<std::string> datasets;
    std::vector<std::string> attributes;
    
    H5Layout() = default;
    H5Layout(const std::string& h5FilePath);
    ~H5Layout();
    
    void explore(const std::string& h5FilePath);
    bool hasAttribute(const std::string& attrName) const;
    std::string filePath() const;
    void getAttributeValue(const std::string& attrName, std::string& value) const;
    bool isStringAttribute(const std::string& attrName) const;
    bool isReal64Attribute(const std::string& attrName) const;
    bool isInt64Attribute(const std::string& attrName) const;
    bool isBooleanAttribute(const std::string& attrName) const;
    
  private:
    std::string h5FilePath_{""};
    int h5FileID_{-1};
    void checkAndOpenFile_(const std::string& h5FilePath);
    void findGroupsAndDatasets_();
    void findAttributes_();
    void reset_();
};


} // end namespace myh5

#endif // CLASS_H5EXPLORER_HPP
