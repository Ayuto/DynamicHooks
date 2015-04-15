CMake_Minimum_Required(VERSION 2.8)
Project(DynamicHooks)

Set(CONVENTION_HEADERS
    conventions/x86MsCdecl.h
    conventions/x86GccCdecl.h

    conventions/x86MsThiscall.h
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


# Create groups
Source_Group("Header Files\\Conventions"    FILES ${CONVENTION_HEADERS})
Source_Group("Source Files\\Conventions"    FILES ${CONVENTION_SOURCES})

# Set the directories to include
Include_Directories(thirdparty/AsmJit/include)

# Disable optimization in release mode
Set(CMAKE_CXX_FLAGS_RELEASE "/Od /Ob0")

# Tell cmake to build a static library
Add_Library(DynamicHooks STATIC ${DYNAMICHOOKS_FILES})

# Set the thirdparty link libraries
If(WIN32)
    Target_Link_Libraries(DynamicHooks
        debug ../../thirdparty/AsmJit/lib/AsmJit_d
        optimized ../../thirdparty/AsmJit/lib/AsmJit
    )
Else()
    If(CMAKE_BUILD_TYPE MATCHES Debug)
        Target_Link_Libraries(DynamicHooks ${CMAKE_SOURCE_DIR}/thirdparty/AsmJit/lib/libAsmJit_d.a)
    Else()
        Target_Link_Libraries(DynamicHooks ${CMAKE_SOURCE_DIR}/thirdparty/AsmJit/lib/libAsmJit.a)
    Endif()
Endif()