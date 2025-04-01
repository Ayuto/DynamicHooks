mkdir .\Build\vs2022-x86
cd .\Build\vs2022-x86
cmake ..\..\src -G"Visual Studio 17" -A Win32 -DASMJIT_STATIC=1 || goto :error
::msbuild DynamicHooks.sln /p:Configuration=Release || goto :error
cd ..\..

mkdir .\Build\vs2022-x86-tests
cd .\Build\vs2022-x86-tests
cmake ..\..\tests -G"Visual Studio 17" -A Win32 -DASMJIT_STATIC=1 || goto :error
::msbuild Tests.sln /p:Configuration=Release || goto :error
cd ..\..

::python run_tests.py Build/vs2022-x86-tests/Release || goto :error
goto :EOF

:error
echo Failed with error %errorlevel%.
exit /b %errorlevel%