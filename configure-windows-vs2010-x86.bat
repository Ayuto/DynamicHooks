mkdir .\Build\vs2010-x86
cd .\Build\vs2010-x86
cmake ..\.. -G"Visual Studio 10" -DBUILD_TYPE=test
pause