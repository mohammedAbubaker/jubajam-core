# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.30

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/mo/jubajam-core

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/mo/jubajam-core

# Include any dependencies generated for this target.
include CMakeFiles/jubajam-core.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/jubajam-core.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/jubajam-core.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/jubajam-core.dir/flags.make

CMakeFiles/jubajam-core.dir/src/parser.cpp.o: CMakeFiles/jubajam-core.dir/flags.make
CMakeFiles/jubajam-core.dir/src/parser.cpp.o: src/parser.cpp
CMakeFiles/jubajam-core.dir/src/parser.cpp.o: CMakeFiles/jubajam-core.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/mo/jubajam-core/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/jubajam-core.dir/src/parser.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/jubajam-core.dir/src/parser.cpp.o -MF CMakeFiles/jubajam-core.dir/src/parser.cpp.o.d -o CMakeFiles/jubajam-core.dir/src/parser.cpp.o -c /home/mo/jubajam-core/src/parser.cpp

CMakeFiles/jubajam-core.dir/src/parser.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/jubajam-core.dir/src/parser.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/mo/jubajam-core/src/parser.cpp > CMakeFiles/jubajam-core.dir/src/parser.cpp.i

CMakeFiles/jubajam-core.dir/src/parser.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/jubajam-core.dir/src/parser.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/mo/jubajam-core/src/parser.cpp -o CMakeFiles/jubajam-core.dir/src/parser.cpp.s

CMakeFiles/jubajam-core.dir/src/main.cpp.o: CMakeFiles/jubajam-core.dir/flags.make
CMakeFiles/jubajam-core.dir/src/main.cpp.o: src/main.cpp
CMakeFiles/jubajam-core.dir/src/main.cpp.o: CMakeFiles/jubajam-core.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/mo/jubajam-core/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/jubajam-core.dir/src/main.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/jubajam-core.dir/src/main.cpp.o -MF CMakeFiles/jubajam-core.dir/src/main.cpp.o.d -o CMakeFiles/jubajam-core.dir/src/main.cpp.o -c /home/mo/jubajam-core/src/main.cpp

CMakeFiles/jubajam-core.dir/src/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/jubajam-core.dir/src/main.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/mo/jubajam-core/src/main.cpp > CMakeFiles/jubajam-core.dir/src/main.cpp.i

CMakeFiles/jubajam-core.dir/src/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/jubajam-core.dir/src/main.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/mo/jubajam-core/src/main.cpp -o CMakeFiles/jubajam-core.dir/src/main.cpp.s

# Object files for target jubajam-core
jubajam__core_OBJECTS = \
"CMakeFiles/jubajam-core.dir/src/parser.cpp.o" \
"CMakeFiles/jubajam-core.dir/src/main.cpp.o"

# External object files for target jubajam-core
jubajam__core_EXTERNAL_OBJECTS =

jubajam-core: CMakeFiles/jubajam-core.dir/src/parser.cpp.o
jubajam-core: CMakeFiles/jubajam-core.dir/src/main.cpp.o
jubajam-core: CMakeFiles/jubajam-core.dir/build.make
jubajam-core: deps/glad/libglad.a
jubajam-core: deps/glfw-3.4/src/libglfw3.a
jubajam-core: deps/glm/glm/libglm.a
jubajam-core: /usr/lib/librt.a
jubajam-core: /usr/lib/libm.so
jubajam-core: CMakeFiles/jubajam-core.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/mo/jubajam-core/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable jubajam-core"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/jubajam-core.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/jubajam-core.dir/build: jubajam-core
.PHONY : CMakeFiles/jubajam-core.dir/build

CMakeFiles/jubajam-core.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/jubajam-core.dir/cmake_clean.cmake
.PHONY : CMakeFiles/jubajam-core.dir/clean

CMakeFiles/jubajam-core.dir/depend:
	cd /home/mo/jubajam-core && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/mo/jubajam-core /home/mo/jubajam-core /home/mo/jubajam-core /home/mo/jubajam-core /home/mo/jubajam-core/CMakeFiles/jubajam-core.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/jubajam-core.dir/depend

