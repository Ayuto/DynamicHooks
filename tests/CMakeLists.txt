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
        target_link_libraries(${name} ../vs2010-x86/Release/DynamicHooks)
        If(WIN32)
            Target_Link_Libraries(${name}
                debug ../../src/thirdparty/AsmJit/lib/AsmJit_d
                optimized ../../src/thirdparty/AsmJit/lib/AsmJit
            )
        Else()
            If(CMAKE_BUILD_TYPE MATCHES Debug)
                Target_Link_Libraries(${name} ../../src/thirdparty/AsmJit/lib/libAsmJit_d.a)
            Else()
                Target_Link_Libraries(${name} ../../src/thirdparty/AsmJit/lib/libAsmJit.a)
            Endif()
        Endif()
    endif()
endmacro()


# =============================================================================
# >> COMPILER FLAGS
# =============================================================================
# Disable optimization in release mode
Set(CMAKE_CXX_FLAGS_RELEASE "/Od /Ob0")


# =============================================================================
# >> INCLUDES
# =============================================================================
Include_Directories(../src)
Include_Directories(../src/thirdparty/AsmJit/include)


# =============================================================================
# >> TESTS
# =============================================================================
create_dynamic_hooks_test(MS_CDECL1 ms_cdecl1.cpp)
create_dynamic_hooks_test(MS_CDECL2 ms_cdecl2.cpp)
create_dynamic_hooks_test(MS_THISCALL1 ms_thiscall1.cpp)