# =============================================================================
# >> REQUIREMENTS & PROJECT
# =============================================================================
CMake_Minimum_Required(VERSION 3.5)
Project(Tests)


# =============================================================================
# >> MACROS
# =============================================================================
# Create a new macro to easily add test executables
macro (create_dynamic_hooks_test name)
    add_executable(${ARGV})
    if (TARGET ${name})
        If(WIN32)
            Target_Link_Libraries(${name} ../vs2022-x86/Release/DynamicHooks)
            Target_Link_Libraries(${name} ../../src/thirdparty/AsmJit/lib/AsmJit)
        Else()
            Target_Link_Libraries(${name} $ENV{PWD}/../unix-x86/libDynamicHooks.a)
            Target_Link_Libraries(${name} $ENV{PWD}/../../src/thirdparty/AsmJit/lib/libAsmJit.a)
            Target_Link_Libraries(${name} rt)
        Endif()
    endif()
endmacro()


# =============================================================================
# >> COMPILER FLAGS
# =============================================================================
If(WIN32)
    # Disable optimization in release mode
    Set(CMAKE_CXX_FLAGS_RELEASE "/Od /Ob0")
Else()
    Set(CMAKE_CXX_FLAGS "-m32")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lrt")
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
    create_dynamic_hooks_test(test_ms_cdecl1 ms_cdecl1.cpp)
    create_dynamic_hooks_test(test_ms_cdecl2 ms_cdecl2.cpp)
    create_dynamic_hooks_test(test_ms_thiscall1 ms_thiscall1.cpp)
    create_dynamic_hooks_test(test_ms_thiscall2 ms_thiscall2.cpp)
    create_dynamic_hooks_test(test_ms_stdcall1 ms_stdcall1.cpp)
    create_dynamic_hooks_test(test_ms_stdcall2 ms_stdcall2.cpp)
    create_dynamic_hooks_test(test_ms_fastcall1 ms_fastcall1.cpp)
    create_dynamic_hooks_test(test_ms_fastcall2 ms_fastcall2.cpp)
Else()
    create_dynamic_hooks_test(test_gcc_cdecl1 gcc_cdecl1.cpp)
    create_dynamic_hooks_test(test_gcc_cdecl2 gcc_cdecl2.cpp)
    create_dynamic_hooks_test(test_gcc_thiscall1 gcc_thiscall1.cpp)
    create_dynamic_hooks_test(test_gcc_thiscall2 gcc_thiscall2.cpp)
Endif()