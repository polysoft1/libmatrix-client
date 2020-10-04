cmake_minimum_required (VERSION 2.8.11)
project (libmatrix-client)

set(OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/target/)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR})

# Options
option(USE_CONAN "Use the Conan package manager" on)
option(USE_CPPRESTSDK "Use CPPRESTSDK for HTTP" on)
option(USE_CUSTOM_HTTP "Use custom HTTP session" off)

set(source_files src/CPPRESTSDKClient.cpp src/CustomHTTPClient.cpp src/HTTP.cpp
    src/MatrixSession.cpp src/utils/MessageUtils.cpp src/utils/UserUtils.cpp)
add_library(libmatrix-client SHARED ${source_files})
add_library(libmatrix-client-static STATIC ${source_files})
add_executable(driver ${source_files} test.cpp)

set_target_properties(libmatrix-client PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(libmatrix-client PROPERTIES CXX_STANDARD 17)
set_target_properties(libmatrix-client-static PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(libmatrix-client-static PROPERTIES CXX_STANDARD 17)
set_target_properties(driver PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(driver PROPERTIES CXX_STANDARD 17)

target_include_directories(libmatrix-client PUBLIC "include/libmatrix-client")
target_include_directories(libmatrix-client-static PUBLIC "include/libmatrix-client")
target_include_directories(driver PUBLIC "include/libmatrix-client")

# define the project
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
elseif(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif()

IF (WIN32)
    target_compile_definitions(libmatrix-client PUBLIC BUILD_SHARED_LIBS)
    ADD_DEFINITIONS("-DPolyChat_EXPORT")
ENDIF (WIN32)

if(USE_CONAN)
    include(${CMAKE_CURRENT_SOURCE_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)

    target_include_directories(libmatrix-client PRIVATE ${CONAN_INCLUDE_DIRS})
    target_include_directories(libmatrix-client-static PRIVATE ${CONAN_INCLUDE_DIRS})
    target_include_directories(driver PRIVATE ${CONAN_INCLUDE_DIRS})
    conan_target_link_libraries(libmatrix-client)
    conan_target_link_libraries(libmatrix-client-static)
    conan_target_link_libraries(driver)
else()
    if (USE_CPPRESTSDK)
        find_package(cpprestsdk CONFIG REQUIRED)
        target_link_libraries(libmatrix-client PRIVATE cpprestsdk::cpprest)
        target_link_libraries(libmatrix-client-static PRIVATE cpprestsdk::cpprest)
    endif()

    find_package(nlohmann_json CONFIG REQUIRED)
    target_link_libraries(libmatrix-client PRIVATE nlohmann_json nlohmann_json::nlohmann_json)
    target_link_libraries(libmatrix-client-static PRIVATE nlohmann_json nlohmann_json::nlohmann_json)

    find_package(fmt CONFIG REQUIRED)
    target_link_libraries(libmatrix-client PRIVATE fmt::fmt fmt::fmt-header-only)
    target_link_libraries(libmatrix-client-static PRIVATE fmt::fmt fmt::fmt-header-only)
endif()

if (USE_CPPRESTSDK)
    add_compile_definitions(USE_CPPRESTSDK)
elseif (USE_CUSTOM_HTTP_SESSION)
    add_compile_definitions(USE_CUSTOM_HTTP_SESSION)
endif()
