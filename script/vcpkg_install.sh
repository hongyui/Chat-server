#!/bin/bash
# init git submodule and install vcpkg

# Check if vcpkg directory exists
if [ -d "./vcpkg" ]; then
    echo "gitmodule already init"
else
    echo "module not init, initing..."
    git submodule update --init --recursive
fi

# Check if vcpkg is installed
if [ -f "./vcpkg/vcpkg" ]; then
    echo "vcpkg already installed"
else
    echo "vcpkg not installed, bootstraping..."
    sh ../vcpkg/bootstrap-vcpkg.sh
fi
