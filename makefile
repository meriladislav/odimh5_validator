###############################################################################
# makefile
# makefile for the odimh5_validator software
# v_0.0, 04.2018, L. Meri, SHMU
###############################################################################

include makefile-commons.in

LIB_LIST = $(LIB_DIR)/libmyhdf5.a 
           
BIN_LIST = $(BIN_DIR)/odimh5-validate

all : $(LIB_LIST) $(BIN_LIST)
	@echo ""
	@echo "###############################################################"
	@echo "# Congratulation! The odimh5_validator build was SUCCESSFULL. #"
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
	       
	       
$(LIB_DIR)/libmyhdf5.a: $(SRC_DIR)/lib_myhdf5/*.cpp \
                        $(SRC_DIR)/lib_myhdf5/*.hpp 
	@echo "checking h5cc ..."
	h5cc -showconfig
	@echo "h5cc OK"
	@echo "Compiling libmyhdf5 ..."
	$(MAKE) -f $(SRC_DIR)/lib_myhdf5/makefile all
	cp -f $(SRC_DIR)/lib_myhdf5/*.hpp $(INC_DIR)
	@echo "Compiling libmyhdf5 ... OK"
	@echo ""


	
$(BIN_DIR)/odimh5-validate: $(SRC_DIR)/odimh5-validate.cpp $(LIB_LIST)
	@echo "Compiling binaries ..."
	$(CXX) $(CXX_FLAGS) $(INC_FLAGS) -o $@ $(SRC_DIR)/odimh5-validate.cpp $(LIB_FLAGS) 
	@echo "Compiling binaries ... OK"
	@echo ""