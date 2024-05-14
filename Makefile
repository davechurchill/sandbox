# to compile and run in one command type:
# make run

# define which compiler to use
CXX    := clang++
OUTPUT := sandbox

# if you need to manually specify your SFML install dir, do so here
# this is often the case on Mac silicon with brew, for me it was:
# SFML_DIR  := /opt/homebrew/Cellar/sfml/2.5.1_1
#SFML_DIR  := .

# compiler and linker flags
CXX_FLAGS := -O3 -std=c++20 -Wno-unused-result
INCLUDES  := -I./src -I ./src/imgui -I$(SFML_DIR)/include
LDFLAGS   := -O3 -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lopengl32 -L$(SFML_DIR)/lib

# if you are on a mac, you must use this LDFLAGS and comment out the previous one
#LDFLAGS   := -O3 -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -L$(SFML_DIR)/lib -framework OpenGL

# the source files for the ecs game engine
SRC_FILES := $(wildcard src/*.cpp src/imgui/*.cpp) 
OBJ_FILES := $(SRC_FILES:.cpp=.o)

# all of these targets will be made if you just type make
all:$(OUTPUT)

# define the main executable requirements / command
$(OUTPUT):$(OBJ_FILES) Makefile
	$(CXX) $(OBJ_FILES) $(LDFLAGS) -o ./bin/$@ 

# specifies how the object files are compiled from cpp files
.cpp.o:
	$(CXX) -c $(CXX_FLAGS) $(INCLUDES) $< -o $@

# typing 'make clean' will remove all intermediate build files
clean:
	rm -f $(OBJ_FILES) ./bin/sandbox
    
# typing 'make run' will compile and run the program
run: $(OUTPUT)
	cd bin && ./sandbox && cd ..