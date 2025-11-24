cmake_minimum_required(VERSION 3.15)
project(CSProCompile VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories - Standalone structure
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

# Create standalone directory structure
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/resources/messages)
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/dependencies)

# Options
option(CSPRO_SDK_AVAILABLE "Build with real CSPro SDK integration" OFF)
option(BUILD_TESTS "Build unit tests" OFF)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/CSProCompile.cpp
    src/CompilerInterface.cpp
)

# Main executable
add_executable(CSProCompile ${SOURCES})

# Enable MFC for CSPro SDK (required by CSPro libraries)
if(MSVC AND CSPRO_SDK_AVAILABLE)
    set_target_properties(CSProCompile PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
    # Use MFC in a shared DLL
    target_compile_definitions(CSProCompile PRIVATE 
        _AFXDLL
        _WIN32_WINNT=0x0601  # Windows 7
        WINVER=0x0601
    )
    set(CMAKE_MFC_FLAG 2)  # 2 = Use MFC in a shared DLL
endif()

# Compiler flags
if(MSVC)
    target_compile_options(CSProCompile PRIVATE /W4 /EHsc)
    target_compile_definitions(CSProCompile PRIVATE 
        _CRT_SECURE_NO_WARNINGS
        _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
    )
else()
    target_compile_options(CSProCompile PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Link filesystem library (required for C++17 on some systems)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(CSProCompile stdc++fs)
endif()

# CSPro SDK Integration (when available)
if(CSPRO_SDK_AVAILABLE)
    message(STATUS "Building with CSPro SDK support")
    
    # Try to find CSPro SDK path
    if(DEFINED ENV{CSPRO_SDK_PATH})
        set(CSPRO_SDK_PATH $ENV{CSPRO_SDK_PATH})
    elseif(DEFINED ENV{CSPRO_PATH})
        set(CSPRO_SDK_PATH $ENV{CSPRO_PATH})
    endif()
    
    if(NOT CSPRO_SDK_PATH)
        message(WARNING "CSPRO_SDK_PATH not set. Trying default locations...")
        
        # Try common CSPro installation paths
        set(CSPRO_SEARCH_PATHS
            "${CMAKE_SOURCE_DIR}/cspro-dev/cspro"
            "C:/Program Files (x86)/CSPro 8.0"
            "C:/Program Files/CSPro 8.0"
            "C:/CSPro"
            "${CMAKE_SOURCE_DIR}/../../../cspro"
        )
        
        foreach(SEARCH_PATH ${CSPRO_SEARCH_PATHS})
            if(EXISTS "${SEARCH_PATH}")
                set(CSPRO_SDK_PATH "${SEARCH_PATH}")
                message(STATUS "Found CSPro at: ${CSPRO_SDK_PATH}")
                break()
            endif()
        endforeach()
    endif()
    
    if(CSPRO_SDK_PATH)
        message(STATUS "Using CSPro SDK from: ${CSPRO_SDK_PATH}")
        
        # Include directories for CSPro source headers
        include_directories(
            ${CSPRO_SDK_PATH}
            ${CSPRO_SDK_PATH}/engine
            ${CSPRO_SDK_PATH}/zBatchO
            ${CSPRO_SDK_PATH}/zEngineO
            ${CSPRO_SDK_PATH}/Wcompile
            ${CSPRO_SDK_PATH}/ZBRIDGEO
            ${CSPRO_SDK_PATH}/zToolsO
            ${CSPRO_SDK_PATH}/zAppO
            ${CSPRO_SDK_PATH}/zLogicO
            ${CSPRO_SDK_PATH}/zMessageO
            ${CSPRO_SDK_PATH}/zListingO
            ${CSPRO_SDK_PATH}/zDictO
            ${CSPRO_SDK_PATH}/zFormO
            ${CSPRO_SDK_PATH}/zCaseO
            ${CSPRO_SDK_PATH}/zDataO
            ${CSPRO_SDK_PATH}/Zsrcmgro
            ${CSPRO_SDK_PATH}/external
        )
        
        # Library directories - look for built libraries
        target_link_directories(CSProCompile PRIVATE
            ${PROJECT_SOURCE_DIR}/lib
            ${CSPRO_SDK_PATH}/Release/lib
            ${CSPRO_SDK_PATH}/Debug/lib
            ${CSPRO_SDK_PATH}/Release
            ${CSPRO_SDK_PATH}/Debug
        )
        
        # Link CSPro libraries - order matters!
        # Import libraries generated from CSPro 8.0 DLLs - use full paths
        target_link_libraries(CSProCompile
            ${PROJECT_SOURCE_DIR}/lib/ZBRIDGEO.lib
            ${PROJECT_SOURCE_DIR}/lib/zBatchO.lib
            ${PROJECT_SOURCE_DIR}/lib/zEngineO.lib
            ${PROJECT_SOURCE_DIR}/lib/zToolsO.lib
            ${PROJECT_SOURCE_DIR}/lib/zAppO.lib
            ${PROJECT_SOURCE_DIR}/lib/zLogicO.lib
            ${PROJECT_SOURCE_DIR}/lib/zMessageO.lib
            ${PROJECT_SOURCE_DIR}/lib/zListingO.lib
            ${PROJECT_SOURCE_DIR}/lib/zDictO.lib
            ${PROJECT_SOURCE_DIR}/lib/zFormO.lib
            ${PROJECT_SOURCE_DIR}/lib/zCaseO.lib
            ${PROJECT_SOURCE_DIR}/lib/zDataO.lib
            ${PROJECT_SOURCE_DIR}/lib/Zsrcmgro.lib
        )
        
        # Define compilation flag
        target_compile_definitions(CSProCompile PRIVATE 
            CSPRO_SDK_AVAILABLE
            CSPRO_REAL_COMPILATION
            WIN_DESKTOP
            _UNICODE
            UNICODE
        )
        
        # ========================================
        # STANDALONE APPLICATION SETUP
        # Copy all necessary files for standalone operation
        # ========================================
        
        # 1. Copy CSPro message files (.mgf) to resources/messages
        message(STATUS "Creating standalone application structure...")
        message(STATUS "Copying CSPro message files to resources/messages/...")
        
        file(GLOB CSPRO_MGF_FILES "${CSPRO_SDK_PATH}/*.mgf")
        
        if(CSPRO_MGF_FILES)
            foreach(MGF_FILE ${CSPRO_MGF_FILES})
                get_filename_component(MGF_FILENAME ${MGF_FILE} NAME)
                
                # Copy to resources/messages (for distribution)
                configure_file(
                    ${MGF_FILE}
                    ${PROJECT_SOURCE_DIR}/resources/messages/${MGF_FILENAME}
                    COPYONLY
                )
                
                # Copy to bin (for direct execution)
                configure_file(
                    ${MGF_FILE}
                    ${PROJECT_SOURCE_DIR}/bin/${MGF_FILENAME}
                    COPYONLY
                )
                
                message(STATUS "  Copied: ${MGF_FILENAME}")
            endforeach()
        else()
            message(WARNING "No MGF files found in ${CSPRO_SDK_PATH}")
            message(WARNING "CSProDesigner.mgf is REQUIRED for error messages!")
            message(WARNING "Without it, you will get 'Invalid message number' errors")
        endif()
        
        # 2. Copy CSPro DLL dependencies to lib folder
        message(STATUS "Copying CSPro library dependencies to lib/...")
        
        # Search for CSPro DLLs in Release and Debug folders
        set(CSPRO_LIB_SEARCH_PATHS
            "${CSPRO_SDK_PATH}/Release"
            "${CSPRO_SDK_PATH}/Debug"
            "${CSPRO_SDK_PATH}/lib"
            "${CSPRO_SDK_PATH}"
        )
        
        set(CSPRO_REQUIRED_LIBS
            zBatchO zEngineO Wcompile zBridgeO zToolsO zAppO
            zLogicO zMessageO zListingO zDictO zFormO zCaseO zDataO
        )
        
        foreach(LIB_NAME ${CSPRO_REQUIRED_LIBS})
            set(LIB_FOUND FALSE)
            
            foreach(SEARCH_PATH ${CSPRO_LIB_SEARCH_PATHS})
                # Check for DLL
                if(EXISTS "${SEARCH_PATH}/${LIB_NAME}.dll" AND NOT LIB_FOUND)
                    configure_file(
                        "${SEARCH_PATH}/${LIB_NAME}.dll"
                        ${PROJECT_SOURCE_DIR}/lib/${LIB_NAME}.dll
                        COPYONLY
                    )
                    configure_file(
                        "${SEARCH_PATH}/${LIB_NAME}.dll"
                        ${PROJECT_SOURCE_DIR}/bin/${LIB_NAME}.dll
                        COPYONLY
                    )
                    message(STATUS "  Copied DLL: ${LIB_NAME}.dll")
                    set(LIB_FOUND TRUE)
                endif()
                
                # Check for LIB (import library)
                if(EXISTS "${SEARCH_PATH}/${LIB_NAME}.lib" AND NOT LIB_FOUND)
                    configure_file(
                        "${SEARCH_PATH}/${LIB_NAME}.lib"
                        ${PROJECT_SOURCE_DIR}/lib/${LIB_NAME}.lib
                        COPYONLY
                    )
                    set(LIB_FOUND TRUE)
                endif()
            endforeach()
            
            if(NOT LIB_FOUND)
                message(WARNING "Could not find library: ${LIB_NAME}")
            endif()
        endforeach()
        
        # 3. Copy additional runtime dependencies
        message(STATUS "Copying runtime dependencies...")
        
        # Copy Visual C++ Runtime if needed
        file(GLOB VCRUNTIME_DLLS 
            "${CSPRO_SDK_PATH}/*.dll"
            "${CSPRO_SDK_PATH}/Release/*.dll"
        )
        
        foreach(DLL_FILE ${VCRUNTIME_DLLS})
            get_filename_component(DLL_FILENAME ${DLL_FILE} NAME)
            
            # Copy common runtime DLLs
            if(DLL_FILENAME MATCHES "^(msvcp|msvcr|vcruntime|ucrtbase)")
                configure_file(
                    ${DLL_FILE}
                    ${PROJECT_SOURCE_DIR}/lib/${DLL_FILENAME}
                    COPYONLY
                )
                configure_file(
                    ${DLL_FILE}
                    ${PROJECT_SOURCE_DIR}/bin/${DLL_FILENAME}
                    COPYONLY
                )
                message(STATUS "  Copied runtime: ${DLL_FILENAME}")
            endif()
        endforeach()
        
        # 4. Update runtime search path for the executable
        set_target_properties(CSProCompile PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib"
        )
        
        message(STATUS "Standalone application structure created successfully!")
        
    else()
        message(WARNING "Could not locate CSPro SDK. Build will succeed but real compilation will not work.")
        message(WARNING "Set CSPRO_SDK_PATH environment variable or use -DCSPRO_SDK_PATH=<path>")
    endif()
endif()

# Installation - Standalone package
install(TARGETS CSProCompile
    RUNTIME DESTINATION bin
)

# Install all dependencies for standalone operation
install(DIRECTORY ${PROJECT_SOURCE_DIR}/lib/
    DESTINATION lib
    FILES_MATCHING 
    PATTERN "*.dll"
    PATTERN "*.lib"
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/resources/
    DESTINATION resources
)

install(FILES 
    ${PROJECT_SOURCE_DIR}/README.md
    ${PROJECT_SOURCE_DIR}/GETTING_STARTED.md
    ${PROJECT_SOURCE_DIR}/MGF_INTEGRATION.md
    DESTINATION .
)

# Print configuration summary
message(STATUS "")
message(STATUS "=== CSProCompile Build Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "CSPro SDK Integration: ${CSPRO_SDK_AVAILABLE}")
if(CSPRO_SDK_AVAILABLE AND CSPRO_SDK_PATH)
    message(STATUS "CSPro SDK Path: ${CSPRO_SDK_PATH}")
endif()
message(STATUS "========================================")
message(STATUS "")
