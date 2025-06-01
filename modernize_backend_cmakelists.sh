#!/bin/bash
set -e

FILE_PATH="backend/src/CMakeLists.txt"
echo "--- Modernizing $FILE_PATH ---"

# Create a backup
cp "$FILE_PATH" "${FILE_PATH}.bak_before_modernize"

# Use awk to perform the targeted replacements more safely than sed for complex logic.
# We'll build the new file line by line.

awk '
BEGIN {
    print "# Modernized CMakeLists.txt for backend/src"
    print ""
    # Variables to track state for gbe target
    gbe_target_defined = 0
    gbe_includes_added = 0
    gbe_cpp_standard_set = 0
}

# Preserve initial variable settings and configure_file
/set \(OCL_BITCODE_BIN / || /set \(OCL_HEADER_DIR / || /set \(OCL_PCH_OBJECT / || /set \(GBE_OBJECT_DIR / || /set \(INTERP_OBJECT_DIR / || /if \(ENABLE_OPENCL_20\)/, /endif \(ENABLE_OPENCL_20\)/ || /configure_file \(GBEConfig.h.in GBEConfig.h\)/ {
    print
    next
}

# Preserve libocl subdirectory addition
/add_subdirectory\(libocl\)/ {
    print
    next
}

# Preserve PARENT_SCOPE variable settings
/set \(LOCAL_GBE_OBJECT_DIR.*PARENT_SCOPE\)/ || /set \(LOCAL_INTERP_OBJECT_DIR.*PARENT_SCOPE\)/ || /set \(LOCAL_OCL_BITCODE_BIN.*PARENT_SCOPE\)/ || /set \(LOCAL_OCL_HEADER_DIR.*PARENT_SCOPE\)/ || /set \(LOCAL_OCL_PCH_OBJECT.*PARENT_SCOPE\)/ {
    print
    next
}
# Preserve ENABLE_OPENCL_20 block for PARENT_SCOPE vars
/if \(ENABLE_OPENCL_20\)/ {
    in_ocl20_parent_scope_block = 1
}
/endif \(ENABLE_OPENCL_20\)/ && in_ocl20_parent_scope_block {
    print
    in_ocl20_parent_scope_block = 0
    next
}
in_ocl20_parent_scope_block { print; next }


# Preserve GBE_SRC definition
/set \(GBE_SRC/, /\)/ {
    print
    next
}

# Preserve GBE_LINK_LIBRARIES definition
/set \(GBE_LINK_LIBRARIES/, /\)/ {
    print
    next
}

# Remove include_directories(.) - will be handled by target_include_directories
/include_directories \(\.\)/ {
    print "# Removed: " $0
    next
}

# Remove link_directories - will be handled by linker flags from llvm-config and target_link_libraries
/link_directories \(${LLVM_LIBRARY_DIRS} \${DRM_LIBDIR}\)/ {
    print "# Removed: " $0
    next
}
/link_directories \(${LLVM_LIBRARY_DIR} \${DRM_LIBDIR}\)/ {
    print "# Removed: " $0
    next
}

# Remove include_directories(${LLVM_INCLUDE_DIRS}) - will be handled by target_include_directories
/include_directories\(\${LLVM_INCLUDE_DIRS}\)/ {
    print "# Removed: " $0
    next
}

# Handle gbe library
# add_library (gbe SHARED ${GBE_SRC})
/add_library \(gbe SHARED \${GBE_SRC}\)/ {
    print
    gbe_target_defined = 1
    # Add target-specific properties immediately after definition
    print "if(gbe)"
    print "    set_target_properties(gbe PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)"
    print "    target_include_directories(gbe PRIVATE . ${LLVM_INCLUDE_DIRS} ${DRM_INCLUDE_DIRS})"
    print "    # Potentially add other include dirs if GBE_SRC implies them from subdirs"
    print "    # Global compile definitions from root CMakeLists might apply or need to be target_compile_definitions"
    print "endif()"
    gbe_includes_added = 1
    gbe_cpp_standard_set = 1
    next
}

# target_link_libraries(gbe ${GBE_LINK_LIBRARIES}) - this is already good
/target_link_libraries\(gbe \${GBE_LINK_LIBRARIES}\)/ {
    print
    next
}

# add_dependencies(gbe beignet_bitcode) - this is fine
/add_dependencies\(gbe beignet_bitcode\)/ {
    print
    next
}

# Handle gbeinterp library
/add_library\(gbeinterp SHARED gbe_bin_interpreter.cpp\)/ {
    print
    print "if(gbeinterp)"
    print "    set_target_properties(gbeinterp PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)"
    print "endif()"
    next
}

# Handle gbe_bin_generater executable
/ADD_EXECUTABLE\(gbe_bin_generater gbe_bin_generater.cpp \${GBE_SRC}\)/ {
    print
    print "if(gbe_bin_generater)"
    print "    set_target_properties(gbe_bin_generater PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)"
    print "    # If GBE_SRC is used, it might need similar include_directories as gbe"
    print "    target_include_directories(gbe_bin_generater PRIVATE . ${LLVM_INCLUDE_DIRS} ${DRM_INCLUDE_DIRS})"
    print "endif()"
    next
}
/ADD_EXECUTABLE\(gbe_bin_generater gbe_bin_generater.cpp\)/ {
    # This is the one that links gbe, so it implies gbe includes are used via linkage
    print
    print "if(gbe_bin_generater)"
    print "    set_target_properties(gbe_bin_generater PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)"
    print "endif()"
    next
}
# TARGET_LINK_LIBRARIES for gbe_bin_generater - already good
/TARGET_LINK_LIBRARIES\(gbe_bin_generater .*\)/ {
    print
    next
}


# Preserve conditional blocks and other commands not explicitly handled
{ print }

END {
    # Final checks if properties were not added for some reason (e.g. target definition not found)
    if (gbe_target_defined && !gbe_includes_added) {
        print "# Fallback: Target gbe was defined but includes not added via awk script - check manually"
    }
}
' "$FILE_PATH" > "${FILE_PATH}.modern"

# Verify the new file looks reasonable (e.g., not empty)
if [ -s "${FILE_PATH}.modern" ]; then
    mv "${FILE_PATH}.modern" "$FILE_PATH"
    echo "File $FILE_PATH has been updated with modern CMake practices."
    echo "Showing diff:"
    diff -u "${FILE_PATH}.bak_before_modernize" "$FILE_PATH" || true
else
    echo "Error: Modernized file ${FILE_PATH}.modern is empty or not created. No changes made."
    exit 1
fi

echo "--- $FILE_PATH modernization attempt complete ---"
