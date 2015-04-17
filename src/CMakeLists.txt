# =============================================================================
# >> REQUIREMENTS & PROJECT
# =============================================================================
CMake_Minimum_Required(VERSION 2.8)
Project(DynamicHooks)


# =============================================================================
# >> SOURCE & HEADER FILES
# =============================================================================
Set(CONVENTION_HEADERS
    conventions/x86MsCdecl.h
    conventions/x86MsThiscall.h
    conventions/x86MsStdcall.h
    
    conventions/x86GccCdecl.h
    conventions/x86GccThiscall.h
)

Set(HEADER_FILES
    ${CONVENTION_HEADERS}
    asm.h
    convention.h
    hook.h
    manager.h
    registers.h
    utilities.h
)

Set(CONVENTION_SOURCES
    conventions/x86MsCdecl.cpp
    conventions/x86MsThiscall.cpp
    conventions/x86MsStdcall.cpp
)

Set(SOURCE_FILES
    ${CONVENTION_SOURCES}
    asm.cpp
    hook.cpp
    manager.cpp
    registers.cpp
    utilities.cpp
)

Set(DYNAMICHOOKS_FILES
    ${HEADER_FILES}
    ${SOURCE_FILES}
)


# =============================================================================
# >> SOURCE GROUPS
# =============================================================================
Source_Group("Header Files\\Conventions"    FILES ${CONVENTION_HEADERS})
Source_Group("Source Files\\Conventions"    FILES ${CONVENTION_SOURCES})


# =============================================================================
# >> COMPILER FLAGS
# =============================================================================
# Disable optimization in release mode
If(WIN32)
    Set(CMAKE_CXX_FLAGS_RELEASE "/Od /Ob0")
Endif()

Set(CMAKE_CXX_FLAGS "-m32")

# Create a static library
Add_Library(DynamicHooks STATIC ${DYNAMICHOOKS_FILES})


# =============================================================================
# >> INCLUDES
# =============================================================================
Include_Directories(thirdparty/AsmJit/include)


# =============================================================================
# >> LINK LIBRARIES
# =============================================================================
If(WIN32)
    Target_Link_Libraries(DynamicHooks ../../thirdparty/AsmJit/lib/AsmJit)
Else()
    Target_Link_Libraries(DynamicHooks ${CMAKE_SOURCE_DIR}/thirdparty/AsmJit/lib/libAsmJit.a)
Endif()