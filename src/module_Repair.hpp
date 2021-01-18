// module_Reapir.hpp
// functions to reapir, fix or change the ODIM-H5 files
// Ladislav Meri, SHMU
// v_0.0, 01.2020

#ifndef MODULE_REPAIR_HPP
#define MODULE_REPAIR_HPP

#include <string>
#include "class_OdimStandard.hpp"

namespace myodim {

extern void copyFile(const std::string& sourceFile, const std::string& copyFile);

} // end myodim

#endif //MODULE_REPAIR_HPP
