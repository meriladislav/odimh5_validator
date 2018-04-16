// class_H5Explorer.hpp
// class to explore the hdf5 file content
// Ladislav Meri, SHMU
// v_0.0, 04.2018

#ifndef CLASS_H5EXPLORER_HPP
#define CLASS_H5EXPLORER_HPP

#include <vector>
#include <string>
#include <utility>

namespace myh5 {

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
