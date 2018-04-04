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

extern std::string getCsvFileNameFrom(const myh5::H5Layout& h5layout);
extern bool compare(const myh5::H5Layout& h5layout, const OdimStandard& odimStandard);

}

#endif // MODULE_COMPARE_HPP
