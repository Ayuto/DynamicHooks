CMake_Minimum_Required(VERSION 2.8)
Project(Tests)

# Disable optimization in release mode
Set(CMAKE_CXX_FLAGS_RELEASE "/Od /Ob0")

Include_Directories(../src)
Include_Directories(../src/thirdparty/AsmJit/include)

add_executable(MS_CDECL ms_cdecl.cpp)

target_link_libraries(MS_CDECL ../vs2010-x86/Release/DynamicHooks)

# Set the thirdparty link libraries
If(WIN32)
    Target_Link_Libraries(MS_CDECL
        debug ../../src/thirdparty/AsmJit/lib/AsmJit_d
        optimized ../../src/thirdparty/AsmJit/lib/AsmJit
    )
Else()
    If(CMAKE_BUILD_TYPE MATCHES Debug)
        Target_Link_Libraries(MS_CDECL ../../src/thirdparty/AsmJit/lib/libAsmJit_d.a)
    Else()
        Target_Link_Libraries(MS_CDECL ../../src/thirdparty/AsmJit/lib/libAsmJit.a)
    Endif()
Endif()