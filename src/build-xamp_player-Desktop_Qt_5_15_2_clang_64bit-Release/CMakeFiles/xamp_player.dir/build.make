# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.15

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Produce verbose output by default.
VERBOSE = 1

# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /Applications/CMake.app/Contents/bin/cmake

# The command to remove a file.
RM = /Applications/CMake.app/Contents/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/bill/Documents/xamp2/src/xamp_player

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/bill/Documents/xamp2/src/build-xamp_player-Desktop_Qt_5_15_2_clang_64bit-Release

# Include any dependencies generated for this target.
include CMakeFiles/xamp_player.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/xamp_player.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/xamp_player.dir/flags.make

CMakeFiles/xamp_player.dir/src/api.cpp.o: CMakeFiles/xamp_player.dir/flags.make
CMakeFiles/xamp_player.dir/src/api.cpp.o: /Users/bill/Documents/xamp2/src/xamp_player/src/api.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/bill/Documents/xamp2/src/build-xamp_player-Desktop_Qt_5_15_2_clang_64bit-Release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/xamp_player.dir/src/api.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/xamp_player.dir/src/api.cpp.o -c /Users/bill/Documents/xamp2/src/xamp_player/src/api.cpp

CMakeFiles/xamp_player.dir/src/api.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/xamp_player.dir/src/api.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/bill/Documents/xamp2/src/xamp_player/src/api.cpp > CMakeFiles/xamp_player.dir/src/api.cpp.i

CMakeFiles/xamp_player.dir/src/api.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/xamp_player.dir/src/api.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/bill/Documents/xamp2/src/xamp_player/src/api.cpp -o CMakeFiles/xamp_player.dir/src/api.cpp.s

CMakeFiles/xamp_player.dir/src/audio_player.cpp.o: CMakeFiles/xamp_player.dir/flags.make
CMakeFiles/xamp_player.dir/src/audio_player.cpp.o: /Users/bill/Documents/xamp2/src/xamp_player/src/audio_player.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/bill/Documents/xamp2/src/build-xamp_player-Desktop_Qt_5_15_2_clang_64bit-Release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/xamp_player.dir/src/audio_player.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/xamp_player.dir/src/audio_player.cpp.o -c /Users/bill/Documents/xamp2/src/xamp_player/src/audio_player.cpp

CMakeFiles/xamp_player.dir/src/audio_player.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/xamp_player.dir/src/audio_player.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/bill/Documents/xamp2/src/xamp_player/src/audio_player.cpp > CMakeFiles/xamp_player.dir/src/audio_player.cpp.i

CMakeFiles/xamp_player.dir/src/audio_player.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/xamp_player.dir/src/audio_player.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/bill/Documents/xamp2/src/xamp_player/src/audio_player.cpp -o CMakeFiles/xamp_player.dir/src/audio_player.cpp.s

CMakeFiles/xamp_player.dir/src/audio_util.cpp.o: CMakeFiles/xamp_player.dir/flags.make
CMakeFiles/xamp_player.dir/src/audio_util.cpp.o: /Users/bill/Documents/xamp2/src/xamp_player/src/audio_util.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/bill/Documents/xamp2/src/build-xamp_player-Desktop_Qt_5_15_2_clang_64bit-Release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/xamp_player.dir/src/audio_util.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/xamp_player.dir/src/audio_util.cpp.o -c /Users/bill/Documents/xamp2/src/xamp_player/src/audio_util.cpp

CMakeFiles/xamp_player.dir/src/audio_util.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/xamp_player.dir/src/audio_util.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/bill/Documents/xamp2/src/xamp_player/src/audio_util.cpp > CMakeFiles/xamp_player.dir/src/audio_util.cpp.i

CMakeFiles/xamp_player.dir/src/audio_util.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/xamp_player.dir/src/audio_util.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/bill/Documents/xamp2/src/xamp_player/src/audio_util.cpp -o CMakeFiles/xamp_player.dir/src/audio_util.cpp.s

CMakeFiles/xamp_player.dir/src/chromaprint.cpp.o: CMakeFiles/xamp_player.dir/flags.make
CMakeFiles/xamp_player.dir/src/chromaprint.cpp.o: /Users/bill/Documents/xamp2/src/xamp_player/src/chromaprint.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/bill/Documents/xamp2/src/build-xamp_player-Desktop_Qt_5_15_2_clang_64bit-Release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/xamp_player.dir/src/chromaprint.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/xamp_player.dir/src/chromaprint.cpp.o -c /Users/bill/Documents/xamp2/src/xamp_player/src/chromaprint.cpp

