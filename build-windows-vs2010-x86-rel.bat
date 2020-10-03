mkdir .\Build\vs2010-x86
cd .\Build\vs2010-x86
cmake ..\..\src -G"Visual Studio 10" || goto :error
msbuild DynamicHooks.sln /p:Configuration=Release /p:VCTargetsPath="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0" || goto :error
cd ..\..

mkdir .\Build\vs2010-x86-tests
cd .\Build\vs2010-x86-tests
cmake ..\..\tests -G"Visual Studio 10" || goto :error
msbuild Tests.sln /p:Configuration=Release /p:VCTargetsPath="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0" || goto :error
cd ..\..

python run_tests.py Build/vs2010-x86-tests/Release || goto :error
goto :EOF

:error
echo Failed with error %errorlevel%.
exit /b %errorlevel%