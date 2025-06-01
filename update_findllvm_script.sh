#!/bin/bash
set -e

FILE_PATH="CMake/FindLLVM.cmake"

# Create a backup
cp "$FILE_PATH" "${FILE_PATH}.bak"

# New content for FindLLVM.cmake
# This version simplifies Clang library finding, assuming Clang libraries are
# either pulled in by the main LLVM library or by a general clang-cpp library.
# It also tries to get Clang include dirs.

cat > "$FILE_PATH" << 'EOF'
# Find the native LLVM includes and library
#
# LLVM_INCLUDE_DIR - where to find llvm include files
# LLVM_LIBRARY_DIR - where to find llvm libs
# LLVM_CFLAGS      - llvm compiler flags
# LLVM_LDFLAGS      - llvm linker flags
# LLVM_MODULE_LIBS - list of llvm libs for working with modules.
# LLVM_FOUND       - True if llvm found.

if (LLVM_INSTALL_DIR)
  find_program(LLVM_CONFIG_EXECUTABLE
               NAMES llvm-config-39 llvm-config-3.9 llvm-config-38 llvm-config-3.8 llvm-config-37 llvm-config-3.7 llvm-config-36 llvm-config-3.6 llvm-config-35 llvm-config-3.5 llvm-config-34 llvm-config-3.4 llvm-config
               DOC "llvm-config executable"
               PATHS ${LLVM_INSTALL_DIR} NO_DEFAULT_PATH HINTS ENV LLVM_DIR)
else (LLVM_INSTALL_DIR)
  find_program(LLVM_CONFIG_EXECUTABLE
               NAMES llvm-config-39 llvm-config-3.9 llvm-config-38 llvm-config-3.8 llvm-config-37 llvm-config-3.7 llvm-config-36 llvm-config-3.6 llvm-config-35 llvm-config-3.5 llvm-config-34 llvm-config-3.4 llvm-config
               DOC "llvm-config executable" HINTS ENV LLVM_DIR)
endif (LLVM_INSTALL_DIR)

if (NOT LLVM_CONFIG_EXECUTABLE)
  # Try finding newer versions if specific ones weren't found
  find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config-18 llvm-config-17 llvm-config-16 llvm-config-15 llvm-config-14 llvm-config-13 llvm-config-12 llvm-config-11 llvm-config-10 llvm-config HINTS ENV LLVM_DIR)
endif()

if (LLVM_CONFIG_EXECUTABLE)
  message(STATUS "LLVM llvm-config found at: ${LLVM_CONFIG_EXECUTABLE}")
else (LLVM_CONFIG_EXECUTABLE)
  message(FATAL_ERROR "Could NOT find LLVM executable (llvm-config). Please set LLVM_INSTALL_DIR to the directory containing llvm-config, or ensure llvm-config is in your PATH.")
endif (LLVM_CONFIG_EXECUTABLE)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
  OUTPUT_VARIABLE LLVM_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REGEX REPLACE "([0-9]+)\.([0-9]+).*" "\1\2" LLVM_VERSION_NODOT ${LLVM_VERSION})
string(REGEX REPLACE "([0-9]+)\.([0-9]+).*" "\1.\2" LLVM_VERSION_NOPATCH ${LLVM_VERSION})
string(REGEX REPLACE "([0-9]+)\..*" "\1" LLVM_VERSION_MAJOR ${LLVM_VERSION})


message(STATUS "LLVM version: ${LLVM_VERSION} (nodot: ${LLVM_VERSION_NODOT}, major: ${LLVM_VERSION_MAJOR})")

if (LLVM_FIND_VERSION_MAJOR AND LLVM_FIND_VERSION_MINOR)
  SET(LLVM_FIND_VERSION_NODOT_CMP "${LLVM_FIND_VERSION_MAJOR}${LLVM_FIND_VERSION_MINOR}")
  if (LLVM_VERSION_NODOT VERSION_LESS LLVM_FIND_VERSION_NODOT_CMP)
    message(FATAL_ERROR "Incompatible LLVM version ${LLVM_VERSION}. Version ${LLVM_FIND_VERSION_MAJOR}.${LLVM_FIND_VERSION_MINOR} or newer is required.")
  else()
    message(STATUS "Found compatible LLVM version ${LLVM_VERSION}")
  endif()
endif()
add_definitions("-DLLVM_VERSION_NODOT=${LLVM_VERSION_NODOT}")
add_definitions("-DLLVM_VERSION_MAJOR=${LLVM_VERSION_MAJOR}")


# Try to find clang executable to get its resource directory for includes
if (LLVM_INSTALL_DIR)
  find_program(CLANG_EXECUTABLE
               NAMES clang-${LLVM_VERSION_NODOT} clang-${LLVM_VERSION_NOPATCH} clang-${LLVM_VERSION_MAJOR} clang
               PATHS ${LLVM_INSTALL_DIR} NO_DEFAULT_PATH HINTS ${LLVM_INSTALL_DIR}/bin)
