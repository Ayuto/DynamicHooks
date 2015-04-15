mkdir .\Build\vs2010-x86
cd .\Build\vs2010-x86
cmake ..\..\src -G"Visual Studio 10"
msbuild DynamicHooks.sln /p:Configuration=Release /p:VCTargetsPath="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0"
cd ..\..

mkdir .\Build\vs2010-x86-tests
cd .\Build\vs2010-x86-tests
cmake ..\..\tests -G"Visual Studio 10"
msbuild Tests.sln /p:Configuration=Release /p:VCTargetsPath="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0"
cd ..\..

python run_tests.py Build/vs2010-x86-tests/Release