# =============================================================================
# >> REQUIREMENTS & PROJECT
# =============================================================================
CMake_Minimum_Required(VERSION 2.8)
Project(Tests)


# =============================================================================
# >> MACROS
# =============================================================================
# Create a new macro to easily add test executables
macro (create_dynamic_hooks_test name)
    add_executable(${ARGV})
    if (TARGET ${name})
        If(WIN32)
            Target_Link_Libraries(${name} ../vs2010-x86/Release/DynamicHooks)
            Target_Link_Libraries(${name} ../../src/thirdparty/AsmJit/lib/AsmJit)
        Else()
            Target_Link_Libraries(${name} $ENV{PWD}/../unix-x86/libDynamicHooks.a)
            Target_Link_Libraries(${name} $ENV{PWD}/../../src/thirdparty/AsmJit/lib/libAsmJit.a)
        Endif()
    endif()
endmacro()


# =============================================================================
# >> COMPILER FLAGS
# =============================================================================
# Disable optimization in release mode
If(WIN32)
    Set(CMAKE_CXX_FLAGS_RELEASE "/Od /Ob0")
Endif()


# =============================================================================
# >> INCLUDES
# =============================================================================
Include_Directories(../src)
Include_Directories(../src/thirdparty/AsmJit/include)


# =============================================================================
# >> TESTS
# =============================================================================
If(WIN32)
    create_dynamic_hooks_test(MS_CDECL1 ms_cdecl1.cpp)
    create_dynamic_hooks_test(MS_CDECL2 ms_cdecl2.cpp)
    create_dynamic_hooks_test(MS_THISCALL1 ms_thiscall1.cpp)
Else()
    create_dynamic_hooks_test(GCC_CDECL1 gcc_cdecl1.cpp)
Endif()