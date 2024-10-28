# README #

This documentation describes the installation and usage of the odimh5-validator software.

### What is this repository for? ###

The odimh5-validator is a command-line tool to check whether a given file is compliant with the EUMETNET-OPERA ODIM-H5 format standard
(OPERA Data Information Model for HDF5). The validator is currently supporting polar volume (PVOL), polar scan (SCAN) and cartesian 2D (COMP, IMAGE) objects.


### How do I get set up? ###

#### Dependencies ####
- C++11 capable compiler (e.g. g++ (GCC) 4.9, or higher)
- the basic hdf5 library installed (it`s enough to have just the C library installed), together with the h5cc tool

#### Build & Setup ####
After cloning this repository, just go to the odimh5_validator directory and build with make:

    $cd odimh5_validator 
    $make

This will compile all the necessary sources and create the odimh5-validate binary in the odimh5_validator/bin subdirectory.

The last step is to setup the `ODIMH5_VALIDATOR_CSV_DIR` environment variable, 
which is the path to the directory with the csv tables describing the ODIM-H5 standard. 
The default csv tables are in the odimh5_validator/data subdirectory, 
so You can set the `ODIMH5_VALIDATOR_CSV_DIR` variable as the full path to this directory 
(the best solution is to add this line to Your ~/.profile or ~/.bashrc file):

    $export ODIMH5_VALIDATOR_CSV_DIR=/your/path/there/odimh5_validator/data

#### Usage ####
##### odimh5-validate #####
```
$odimh5-validate [OPTION...]

 Mandatory options:
  -i, --input arg  input ODIM-H5 file to analyze

 Optional options:
  -h, --help                    print this help message
  -c, --csv arg                 standard-definition .csv table, e.g.
                                your_path/your_table.csv
  -v, --version arg             standard version to use, e.g. 2.1
  -t, --valueTable arg          optional .csv table with the assumed
                                attribute values - the format is as in the
                                standard-definition .csv table
  -f, --failedEntriesTable arg  the csv table to save the problematic
                                entries, which is used in the correction step - the
                                format is as in the standard-definition .csv table
      --onlyValueCheck          check only the values defined by the -t or
                                --valueTable option, default is False
      --checkOptional           check the presence of the optional ODIM
                                entries, default is False
      --checkExtras             check the presence of extra entries, not
                                mentioned in the standard, default is False
      --noInfo                  don`t print INFO messages, only WARNINGs and
                                ERRORs, default is False

```

Program to analyze the ODIM-H5 standard-conformance.

You can set the desired ODIM-H5 standard definition csv file in three ways:

- without any additional arguments the program is searching for the value of /Conventions and /what/object attributes in the input file 
and creates the name of the csv file as `ODIMH5_VALIDATOR_CSV_DIR/ODIM_H5_VX_Y_OBJ.csv`, where the X and Y is from the /Conventions attribute and the OBJ is from the /what/object attribute.

- using the `-c` or `--csv` option, You can set any path to a file intended to use as the ODIM-H5 standard definition file

- using the `-v` or `--version` option, You can set the desired standard version number (e.g. 2.2), 
and the program is creating the name of the csv file in the similar manner as in the first option, but it uses the supplied numbers for X and Y

You can add an additional table with assumed attribute values for the given ODIM-H5 file by the `-t` or `--valueTable` option. The format of this table should be the same as of the standard definition csv file. To check only the values defined by this table (without checking the whole ODIM compliance) use the `--onlyValueCheck` option. For further info please see the [Assumed Value Definition Format](#markdown-header-assumed-value-definition-format) paragraph.

The default behavior is to check only the presence and layout of the mandatory items. 
You can enable the controlling of the optional items with the `-checkOptional` option 
and enable the checking of the presence of some extra items not mentioned in the standard with the `--checkExtras` option.

To restrict the output only to WARNING and ERROR messages You can use the `--noInfo` option.

##### odimh5-correct #####
```
$odimh5-correct [OPTION...]

 Mandatory options:
  -i, --input arg            input ODIM-H5 file to change
  -o, --output arg           output - the changed ODIM-H5 file to save
  -c, --correctionTable arg  the csv table to listing the problematic entries
                             - the format is as in the standard-definition
                             .csv table

 Optional options:
  -h, --help    print this help message
      --noInfo  don`t print INFO messages, only WARNINGs and ERRORs, default
                is False

```

Program to correct or add new entries to an ODIM-H5 file.
To correct an ODIM-H5 file, the user needs to create a csv table to describe the desired corrections.
The csv table can be created by simply writing one by the user (see example correction csv files in the `data/example` directory), or it can be created by the `odimh5-validate` program using the `-f` or `--failedEntriesTable` command line option.
This option creates a csv table listing the problematic entries found in the file, which can be used in the `-c` or `--correctionTable` command line option of the `odimh5-correct` program.

If there is no value present in the correction table for the given attribute, only the datatype of the attribute is checked and corrected.

If a completely new group is created during the correction, the new group must be listed in the correction table before any of the attributes of the group. 

When some correction is applied to a given file a new /how/metadata_changed attribute is created, listing the affected attributes.

Example usage:

1.  This step is optional. The user can create correction table by its own, if the problematic entries are known. If not, check the file with the odimh5-validate and create a table with the problematic entries. Assuming you are in the odimh5_validator directory:

    ```
    $./bin/odimh5-validate -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -c    ./data/ODIM_H5_V2_1_PVOL.csv  -f ./out/T_PAGZ41_C_LZIB.failed.csv --checkOptional –noInfo
    ```

    The output of this command should be:

    ```
    WARNING - NON-STANDARD DATA TYPE - optional entry "/how/startepochs" has non-standard datatype - it`s supposed to be a 64-bit real scalar, but isn`t. See section 3.1 in v2.1 (or higher) ODIM-H5 documetaton.
    WARNING - NON-STANDARD DATA TYPE - optional entry "/how/endepochs" has non-standard datatype - it`s supposed to be a 64-bit real scalar, but isn`t. See section 3.1 in v2.1 (or higher) ODIM-H5 documetaton.
    WARNING - NON-STANDARD DATA TYPE - optional entry "/how/lowprf" has non-standard datatype - it`s supposed to be a 64-bit real scalar, but isn`t. See section 3.1 in v2.1 (or higher) ODIM-H5 documetaton.
    WARNING - NON-STANDARD DATA TYPE - optional entry "/how/highprf" has non-standard datatype - it`s supposed to be a 64-bit real scalar, but isn`t. See section 3.1 in v2.1 (or higher) ODIM-H5 documetaton.
    INFO - saving failed entries to  ./out/T_PAGZ41_C_LZIB.failed.csv csv table
    WARNING - NON-COMPLIANT FILE - file ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf IS NOT a standard-compliant ODIM-H5 file - see the previous WARNING messages
    ```

    Four problematic attributes were found with non-standard datatype and the created `./out/T_PAGZ41_C_LZIB.failed.csv` correction table should have this content:
     
    ```
    Node;Category;Type;IsMandatory;PossibleValues;Reference
    /how/startepochs;Attribute;real;FALSE;;OPERA_ODIM_v2.1.pdf,Section 4.4
    /how/endepochs;Attribute;real;FALSE;;OPERA_ODIM_v2.1.pdf,Section 4.4
    /how/lowprf;Attribute;real;FALSE;;OPERA_ODIM_v2.1.pdf,Section 4.4
    /how/highprf;Attribute;real;FALSE;;OPERA_ODIM_v2.1.pdf,Section 4.4
    ```

    **Warning**: If there is an attribute, which does not corresponds with the `PossibleValues` column     from the standard-definition table (in this case the `./data/ODIM_H5_V2_1_PVOL.csv`  file), the correction table (the `./out/T_PAGZ41_C_LZIB.failed.csv` file) will contain this entry also with this `PossibleValues` value (e.g. `">0.0"`). This entry needs to be edited by the user to specify the exact value which will be used during the correction step. 

2.  In the correction step the ./out/T_PAGZ41_C_LZIB.failed.csv table is used to correct the problematic attributes:

    ```
    ./bin/odimh5-correct -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -o    ./out/T_PAGZ41_C_LZIB_20180403000000.corrected.hdf -c ./out/T_PAGZ41_C_LZIB.failed.csv
    ```

    In this case the new `./out/T_PAGZ41_C_LZIB_20180403000000.corrected.hdf`  file is created with the corrected attributes. When checking this file again by the `odimh5-validate` tool, no non-compliant attributes are found.


##### odimh5-check-value #####
```
$odimh5-check-value [OPTION...]

  Mandatory options:
  -i, --input arg      input ODIM-H5 file to analyze
  -a, --attribute arg  the full path to the attribute to check
  -v, --value arg      the assumed value of the attribute

 Optional options:
  -h, --help    print this help message
  -t, --type arg  specify the type of te attribute - possible values: string,
                  int, real
      --noInfo  don`t print INFO messages, only WARNINGs and ERRORs, default
                is False

```

Program to check the value of an attribute in a ODIM-H5 file.

This program is used to simply check the value of a given attribute. It runs faster, because it doesn\`t need to load all the hdf5 file structure and the ODIM standard. 

The assumed type of the attribute can be set by the `-t` or `--type` option, or can be detected according to its value - set by the `-v` or `--value` option. It is assumed to be a string if any alphabetical letter appears in the value. It\`s assumed to be a 64-bit real if it isn\`t a string and has a decimal point. In any other cases the value is assumed to be a 64-bit integer. For further info please see the [Assumed Value Definition Format](#markdown-header-assumed-value-definition-format) paragraph.

To restrict the output only to WARNING and ERROR messages You can use the `--noInfo` option.


##### Assumed Value Definition Format #####
This paragraph describes the format to define the assumed value of the attributes used in the PossibleValues column of the standard-definition csv tables and by the `-v` or `--value` option of the `odimh5-check-value` program.

To check the value of **string attributes** use the exact assumed value or an [ECMAScript regular expression](http://www.cplusplus.com/reference/regex/ECMAScript/) (regex) - some example regular expressions are in the standard-definition csv tables.  

Examples (assuming, You are in the odimh5_validator root directory):  

- to check whether the exact value of the "/how/system" string attribute is "SKJAV" use the exact value :  

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /how/system -v "SKJAV" -t string`  

- to check whether the value of the "/how/system" string attribute starts with the "SK" substring use regex :  

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /how/system -v "SK.*" -t string`  
  
  
To check the value of **real and int SCALAR attributes** use the exact assumed value or a logical expression according to the examples below.  

Examples:  

- to check whether the exact value of the "/how/wavelength" real attribute is **equal to** 5.352 use the exact value or the "=" or "==" logical operators:  

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /how/wavelength -v "5.352" -t real`    

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /how/wavelength -v "=5.352" -t real`  

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /how/wavelength -v "==5.352" -t real`  

- to check whether the value of the "/how/wavelength" real attribute is **less than or greater than some value** use the "<, ">", "<=" and ">=" logical operators. WARNING - You should use the quotation marks to not parse the "<" and ">" signs by the shell as redirections :  

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /how/wavelength -v ">5.0" -t real`  

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /how/wavelength -v ">=5" -t real`  

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /how/wavelength -v "<6" -t real`

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /how/wavelength -v "<=6.0" -t real`  

- to check whether the value of the "/how/wavelength" real attribute **lies inside some interval** use the "&&" (and) logical operator or the +- tolerance notation. WARNING - You should use the quotation marks to not parse the "<" and ">" signs by the shell as redirections :  

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /how/wavelength -v ">5.0&&<6.0" -t real` 

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /how/wavelength -v ">=5.0&&<=6.0" -t real`   

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /how/wavelength -v "=5.5+-0.5" -t real` 

- to check whether the value of the "/how/wavelength" real attribute **lies outside some interval** use the "||" (or) logical operator. WARNING - You should use the quotation marks to not parse the "<" and ">" signs by the shell as redirections :  

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /how/wavelength -v "<6.0||>7.0" -t real`  

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /how/wavelength -v "<=5.352||>=6.0" -t real`    
  
  
To check the value of **real and int ARRAY attributes** use the available statistics and logical expressions according to the examples below. When checking array attributes, You need to use also the `-t` or `--type` command line parameter to parse the logical expression correctly. The available statistics are:

- first - the first value in the array

- last - the last value in the array

- min - the minimum of the values in the array

- max - tha maximum of the values in the array

- mean - the avarge of the values in the array


Examples:  

- to check whether the first value of "/dataset1/how/startazA" real array attribute is **lower than** 1.0 degrees:  

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /dataset1/how/startazA -t real -v "first<1.0"`   

- to check whether the first value of "/dataset1/how/startazA" real array attribute is **lower than** 1.0 degrees and the last value is **greater than** 359.0 degrees:  

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /dataset1/how/startazA -t real -v "first<1.0&&last>359.0"`

- to check several statistics, You can chain the comparisons:  

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /dataset1/how/startazA -t real -v "first<1.0&&last>359.0&&min>0.0&&max<360.0"`    



The same type of logical expressions should be used also in the column "PossibleValues" in the csv tables used by the `odimh5-validate` program. The "Type" column for array attributes should have "real array" or "integer array" value. See examples in the `data/example/T_PAGZ41_C_LZIB.values.interval.csv` table.


#### Example Files ####
You can find some example ODIM-H5 volumes in the odimh5_validator/data/examples subdirectory.



### Contact ###
I case of any wishes, ideas, suggestions, bugs or contribution please contact me at ladislav.meri@shmu.sk or use the issue tracker.
