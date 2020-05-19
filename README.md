# README #

This documentation describes the installation and usage of the odimh5-validator software.

### What is this repository for? ###

The odimh5-validator is a command-line tool to check whether a given file is compliant with the EUMETNET-OPERA ODIM-H5 format standard
(OPERA Data Information Model for HDF5).


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
  -h, --help           print this help message
  -c, --csv arg        standard-definition .csv table, e.g.
                       your_path/your_table.csv
  -v, --version arg    standard version to use, e.g. 2.1
  -t, --valueTable arg  optional .csv table with the assumed attribute values
                        - the format is as in the standard-definition .csv
                        table
      --onlyValueCheck  check only the values defined by the -t or
                        --valueTable option, default is False
      --checkOptional  check the presence of the optional ODIM entries,
                       default is False
      --checkExtras    check the presence of extra entries, not mentioned in
                       the standard, default is False
      --noInfo         don`t print INFO messages, only WARNINGs and ERRORs,
                       default is False
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

- to check whether the value of the "/how/wavelength" real attribute **lies inside some interval** use the "&&" (and) logical operator. WARNING - You should use the quotation marks to not parse the "<" and ">" signs by the shell as redirections :  

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /how/wavelength -v ">5.0&&<6.0" -t real` 

`$./bin/odimh5-check-value -i ./data/example/T_PAGZ41_C_LZIB_20180403000000.hdf -a /how/wavelength -v ">=5.0&&<=6.0" -t real`   

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
I case of any wishes, ideas, suggestions, bugs or contribution please contact me at ladislav.meri@shmu.sk.