else()
  find_program(CLANG_EXECUTABLE
               NAMES clang-${LLVM_VERSION_NODOT} clang-${LLVM_VERSION_NOPATCH} clang-${LLVM_VERSION_MAJOR} clang)
endif()

if(CLANG_EXECUTABLE)
    message(STATUS "Clang executable found at: ${CLANG_EXECUTABLE}")
    execute_process(
      COMMAND ${CLANG_EXECUTABLE} -print-resource-dir
      OUTPUT_VARIABLE CLANG_RESOURCE_DIR
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(CLANG_RESOURCE_DIR AND EXISTS "${CLANG_RESOURCE_DIR}/include")
        message(STATUS "Clang resource include directory: ${CLANG_RESOURCE_DIR}/include")
        set(CLANG_INCLUDE_DIR "${CLANG_RESOURCE_DIR}/include")
    else()
        message(STATUS "Could not determine Clang resource include directory or it does not exist.")
        set(CLANG_INCLUDE_DIR "") # Fallback later
    endif()
else()
    message(WARNING "Clang executable not found. May have trouble finding Clang-specific headers.")
    set(CLANG_INCLUDE_DIR "") # Fallback later
endif()

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
  OUTPUT_VARIABLE LLVM_INCLUDE_DIR_MAIN
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(LLVM_INCLUDE_DIRS ${LLVM_INCLUDE_DIR_MAIN})
if(CLANG_INCLUDE_DIR AND EXISTS ${CLANG_INCLUDE_DIR})
  list(APPEND LLVM_INCLUDE_DIRS ${CLANG_INCLUDE_DIR})
else()
  # Fallback: try standard Clang include path relative to LLVM include dir for older setups
  if(EXISTS "${LLVM_INCLUDE_DIR_MAIN}/clang")
    list(APPEND LLVM_INCLUDE_DIRS "${LLVM_INCLUDE_DIR_MAIN}/clang")
  endif()
  if(EXISTS "${LLVM_INCLUDE_DIR_MAIN}/clang-c")
    list(APPEND LLVM_INCLUDE_DIRS "${LLVM_INCLUDE_DIR_MAIN}/clang-c")
  endif()
endif()
list(REMOVE_DUPLICATES LLVM_INCLUDE_DIRS)
message(STATUS "LLVM Include Directories: ${LLVM_INCLUDE_DIRS}")
include_directories(${LLVM_INCLUDE_DIRS})


execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --libdir
  OUTPUT_VARIABLE LLVM_LIBRARY_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "LLVM Library Directory: ${LLVM_LIBRARY_DIR}")

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --cppflags
  OUTPUT_VARIABLE LLVM_CFLAGS_FROM_CONFIG
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(LLVM_CFLAGS ${LLVM_CFLAGS_FROM_CONFIG}) # Start with llvm-config's cflags
# Add other include dirs we found to CFLAGS (already handled by include_directories() for compilation)
# but ensure they are part of LLVM_CFLAGS for other uses if any.
foreach(dir ${LLVM_INCLUDE_DIRS})
    set(LLVM_CFLAGS "${LLVM_CFLAGS} -I${dir}")
endforeach()
message(STATUS "LLVM CFLAGS: ${LLVM_CFLAGS}")


execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
  OUTPUT_VARIABLE LLVM_LDFLAGS_FROM_CONFIG
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(LLVM_LDFLAGS ${LLVM_LDFLAGS_FROM_CONFIG})
message(STATUS "LLVM LDFLAGS: ${LLVM_LDFLAGS}")

# Get core LLVM libraries
execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs --link-static # Attempt static linking first
  OUTPUT_VARIABLE LLVM_MODULE_LIBS_STATIC
  RESULT_VARIABLE LLVM_LIBS_STATIC_RESULT
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs
  OUTPUT_VARIABLE LLVM_MODULE_LIBS_DYNAMIC
  RESULT_VARIABLE LLVM_LIBS_DYNAMIC_RESULT
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(LLVM_LIBS_STATIC_RESULT EQUAL 0 AND LLVM_MODULE_LIBS_STATIC)
    set(LLVM_MODULE_LIBS ${LLVM_MODULE_LIBS_STATIC})
    message(STATUS "LLVM Core Libraries (static): ${LLVM_MODULE_LIBS}")
else()
    set(LLVM_MODULE_LIBS ${LLVM_MODULE_LIBS_DYNAMIC})
    message(STATUS "LLVM Core Libraries (dynamic): ${LLVM_MODULE_LIBS}")
endif()


# For Clang, we'll try to link against libclang-cpp
# Modern Clang often uses libclang-cpp.so (or versioned e.g. libclang-cpp.so.18)
# For older ones, it might be different.
set(CLANG_LIBRARIES "")
find_library(CLANG_CPP_LIB NAMES clang-cpp clang-cpp-${LLVM_VERSION_MAJOR} clang-cpp-${LLVM_VERSION_NOPATCH}
             HINTS ${LLVM_LIBRARY_DIR} ${LLVM_LIBRARY_DIR}/../lib) # Search LLVM libdir and common related paths

