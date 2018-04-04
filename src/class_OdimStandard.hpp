// class_OdimStandard.hpp
// class to work with ODIM specification
// Ladislav Meri, SHMU
// v_0.0, 04.2018

#ifndef CLASS_ODIMSTANDARD_HPP
#define CLASS_ODIMSTANDARD_HPP

#include <vector>
#include <string>
#include "class_OdimEntry.hpp"

namespace myodim {

class OdimStandard {
  public:
    std::vector<OdimEntry> entries;
    OdimStandard() = default;
    OdimStandard(const std::string& csvFilePath);
    void readFromCsv(const std::string& csvFilePath);
    
  private:
    
};

} //end namespace myodim

#endif // CLASS_ODIMSTANDARD_HPP
