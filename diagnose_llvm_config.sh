#!/bin/bash
set -e
echo "--- llvm-config details for LLVM 18.1.3 ---"

echo ""
echo "1. llvm-config --version:"
llvm-config --version

echo ""
echo "2. llvm-config --prefix:"
llvm-config --prefix

echo ""
echo "3. llvm-config --includedir:"
llvm-config --includedir

echo ""
echo "4. llvm-config --libdir:"
llvm-config --libdir

echo ""
echo "5. llvm-config --cflags:"
llvm-config --cflags

echo ""
echo "6. llvm-config --ldflags:"
llvm-config --ldflags

echo ""
echo "7. llvm-config --libs (default):"
llvm-config --libs

echo ""
echo "8. llvm-config --libs core mcjit codegen bitreader bitwriter linker irreader:"
llvm-config --libs core mcjit codegen bitreader bitwriter linker irreader

echo ""
echo "9. llvm-config --system-libs:"
llvm-config --system-libs

echo ""
echo "10. Checking for presence of libclang-cpp.so in llvm-config --libdir"
LLVM_LIBDIR=$(llvm-config --libdir)
if [ -f "$LLVM_LIBDIR/libclang-cpp.so" ]; then
    echo "Found $LLVM_LIBDIR/libclang-cpp.so"
else
    echo "$LLVM_LIBDIR/libclang-cpp.so not found."
    echo "Listing files in $LLVM_LIBDIR that contain 'clang':"
    find "$LLVM_LIBDIR" -name "*clang*" -print
fi

echo ""
echo "11. Attempting to get Clang specific libs (behavior might vary with llvm-config version):"
if llvm-config --help | grep -q "clang"; then
    echo "llvm-config appears to have options for 'clang' components."
    # Attempt to list clang libraries, this syntax might not be supported or might need specific component names
    llvm-config --libs clang 2>/dev/null || echo "llvm-config --libs clang failed or produced no output."
else
    echo "llvm-config --help does not show specific 'clang' component options for --libs."
fi

echo ""
echo "--- End of llvm-config details ---"
