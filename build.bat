@echo off
echo configuring project...
cmake -B build -S . > cmake_output.txt 2>&1 || (
    findstr /C:"vcpkg/scripts/buildsystems/vcpkg.cmake" cmake_output.txt >nul || (
        echo cmake failed, see cmake_output.txt for details
        pause
        exit /b
    )
    echo Detected vcpkg error, running vcpkg_install.bat
    call "script/vcpkg_install.bat"
)
echo building project...
cmake --build build
pause