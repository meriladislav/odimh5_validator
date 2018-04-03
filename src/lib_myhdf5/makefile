include makefile-commons.in

THIS_SRC_DIR = $(SRC_DIR)/lib_myhdf5

TARGET_LIB = $(LIB_DIR)/libmyhdf5.a 

OBJ_LIST = $(OBJ_DIR)/class_MyHdf5Unit.o  

all : $(TARGET_LIB)

clean :
	rm -f $(OBJ_LIST) \
          $(TARGET_LIB) 
          
$(TARGET_LIB) : $(OBJ_LIST) 
	$(AR) $(ARFLAGS) $@ $(OBJ_LIST) 

$(OBJ_DIR)/class_MyHdf5Unit.o : $(THIS_SRC_DIR)/class_MyHdf5Unit.cpp \
                                $(THIS_SRC_DIR)/class_MyHdf5Unit.hpp 
	$(CXX) $(CXX_FLAGS) $(INC_FLAGS) -c -o $@ $(THIS_SRC_DIR)/class_MyHdf5Unit.cpp
	 