if(CLANG_CPP_LIB)
  message(STATUS "Found Clang C++ library: ${CLANG_CPP_LIB}")
  set(CLANG_LIBRARIES ${CLANG_CPP_LIB})
else()
  message(WARNING "Could not find libclang-cpp. Trying to find generic libclang.")
  # Fallback to finding generic libclang
  find_library(CLANG_GENERIC_LIB NAMES clang Clang
               HINTS ${LLVM_LIBRARY_DIR} ${LLVM_LIBRARY_DIR}/../lib)
  if(CLANG_GENERIC_LIB)
    message(STATUS "Found generic Clang library: ${CLANG_GENERIC_LIB}")
    set(CLANG_LIBRARIES ${CLANG_GENERIC_LIB})
  else()
    message(WARNING "Could not find generic Clang library (libclang-cpp or libclang). Clang linking might fail.")
  endif()
endif()

# Append Clang libraries to LLVM_MODULE_LIBS if found
if(CLANG_LIBRARIES)
  set(LLVM_MODULE_LIBS "${LLVM_MODULE_LIBS} ${CLANG_LIBRARIES}")
endif()

# System libraries (like -ldl, -lpthread, etc.)
execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --system-libs --link-static
  OUTPUT_VARIABLE LLVM_SYSTEM_LIBS_STATIC
  RESULT_VARIABLE LLVM_SYSTEM_LIBS_STATIC_RESULT
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --system-libs
  OUTPUT_VARIABLE LLVM_SYSTEM_LIBS_DYNAMIC
  RESULT_VARIABLE LLVM_SYSTEM_LIBS_DYNAMIC_RESULT
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(LLVM_SYSTEM_LIBS_STATIC_RESULT EQUAL 0 AND LLVM_SYSTEM_LIBS_STATIC)
    set(LLVM_SYSTEM_LIBS ${LLVM_SYSTEM_LIBS_STATIC})
else()
    set(LLVM_SYSTEM_LIBS ${LLVM_SYSTEM_LIBS_DYNAMIC})
endif()

if (LLVM_SYSTEM_LIBS)
  string(REGEX REPLACE " *
" "" _temp_libs "${LLVM_SYSTEM_LIBS}") # Remove newlines
  set(LLVM_SYSTEM_LIBS ${_temp_libs})
  message(STATUS "LLVM System Libraries: ${LLVM_SYSTEM_LIBS}")
  set(LLVM_MODULE_LIBS "${LLVM_MODULE_LIBS} ${LLVM_SYSTEM_LIBS}")
endif()

# Define LLVM_FOUND and required variables for use by main CMakeLists.txt
if (LLVM_INCLUDE_DIRS AND LLVM_LIBRARY_DIR AND LLVM_MODULE_LIBS)
  set(LLVM_FOUND TRUE)
  # The main CMakeLists.txt seems to expect LLVM_LDFLAGS to contain library paths for linker
  # and LLVM_MODULE_LIBS to contain the actual -l flags.
  # llvm-config --ldflags often includes -L<path>
  # llvm-config --libs includes -l<lib_name>
  # Ensure CMAKE_SHARED_LINKER_FLAGS gets what it needs.
  # The original FindLLVM.cmake put --ldflags output into LLVM_LDFLAGS
  # and --libs output into LLVM_MODULE_LIBS.
  # The main CMakeLists.txt uses:
  # set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-Bsymbolic -Wl,--no-undefined ${LLVM_LDFLAGS}")
  # It seems it doesn't directly use LLVM_MODULE_LIBS in CMAKE_SHARED_LINKER_FLAGS, but individual targets link against ${LLVM_MODULE_LIBS}
  # This should be okay.
else()
  set(LLVM_FOUND FALSE)
  message(WARNING "FindLLVM.cmake did not find all required LLVM components.")
endif()

# Mark variables as advanced if you want them to be hidden in CMake GUI by default
MARK_AS_ADVANCED(
    LLVM_CONFIG_EXECUTABLE
    CLANG_EXECUTABLE
    LLVM_INCLUDE_DIR_MAIN
    CLANG_INCLUDE_DIR
    LLVM_CFLAGS_FROM_CONFIG
    LLVM_LDFLAGS_FROM_CONFIG
    LLVM_MODULE_LIBS_STATIC
    LLVM_MODULE_LIBS_DYNAMIC
    LLVM_SYSTEM_LIBS_STATIC
    LLVM_SYSTEM_LIBS_DYNAMIC
    CLANG_CPP_LIB
    CLANG_GENERIC_LIB
)
EOF

echo "File CMake/FindLLVM.cmake has been updated."
echo "Showing diff:"
diff -u "${FILE_PATH}.bak" "$FILE_PATH" || true # Show diff, continue if no changes (though there should be)

# Verify the new file content (optional sanity check)
echo ""
echo "First few lines of the new CMake/FindLLVM.cmake:"
head -n 20 "$FILE_PATH"
echo ""
echo "Last few lines of the new CMake/FindLLVM.cmake:"
tail -n 20 "$FILE_PATH"
