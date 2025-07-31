#
# Try to find ocl_icd library and include path.
# Once done this will define
#
# OCLIcd_FOUND
# OCLIcd_INCLUDE_PATH
#

# Try to find CL/cl_icd.h first (common location, e.g. /usr/include/CL/cl_icd.h)
# Then try ocl_icd.h directly in the search paths.
FIND_PATH(OCLIcd_INCLUDE_PATH
  NAMES CL/cl_icd.h ocl_icd.h
  PATHS
    ~/include/
    /usr/include/
    /usr/local/include/
    /sw/include/
    /opt/local/include/
  DOC "The directory where OpenCL ICD headers reside (e.g., ocl_icd.h or CL/cl_icd.h)")

# If OCLIcd_INCLUDE_PATH is set (e.g. /usr/include if /usr/include/CL/cl_icd.h is found),
# then the header is considered found. The actual #include in C++ code would be <CL/cl_icd.h>.
# The INCLUDE_DIRECTORIES command should add the parent directory (e.g. /usr/include) to the search paths.

IF(OCLIcd_INCLUDE_PATH)
  INCLUDE_DIRECTORIES(${OCLIcd_INCLUDE_PATH})
  SET(OCLIcd_FOUND 1 CACHE STRING "Set to 1 if OCLIcd is found, 0 otherwise")
ELSE(OCLIcd_INCLUDE_PATH)
  SET(OCLIcd_FOUND 0 CACHE STRING "Set to 1 if OCLIcd is found, 0 otherwise")
ENDIF(OCLIcd_INCLUDE_PATH)

MARK_AS_ADVANCED(OCLIcd_FOUND)
