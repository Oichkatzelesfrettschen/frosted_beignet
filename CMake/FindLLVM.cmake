# Find the native LLVM includes and library
#
# LLVM_INCLUDE_DIR - where to find llvm include files
# LLVM_LIBRARY_DIR - where to find llvm libs
# LLVM_CFLAGS      - llvm compiler flags
# LLVM_LDFLAGS     - llvm linker flags
# LLVM_MODULE_LIBS - list of llvm libs for working with modules.
# LLVM_FOUND       - True if llvm found.

if (LLVM_INSTALL_DIR)
  find_program(LLVM_CONFIG_EXECUTABLE
               NAMES llvm-config-20 llvm-config-20.0 llvm-config-19 llvm-config-19.0 llvm-config-18 llvm-config-18.0 llvm-config-17 llvm-config-17.0 llvm-config-16 llvm-config-16.0 llvm-config-15 llvm-config-15.0 llvm-config-14 llvm-config-14.0 llvm-config-13 llvm-config-13.0 llvm-config-12 llvm-config-12.0 llvm-config-11 llvm-config-11.0 llvm-config-10 llvm-config-10.0 llvm-config-9 llvm-config-9.0 llvm-config-8 llvm-config-8.0 llvm-config-7 llvm-config-7.0 llvm-config-6 llvm-config-6.0 llvm-config-5 llvm-config-5.0 llvm-config-4 llvm-config-4.0 llvm-config-39 llvm-config-3.9 llvm-config-38 llvm-config-3.8 llvm-config-37 llvm-config-3.7 llvm-config-36 llvm-config-3.6 llvm-config-35 llvm-config-3.5 llvm-config-34 llvm-config-3.4 llvm-config
               DOC "llvm-config executable"
               PATHS ${LLVM_INSTALL_DIR} NO_DEFAULT_PATH HINTS ENV LLVM_DIR)
else (LLVM_INSTALL_DIR)
  find_program(LLVM_CONFIG_EXECUTABLE
               NAMES llvm-config-20 llvm-config-20.0 llvm-config-19 llvm-config-19.0 llvm-config-18 llvm-config-18.0 llvm-config-17 llvm-config-17.0 llvm-config-16 llvm-config-16.0 llvm-config-15 llvm-config-15.0 llvm-config-14 llvm-config-14.0 llvm-config-13 llvm-config-13.0 llvm-config-12 llvm-config-12.0 llvm-config-11 llvm-config-11.0 llvm-config-10 llvm-config-10.0 llvm-config-9 llvm-config-9.0 llvm-config-8 llvm-config-8.0 llvm-config-7 llvm-config-7.0 llvm-config-6 llvm-config-6.0 llvm-config-5 llvm-config-5.0 llvm-config-4 llvm-config-4.0 llvm-config-39 llvm-config-3.9 llvm-config-38 llvm-config-3.8 llvm-config-37 llvm-config-3.7 llvm-config-36 llvm-config-3.6 llvm-config-35 llvm-config-3.5 llvm-config-34 llvm-config-3.4 llvm-config
               DOC "llvm-config executable" HINTS ENV LLVM_DIR)
endif (LLVM_INSTALL_DIR)

if (NOT LLVM_CONFIG_EXECUTABLE)
  # Fallback search
  find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config HINTS ENV LLVM_DIR)
endif()

if (LLVM_CONFIG_EXECUTABLE)
  message(STATUS "LLVM llvm-config found at: ${LLVM_CONFIG_EXECUTABLE}")
else (LLVM_CONFIG_EXECUTABLE)
  message(FATAL_ERROR "Could NOT find LLVM executable (llvm-config). Please set LLVM_INSTALL_DIR.")
endif (LLVM_CONFIG_EXECUTABLE)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
  OUTPUT_VARIABLE LLVM_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Robust version regex parsing for modern LLVM (handling double digit major versions)
