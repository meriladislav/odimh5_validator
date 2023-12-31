// module_Compare.hpp
// functions to compare the hdf5 layout with the ODIM specification
// Ladislav Meri, SHMU
// v_0.0, 04.2018

#ifndef MODULE_COMPARE_HPP
#define MODULE_COMPARE_HPP

#include <string>
#include "class_H5Layout.hpp"
#include "class_OdimStandard.hpp"

namespace myodim {

extern bool printInfo;

extern std::string getCsvFileNameFrom(const myodim::H5Layout& h5layout);
extern std::string getCsvFileNameFrom(const myodim::H5Layout& h5layout, std::string version);
extern bool compare(myodim::H5Layout& h5layout, const OdimStandard& odimStandard, 
                    const bool checkOptional=false, const bool checkExtras=false,
                    OdimStandard* failedEntries=nullptr);
extern bool isStringValue(const std::string& value);
extern bool hasDoublePoint(const std::string& value);
extern bool checkValue(const std::string& attrValue, const std::string& assumedValueStr,
                       std::string& errorMessage);
extern bool checkValue(const double attrValue, const std::string& assumedValueStr,
                       std::string& errorMessage, const bool isReal=false);
extern bool checkValue(const std::vector<double>& attrValues, const std::string& assumedValueStr,
                       std::string& errorMessage);
extern bool checkWhatSource(const std::string& whatSource, const std::string& basicRegex,
                            std::string& errorMessage);


}

#endif // MODULE_COMPARE_HPP

