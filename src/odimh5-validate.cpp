#include <cstdio> 
#include <iostream>
#include <string>
#include <stdexcept>
#include "csv.h"
#include "class_MyHdf5Unit.hpp"

#include "class_H5Layout.hpp"

const std::string USAGE{"usage: odimh5-validate h5File standardDefinition.csv"};
const int MIN_ARG_NUM{3};

const int CSV_COL_NUM{5};
const char CSV_SEPARATOR{';'};


class OdimEntry {
  public:
    enum Category { Group, Attribute, Dataset };
    enum Type { string, real, integer, undefined };
    
    std::string node;
    Category    category;
    Type        type;
    bool        isMandatory{false};
    std::string possibleValues;
    
    OdimEntry() = default;
    OdimEntry(const std::string nodeStr, const std::string categoryStr, const std::string typeStr, 
              const std::string isMandatoryStr, const std::string possibleValuesStr){ 
      set(nodeStr, categoryStr, typeStr, isMandatoryStr, possibleValuesStr);
    }
    void set(const std::string nodeStr, const std::string categoryStr, const std::string typeStr, 
              const std::string isMandatoryStr, const std::string possibleValuesStr) {
      node = nodeStr;
      parseCategory_(categoryStr);
      parseType_(typeStr);
      parseIsMandatory_(isMandatoryStr);
      possibleValues = possibleValuesStr;
    }
    bool isGroup() const {return category == Group;}
    bool isAttribute() const {return category == Attribute;}
    bool isDataset() const {return category == Dataset;}
    
  
  private:
    void parseCategory_(const std::string categoryStr) {
      if ( categoryStr == "Group" ) category = Category::Group;
      else if ( categoryStr == "Attribute" ) category = Category::Attribute;
      else if ( categoryStr == "Dataset" ) category = Category::Dataset;
      else throw std::invalid_argument{"Unknown category - "+categoryStr};
    }
    void parseType_(const std::string typeStr) {
      if ( isGroup() || isDataset() ) {
        type = undefined;
        return;
      }
      if ( typeStr == "string" ) type = Type::string;
      else if ( typeStr == "real" ) type = Type::real;
      else if ( typeStr == "integer" ) type = Type::integer;
      else throw std::invalid_argument{"Unknown type - "+typeStr};
    }
    void parseIsMandatory_(const std::string isMandatoryStr) {
      isMandatory = isMandatoryStr == "TRUE" ? true : false;
    }
};

bool entryIsPresentInH5Group(const OdimEntry& entry, MyHdf5Unit& h5Group) {
  
  //std::cout << h5Group.numberOfLinksInGroup() << std::endl;
  return false;
}

/************************************************************

  Operator function for H5Ovisit.  This function prints the
  name and type of the object passed to it.

 ************************************************************/
herr_t op_func (hid_t loc_id, const char *name, const H5O_info_t *info,
            void *operator_data)
{
    printf ("/");               /* Print root group in object path */

    /*
     * Check if the current object is the root group, and if not print
     * the full path name and type.
     */
    if (name[0] == '.')         /* Root group, do not print '.' */
        printf ("  (Group)\n");
    else
        switch (info->type) {
            case H5O_TYPE_GROUP:
                printf ("%s  (Group)\n", name);
                break;
            case H5O_TYPE_DATASET:
                printf ("%s  (Dataset)\n", name);
                break;
            case H5O_TYPE_NAMED_DATATYPE:
                printf ("%s  (Datatype)\n", name);
                break;
            default:
                printf ("%s  (Unknown)\n", name);
        }

    return 0;
}

int main(int argc, char* argv[]) {
  setbuf(stdout, NULL);
  
  if ( argc < MIN_ARG_NUM ) {
    std::cout << USAGE << std::endl;
    return -1;
  }
  
  std::string h5File{argv[1]};
  std::string csvFile{argv[2]};
  
  myh5::H5Layout h5layout(h5File);
  std::cout << "dbg - " << h5layout.groups.size() << " groups, " << h5layout.datasets.size() <<
               " datasets and " << h5layout.attributes.size() << " attributes found" << std::endl;
  for (const auto& g : h5layout.groups) std::cout << g << " - group" << std::endl;
  for (const auto& d : h5layout.datasets) std::cout << d << " - dataset" << std::endl;
  for (const auto& a : h5layout.attributes) std::cout << a << " - attribute" << std::endl;
  
  
  /*
  io::CSVReader<CSV_COL_NUM, io::trim_chars<CSV_SEPARATOR>, io::no_quote_escape<CSV_SEPARATOR>> csv(csvFile);
  csv.read_header(io::ignore_no_column, 
                  "Node", "Category", "Type", "IsMandatory", "PossibleValues");
  
  MyHdf5Unit h5;
  h5.openFile(h5File);
  H5Ovisit (h5.id(), H5_INDEX_NAME, H5_ITER_NATIVE, op_func, NULL);
  MyHdf5Unit h5root;
  h5root.openGroup(h5, "/");
  
  OdimEntry entry;
  std::string node, category, type, isMandatory, possibleValues;
  while(csv.read_row(node, category, type, isMandatory, possibleValues)){
    entry.set(node, category, type, isMandatory, possibleValues);
    bool isPresent = entryIsPresentInH5Group(entry, h5root);
    //std::cout << "dbg - " << entry.node << " " << isPresent << std::endl;
  }
  
  h5.close();
  */
  
  return 0;
}
