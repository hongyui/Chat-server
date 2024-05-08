@echo off
:: init git submodule and install vcpkg
if exist .\vcpkg (
    echo gitmodule already init
) else (
    echo module not init, initing...
    git submodule update --init --recursive
)
:: check vcpkg is install or not
if exist .\vcpkg\vcpkg.exe (
    echo vcpkg already installed
) else (
    echo vcpkg not installed, bootstraping...
    .\vcpkg\bootstrap-vcpkg.bat
)