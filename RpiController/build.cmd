@echo off
cd %~dp0
if not exist build mkdir build
cd build
cmake -G "Visual Studio 16 2019" -Thost=x64 -A x64 ..
cmake --build . --config Release -- /m /verbosity:minimal 