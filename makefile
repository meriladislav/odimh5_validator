###############################################################################
# makefile
# makefile for the odimh5_validator software
# v_0.0, 04.2018, L. Meri, SHMU
###############################################################################

include makefile-commons.in

LIB_LIST = $(LIB_DIR)/libmyodimh5.a 
           
BIN_LIST = $(BIN_DIR)/odimh5-validate

OBJ_LIST = $(OBJ_DIR)/class_H5Layout.o \
           $(OBJ_DIR)/class_OdimEntry.o \
           $(OBJ_DIR)/class_OdimStandard.o \
           $(OBJ_DIR)/module_Compare.o

all : $(LIB_LIST) $(BIN_LIST)
	@echo ""
	@echo "###############################################################"
	@echo "# Congratulation! The odimh5_validator build was SUCCESSFULL.  "
	@echo "# Consider setting env variable: ODIMH5_VALIDATOR_CSV_DIR=$(PWD)/data - see also in the Readme.md file"
	@echo "###############################################################"
	@echo ""
	
cleanall: 
	rm -rf $(BIN_DIR)/* \
	       $(LIB_DIR)/* \
	       $(OBJ_DIR)/* 
	       
cleanmy:
	rm -rf $(BIN_DIR)/odimh5-* \
	       $(LIB_DIR)/libmy*.a \
	       $(OBJ_DIR)/*
	       
	       
$(LIB_DIR)/libmyodimh5.a: $(OBJ_LIST)
	@echo ""
	@echo "checking h5cc ..."
	$(H5CC) -showconfig
	@echo "h5cc OK"
	@echo ""
	$(AR) $(ARFLAGS) $@ $(OBJ_LIST) 


$(BIN_DIR)/odimh5-validate: $(SRC_DIR)/odimh5-validate.cpp $(LIB_LIST)
	@echo "Compiling binaries ..."
	$(CXX) $(CXX_FLAGS) $(INC_FLAGS) -o $@ $(SRC_DIR)/odimh5-validate.cpp $(LIB_FLAGS) 
	@echo "Compiling binaries ... OK"
	@echo ""
	
$(OBJ_DIR)/class_H5Layout.o: $(SRC_DIR)/class_H5Layout.cpp $(SRC_DIR)/class_H5Layout.hpp 
	$(CXX) $(CXX_FLAGS) $(INC_FLAGS) -c -o $@ $(SRC_DIR)/class_H5Layout.cpp

$(OBJ_DIR)/class_OdimEntry.o: $(SRC_DIR)/class_OdimEntry.cpp $(SRC_DIR)/class_OdimEntry.hpp 
	$(CXX) $(CXX_FLAGS) $(INC_FLAGS) -c -o $@ $(SRC_DIR)/class_OdimEntry.cpp  

$(OBJ_DIR)/class_OdimStandard.o: $(SRC_DIR)/class_OdimStandard.cpp $(SRC_DIR)/class_OdimStandard.hpp \
                                 $(OBJ_DIR)/class_OdimEntry.o
	$(CXX) $(CXX_FLAGS) $(INC_FLAGS) -c -o $@ $(SRC_DIR)/class_OdimStandard.cpp

$(OBJ_DIR)/module_Compare.o: $(SRC_DIR)/module_Compare.cpp $(SRC_DIR)/module_Compare.hpp \
                             $(OBJ_DIR)/class_H5Layout.o \
                             $(OBJ_DIR)/class_OdimStandard.o
	$(CXX) $(CXX_FLAGS) $(INC_FLAGS) -c -o $@ $(SRC_DIR)/module_Compare.cpp