CMakeFiles/xamp_player.dir/src/chromaprint.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/xamp_player.dir/src/chromaprint.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/bill/Documents/xamp2/src/xamp_player/src/chromaprint.cpp > CMakeFiles/xamp_player.dir/src/chromaprint.cpp.i

CMakeFiles/xamp_player.dir/src/chromaprint.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/xamp_player.dir/src/chromaprint.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/bill/Documents/xamp2/src/xamp_player/src/chromaprint.cpp -o CMakeFiles/xamp_player.dir/src/chromaprint.cpp.s

CMakeFiles/xamp_player.dir/src/fft.cpp.o: CMakeFiles/xamp_player.dir/flags.make
CMakeFiles/xamp_player.dir/src/fft.cpp.o: /Users/bill/Documents/xamp2/src/xamp_player/src/fft.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/bill/Documents/xamp2/src/build-xamp_player-Desktop_Qt_5_15_2_clang_64bit-Release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/xamp_player.dir/src/fft.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/xamp_player.dir/src/fft.cpp.o -c /Users/bill/Documents/xamp2/src/xamp_player/src/fft.cpp

CMakeFiles/xamp_player.dir/src/fft.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/xamp_player.dir/src/fft.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/bill/Documents/xamp2/src/xamp_player/src/fft.cpp > CMakeFiles/xamp_player.dir/src/fft.cpp.i

CMakeFiles/xamp_player.dir/src/fft.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/xamp_player.dir/src/fft.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/bill/Documents/xamp2/src/xamp_player/src/fft.cpp -o CMakeFiles/xamp_player.dir/src/fft.cpp.s

CMakeFiles/xamp_player.dir/src/libebur128/ebur128.c.o: CMakeFiles/xamp_player.dir/flags.make
CMakeFiles/xamp_player.dir/src/libebur128/ebur128.c.o: /Users/bill/Documents/xamp2/src/xamp_player/src/libebur128/ebur128.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/bill/Documents/xamp2/src/build-xamp_player-Desktop_Qt_5_15_2_clang_64bit-Release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object CMakeFiles/xamp_player.dir/src/libebur128/ebur128.c.o"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/xamp_player.dir/src/libebur128/ebur128.c.o   -c /Users/bill/Documents/xamp2/src/xamp_player/src/libebur128/ebur128.c

CMakeFiles/xamp_player.dir/src/libebur128/ebur128.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/xamp_player.dir/src/libebur128/ebur128.c.i"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/bill/Documents/xamp2/src/xamp_player/src/libebur128/ebur128.c > CMakeFiles/xamp_player.dir/src/libebur128/ebur128.c.i

CMakeFiles/xamp_player.dir/src/libebur128/ebur128.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/xamp_player.dir/src/libebur128/ebur128.c.s"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/bill/Documents/xamp2/src/xamp_player/src/libebur128/ebur128.c -o CMakeFiles/xamp_player.dir/src/libebur128/ebur128.c.s

CMakeFiles/xamp_player.dir/src/loudness_scanner.cpp.o: CMakeFiles/xamp_player.dir/flags.make
CMakeFiles/xamp_player.dir/src/loudness_scanner.cpp.o: /Users/bill/Documents/xamp2/src/xamp_player/src/loudness_scanner.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/bill/Documents/xamp2/src/build-xamp_player-Desktop_Qt_5_15_2_clang_64bit-Release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object CMakeFiles/xamp_player.dir/src/loudness_scanner.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/xamp_player.dir/src/loudness_scanner.cpp.o -c /Users/bill/Documents/xamp2/src/xamp_player/src/loudness_scanner.cpp

CMakeFiles/xamp_player.dir/src/loudness_scanner.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/xamp_player.dir/src/loudness_scanner.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/bill/Documents/xamp2/src/xamp_player/src/loudness_scanner.cpp > CMakeFiles/xamp_player.dir/src/loudness_scanner.cpp.i

CMakeFiles/xamp_player.dir/src/loudness_scanner.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/xamp_player.dir/src/loudness_scanner.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/bill/Documents/xamp2/src/xamp_player/src/loudness_scanner.cpp -o CMakeFiles/xamp_player.dir/src/loudness_scanner.cpp.s

