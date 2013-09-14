CMake_Minimum_Required(VERSION 2.8)
Project(DynamicHooks)

Set(HEADER_FILES
    src/api.h
    src/asm.h
    src/utilities.h
)

Set(SOURCE_FILES
    src/api.cpp
    src/asm.cpp
    src/utilities.cpp
)

Set(DYNAMICHOOKS_FILES
    ${HEADER_FILES}
    ${SOURCE_FILES}
)

Include_Directories(src/thirdparty/AsmJit/include)

# Build static library?
If(BUILD_TYPE MATCHES static)
    Add_Library(DynamicHooks STATIC ${DYNAMICHOOKS_FILES})
    
# Build shared library?
Elseif(BUILD_TYPE MATCHES shared)
    Add_Library(DynamicHooks SHARED ${DYNAMICHOOKS_FILES})
    
# Build test/executable?
Elseif(BUILD_TYPE MATCHES test)
    # Add test.cpp
    Set(DYNAMICHOOKS_FILES ${DYNAMICHOOKS_FILES} src/test.cpp)
    Add_Executable(DynamicHooks ${DYNAMICHOOKS_FILES})
Endif()

If(WIN32)
    Target_Link_Libraries(DynamicHooks
        debug src/thirdparty/AsmJit/lib/AsmJit_d
        optimized src/thirdparty/AsmJit/lib/AsmJit
    )
Else()
    If(CMAKE_BUILD_TYPE MATCHES Debug)
        Target_Link_Libraries(DynamicHooks ${CMAKE_SOURCE_DIR}/src/thirdparty/AsmJit/lib/libAsmJit_d.a)
    Else()
        Target_Link_Libraries(DynamicHooks ${CMAKE_SOURCE_DIR}/src/thirdparty/AsmJit/lib/libAsmJit.a)
    Endif()
Endif()
