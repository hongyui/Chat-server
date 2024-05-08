#!/bin/bash
echo "configuring project..."
cmake -B build -S . > cmake_output.txt 2>&1
if [ $? -ne 0 ]; then  # Check if cmake command failed
    grep "vcpkg/scripts/buildsystems/vcpkg.cmake" cmake_output.txt > /dev/null
    if [ $? -ne 0 ]; then  # Check if the error is not related to vcpkg
        echo "cmake failed, see cmake_output.txt for details"
        read -p "Press any key to continue . . . " -n1 -s  # Equivalent to pause
        exit 1
    fi
    echo "Detected vcpkg error, running vcpkg_install.sh"
    bash "script/vcpkg_install.sh"  # Make sure this script has execute permissions
fi
echo "building project..."
cmake --build build
read -p "Press any key to continue . . . " -n1 -s  # Equivalent to pause