# Object files for target xamp_player
xamp_player_OBJECTS = \
"CMakeFiles/xamp_player.dir/src/api.cpp.o" \
"CMakeFiles/xamp_player.dir/src/audio_player.cpp.o" \
"CMakeFiles/xamp_player.dir/src/audio_util.cpp.o" \
"CMakeFiles/xamp_player.dir/src/chromaprint.cpp.o" \
"CMakeFiles/xamp_player.dir/src/fft.cpp.o" \
"CMakeFiles/xamp_player.dir/src/libebur128/ebur128.c.o" \
"CMakeFiles/xamp_player.dir/src/loudness_scanner.cpp.o"

# External object files for target xamp_player
xamp_player_EXTERNAL_OBJECTS =

/Users/bill/Documents/xamp2/src/xamp_player/release/libxamp_player.dylib: CMakeFiles/xamp_player.dir/src/api.cpp.o
/Users/bill/Documents/xamp2/src/xamp_player/release/libxamp_player.dylib: CMakeFiles/xamp_player.dir/src/audio_player.cpp.o
/Users/bill/Documents/xamp2/src/xamp_player/release/libxamp_player.dylib: CMakeFiles/xamp_player.dir/src/audio_util.cpp.o
/Users/bill/Documents/xamp2/src/xamp_player/release/libxamp_player.dylib: CMakeFiles/xamp_player.dir/src/chromaprint.cpp.o
/Users/bill/Documents/xamp2/src/xamp_player/release/libxamp_player.dylib: CMakeFiles/xamp_player.dir/src/fft.cpp.o
/Users/bill/Documents/xamp2/src/xamp_player/release/libxamp_player.dylib: CMakeFiles/xamp_player.dir/src/libebur128/ebur128.c.o
/Users/bill/Documents/xamp2/src/xamp_player/release/libxamp_player.dylib: CMakeFiles/xamp_player.dir/src/loudness_scanner.cpp.o
/Users/bill/Documents/xamp2/src/xamp_player/release/libxamp_player.dylib: CMakeFiles/xamp_player.dir/build.make
/Users/bill/Documents/xamp2/src/xamp_player/release/libxamp_player.dylib: /Users/bill/Documents/xamp2/src/xamp_base/release/libxamp_base.dylib
/Users/bill/Documents/xamp2/src/xamp_player/release/libxamp_player.dylib: /Users/bill/Documents/xamp2/src/xamp_output_device/release/libxamp_output_device.dylib
/Users/bill/Documents/xamp2/src/xamp_player/release/libxamp_player.dylib: /Users/bill/Documents/xamp2/src/xamp_stream/release/libxamp_stream.dylib
/Users/bill/Documents/xamp2/src/xamp_player/release/libxamp_player.dylib: /Users/bill/Documents/xamp2/src/xamp_metadata/release/libxamp_metadata.dylib
/Users/bill/Documents/xamp2/src/xamp_player/release/libxamp_player.dylib: CMakeFiles/xamp_player.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/bill/Documents/xamp2/src/build-xamp_player-Desktop_Qt_5_15_2_clang_64bit-Release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Linking CXX shared library /Users/bill/Documents/xamp2/src/xamp_player/release/libxamp_player.dylib"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/xamp_player.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/xamp_player.dir/build: /Users/bill/Documents/xamp2/src/xamp_player/release/libxamp_player.dylib

.PHONY : CMakeFiles/xamp_player.dir/build

CMakeFiles/xamp_player.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/xamp_player.dir/cmake_clean.cmake
.PHONY : CMakeFiles/xamp_player.dir/clean

CMakeFiles/xamp_player.dir/depend:
	cd /Users/bill/Documents/xamp2/src/build-xamp_player-Desktop_Qt_5_15_2_clang_64bit-Release && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/bill/Documents/xamp2/src/xamp_player /Users/bill/Documents/xamp2/src/xamp_player /Users/bill/Documents/xamp2/src/build-xamp_player-Desktop_Qt_5_15_2_clang_64bit-Release /Users/bill/Documents/xamp2/src/build-xamp_player-Desktop_Qt_5_15_2_clang_64bit-Release /Users/bill/Documents/xamp2/src/build-xamp_player-Desktop_Qt_5_15_2_clang_64bit-Release/CMakeFiles/xamp_player.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/xamp_player.dir/depend