string(REGEX REPLACE "([0-9]+)\\.([0-9]+).*" "\\1\\2" LLVM_VERSION_NODOT ${LLVM_VERSION})
string(REGEX REPLACE "([0-9]+)\\.([0-9]+).*" "\\1.\\2" LLVM_VERSION_NOPATCH ${LLVM_VERSION})
string(REGEX REPLACE "([0-9]+)\\..*" "\\1" LLVM_MAJOR_VERSION ${LLVM_VERSION})

message(STATUS "LLVM version: ${LLVM_VERSION} (Major: ${LLVM_MAJOR_VERSION}, NoDot: ${LLVM_VERSION_NODOT})")

# Version compatibility check
if (LLVM_FIND_VERSION_MAJOR AND LLVM_FIND_VERSION_MINOR)
  SET(LLVM_FIND_VERSION_NODOT_CMP "${LLVM_FIND_VERSION_MAJOR}${LLVM_FIND_VERSION_MINOR}")
  if (LLVM_VERSION_NODOT VERSION_LESS LLVM_FIND_VERSION_NODOT_CMP)
    message(FATAL_ERROR "Incompatible LLVM version ${LLVM_VERSION}. Version ${LLVM_FIND_VERSION_MAJOR}.${LLVM_FIND_VERSION_MINOR} or newer is required.")
  else()
    message(STATUS "Found compatible LLVM version ${LLVM_VERSION}")
  endif()
endif()

add_definitions("-DLLVM_VERSION_NODOT=${LLVM_VERSION_NODOT}")
add_definitions("-DLLVM_MAJOR_VERSION=${LLVM_MAJOR_VERSION}")

# Find Clang and Tools
if (LLVM_INSTALL_DIR)
  find_program(CLANG_EXECUTABLE
               NAMES clang-${LLVM_MAJOR_VERSION} clang-${LLVM_VERSION_NODOT} clang-${LLVM_VERSION_NOPATCH} clang
               PATHS ${LLVM_INSTALL_DIR} NO_DEFAULT_PATH HINTS ${LLVM_INSTALL_DIR}/bin)
  find_program(LLVM_AS_EXECUTABLE
               NAMES llvm-as-${LLVM_MAJOR_VERSION} llvm-as-${LLVM_VERSION_NODOT} llvm-as-${LLVM_VERSION_NOPATCH} llvm-as
               PATHS ${LLVM_INSTALL_DIR} NO_DEFAULT_PATH)
  find_program(LLVM_LINK_EXECUTABLE
               NAMES llvm-link-${LLVM_MAJOR_VERSION} llvm-link-${LLVM_VERSION_NODOT} llvm-link-${LLVM_VERSION_NOPATCH} llvm-link
               PATHS ${LLVM_INSTALL_DIR} NO_DEFAULT_PATH)
else()
  find_program(CLANG_EXECUTABLE
               NAMES clang-${LLVM_MAJOR_VERSION} clang-${LLVM_VERSION_NODOT} clang-${LLVM_VERSION_NOPATCH} clang)
  find_program(LLVM_AS_EXECUTABLE
               NAMES llvm-as-${LLVM_MAJOR_VERSION} llvm-as-${LLVM_VERSION_NODOT} llvm-as-${LLVM_VERSION_NOPATCH} llvm-as)
  find_program(LLVM_LINK_EXECUTABLE
               NAMES llvm-link-${LLVM_MAJOR_VERSION} llvm-link-${LLVM_VERSION_NODOT} llvm-link-${LLVM_VERSION_NOPATCH} llvm-link)
endif()

# Determine Clang Resource Directory (Critical for OpenCL internal headers)
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
        message(STATUS "Could not determine Clang resource include directory.")
        set(CLANG_INCLUDE_DIR "")
    endif()
else()
    message(WARNING "Clang executable not found. May have trouble finding Clang-specific headers.")
    set(CLANG_INCLUDE_DIR "")
