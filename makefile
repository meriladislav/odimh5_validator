###############################################################################
# makefile
# makefile for the odimh5_validator software
# v_0.0, 04.2018, L. Meri, SHMU
###############################################################################

include makefile-commons.in

LIB_LIST = $(LIB_DIR)/libmyodimh5.a 
           
BIN_LIST = $(BIN_DIR)/odimh5-validate \
           $(BIN_DIR)/odimh5-check-value \
           $(BIN_DIR)/odimh5-correct

OBJ_LIST = $(OBJ_DIR)/class_H5Layout.o \
           $(OBJ_DIR)/class_OdimEntry.o \
           $(OBJ_DIR)/class_OdimStandard.o \
           $(OBJ_DIR)/module_Compare.o  \
           $(OBJ_DIR)/module_Correct.o

all : $(LIB_LIST) $(BIN_LIST)
	@echo ""
	@echo "###############################################################"
	@echo "# Congratulation! The odimh5_validator build was SUCCESSFULL.  "
	@echo "# IMPORTANT : Consider setting env variable: ODIMH5_VALIDATOR_CSV_DIR=$(PWD)/data - see also in the Readme.md file"
	@echo "###############################################################"
	@echo ""
	
clean: 
	rm -rf $(BIN_DIR)/* \
	       $(LIB_DIR)/* \
	       $(OBJ_DIR)/* 
	       
$(LIB_DIR)/libmyodimh5.a: $(OBJ_LIST)
	@echo ""
	@echo "Creating lib ..."
	$(AR) $(ARFLAGS) $@ $(OBJ_LIST) 
	@echo "Creating lib ... OK"
	@echo ""


$(BIN_DIR)/odimh5-validate: $(SRC_DIR)/odimh5-validate.cpp $(LIB_LIST)
	@echo ""
	@echo "Compiling odimh5-validate ..."
	$(CXX) $(CXX_FLAGS) $(INC_FLAGS) -o $@ $(SRC_DIR)/odimh5-validate.cpp $(LIB_FLAGS) 
	@echo "Compiling odimh5-validate ... OK"
	@echo ""
	
$(BIN_DIR)/odimh5-check-value: $(SRC_DIR)/odimh5-check-value.cpp $(LIB_LIST)
	@echo ""
	@echo "Compiling odimh5-check-value ..."
	$(CXX) $(CXX_FLAGS) $(INC_FLAGS) -o $@ $(SRC_DIR)/odimh5-check-value.cpp $(LIB_FLAGS) 
	@echo "Compiling odimh5-check-value ... OK"
	@echo ""
	
$(BIN_DIR)/odimh5-correct: $(SRC_DIR)/odimh5-correct.cpp $(LIB_LIST)
	@echo ""
	@echo "Compiling odimh5-correct ..."
	$(CXX) $(CXX_FLAGS) $(INC_FLAGS) -o $@ $(SRC_DIR)/odimh5-correct.cpp $(LIB_FLAGS) 
	@echo "Compiling odimh5-correct ... OK"
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

$(OBJ_DIR)/module_Correct.o: $(SRC_DIR)/module_Correct.cpp $(SRC_DIR)/module_Correct.hpp \
                            $(OBJ_DIR)/class_OdimStandard.o
	$(CXX) $(CXX_FLAGS) $(INC_FLAGS) -c -o $@ $(SRC_DIR)/module_Correct.cpp
	