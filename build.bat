@echo off
echo configuring project...
PowerShell -Command "cmake -B build -S . 2>&1 | Tee-Object -FilePath cmake_output.txt" || (
    findstr /C:"vcpkg/scripts/buildsystems/vcpkg.cmake" cmake_output.txt >nul || (
        echo cmake failed, see cmake_output.txt for details
        pause
        exit /b
    )
    echo Detected vcpkg error, running vcpkg_install.bat
    PowerShell -Command "script/vcpkg_install.bat 2>&1 | Tee-Object -FilePath CON"
)

echo building project...
cmake --build build
pause