endif()

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
  OUTPUT_VARIABLE LLVM_INCLUDE_DIR_MAIN
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(LLVM_INCLUDE_DIRS ${LLVM_INCLUDE_DIR_MAIN})
if(CLANG_INCLUDE_DIR)
  list(APPEND LLVM_INCLUDE_DIRS ${CLANG_INCLUDE_DIR})
endif()

# Fallback for older layouts or static paths
if(EXISTS "${LLVM_INCLUDE_DIR_MAIN}/clang")
  list(APPEND LLVM_INCLUDE_DIRS "${LLVM_INCLUDE_DIR_MAIN}/clang")
endif()
if(EXISTS "${LLVM_INCLUDE_DIR_MAIN}/clang-c")
  list(APPEND LLVM_INCLUDE_DIRS "${LLVM_INCLUDE_DIR_MAIN}/clang-c")
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
set(LLVM_CFLAGS ${LLVM_CFLAGS_FROM_CONFIG})
foreach(dir ${LLVM_INCLUDE_DIRS})
    set(LLVM_CFLAGS "${LLVM_CFLAGS} -I${dir}")
endforeach()

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
  OUTPUT_VARIABLE LLVM_LDFLAGS_FROM_CONFIG
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(LLVM_LDFLAGS ${LLVM_LDFLAGS_FROM_CONFIG})

# Get core LLVM libraries
execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs --link-static
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
    message(STATUS "LLVM Core Libraries (static used)")
else()
    set(LLVM_MODULE_LIBS ${LLVM_MODULE_LIBS_DYNAMIC})
    message(STATUS "LLVM Core Libraries (dynamic used)")
endif()

# Find libclang-cpp (Preferred for LLVM 9+)
set(CLANG_LIBRARIES "")
find_library(CLANG_CPP_LIB NAMES clang-cpp clang-cpp-${LLVM_MAJOR_VERSION} clang-cpp-${LLVM_VERSION_NODOT}
             HINTS ${LLVM_LIBRARY_DIR} ${LLVM_LIBRARY_DIR}/../lib)

if(CLANG_CPP_LIB)
  message(STATUS "Found Clang C++ library: ${CLANG_CPP_LIB}")
  set(CLANG_LIBRARIES ${CLANG_CPP_LIB})
else()
  # Fallback
  find_library(CLANG_GENERIC_LIB NAMES clang Clang
               HINTS ${LLVM_LIBRARY_DIR} ${LLVM_LIBRARY_DIR}/../lib)
  if(CLANG_GENERIC_LIB)
    message(STATUS "Found generic Clang library: ${CLANG_GENERIC_LIB}")
    set(CLANG_LIBRARIES ${CLANG_GENERIC_LIB})
  else()
    message(WARNING "Could not find libclang-cpp or libclang.")
  endif()
endif()

if(CLANG_LIBRARIES)
  set(LLVM_MODULE_LIBS "${LLVM_MODULE_LIBS} ${CLANG_LIBRARIES}")
endif()

# System libraries
execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --system-libs
  OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
if (LLVM_SYSTEM_LIBS)
  string(REGEX REPLACE "[\n\r]" "" LLVM_SYSTEM_LIBS "${LLVM_SYSTEM_LIBS}")
  message(STATUS "LLVM System Libraries: ${LLVM_SYSTEM_LIBS}")
  set(LLVM_MODULE_LIBS "${LLVM_MODULE_LIBS} ${LLVM_SYSTEM_LIBS}")
endif()

if (LLVM_INCLUDE_DIRS AND LLVM_LIBRARY_DIR AND LLVM_MODULE_LIBS)
  set(LLVM_FOUND TRUE)
else()
  set(LLVM_FOUND FALSE)
  message(WARNING "FindLLVM.cmake did not find all required LLVM components.")
endif()

MARK_AS_ADVANCED(LLVM_CONFIG_EXECUTABLE CLANG_EXECUTABLE LLVM_INCLUDE_DIR_MAIN CLANG_CPP_LIB)