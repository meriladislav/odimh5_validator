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
so You can set the `ODIMH5_VALIDATOR_CSV_DIR` variable as the full path to this directoty 
(the best solution is to add this line to Your ~/.profile or ~/.bashrc file):

    $export ODIMH5_VALIDATOR_CSV_DIR=/your/path/there/odimh5_validator/data

#### Usage ####
```
$odimh5-validate [OPTION...]

 Mandatory options:
  -i, --input arg  input ODIM-H5 file to analyse

 Optional options:
  -h, --help           print this help message
  -c, --csv arg        standard-definition .csv table, e.g.
                       your_path/your_table.csv
  -v, --version arg    standard version to use, e.g. 2.1
  -t, --valueTable arg  optional .csv table with the assumed attribute values
                        - the format is as in the standard-definition .csv
                        table
      --checkOptional  check the presence of the optional ODIM entries,
                       default is False
      --checkExtras    check the presence of extra entries, not mentioned in
                       the standard, default is False
      --noInfo         don`t print INFO messages, only WARNINGs and ERRORs,
                       default is False
```

You can set the desired ODIM-H5 standard definition csv file in three ways:

- without any additional arguments the program is searching for the value of /Conventions and /what/object attributes in the input file 
and creates the name of the csv file as `ODIMH5_VALIDATOR_CSV_DIR/ODIM_H5_VX_Y_OBJ.csv`, where the X and Y is from the /Conventions attribute and the OBJ is from the /what/object attribute.

- using the `-c` or `--csv` option, You can set any path to a file intended to use as the ODIM-H5 standard definition file

- using the `-v` or `--version` option, You can set the desired stdandard version number (e.g. 2.2), 
and the program is creating the name of the csv file in the similar manner as in the first option, but it uses the supplied numbers for X and Y

You can add an additional table with assumed attribute values for the given ODIM-H5 file by the `-t` or `--valueTable` option. The format of this table should be the same as of the standard definition csv file.

The default behaviour is to check only the presence and layout of the mandatory items. 
You can enable the controlling of the optional items wiht the `-checkOptional` option 
and enable the chcecking of the presence of some extar items not mentioned in the standard with the `--checkExtras` option.

To restrict the output only to WARNING and ERROR messages You can use the `--noInfo` option.

#### Examples ####
You can find some example ODIM-H5 volumes in the odimh5_validator/data/examples subdirectory.


### Contact ###
I case of any wishes, ideas, suggestions, bugs or contribution please contact me at ladislav.meri@shmu.sk.
