// class_OdimEntry.hpp
// class to work with ODIM specification entries
// Ladislav Meri, SHMU
// v_0.0, 04.2018

#ifndef CLASS_ODIMENTRY_HPP
#define CLASS_ODIMENTRY_HPP

#include <string>

namespace myodim {

class OdimEntry {
  public:
    enum Category { Group, Attribute, Dataset };
    enum Type { String, Real, Integer,
                StringArray, RealArray, IntegerArray,
                StringArray2D, RealArray2D, IntegerArray2D,
                Link, Undefined };
    
    std::string node{""};
    Category    category{Group};
    Type        type{String};
    bool        isMandatory{false};
    std::string possibleValues{""};
    std::string reference{""};
    
    OdimEntry() = default;
    OdimEntry(const std::string nodeStr, const std::string categoryStr, const std::string typeStr, 
              const std::string isMandatoryStr, const std::string possibleValuesStr, const std::string reference);
    void set(const std::string nodeStr, const std::string categoryStr, const std::string typeStr, 
             const std::string isMandatoryStr, const std::string possibleValuesStr, const std::string reference);
    bool isGroup() const {return category == Group;}
    bool isAttribute() const {return category == Attribute;}
    bool isDataset() const {return category == Dataset;}
    std::string categoryToString() const;
    std::string typeToString() const;
    
  
  private:
    void parseCategory_(const std::string categoryStr);
    void parseType_(const std::string typeStr);
    void parseIsMandatory_(const std::string isMandatoryStr) ;
};

} //end namespace myodim

#endif // CLASS_ODIMENTRY_HPP
