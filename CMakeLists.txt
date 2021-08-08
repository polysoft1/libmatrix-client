cmake_minimum_required(VERSION 3.0)
project(libmatrix-client VERSION 0.0.1 LANGUAGES CXX)

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
option(USE_SYSTEM_OLM "Use the system version of OLM" off)
option(OLM_TESTS "Generate tests for OLM (fetching from source only)" off)
option(BUILD_DRIVERS "Build drivers for testing code" off)

set(source_files src/CPPRESTSDKClient.cpp src/CustomHTTPClient.cpp src/HTTP.cpp
	src/MatrixSession.cpp src/utils/MessageUtils.cpp src/utils/UserUtils.cpp
    src/encryption/Account.cpp src/utils/EncryptionUtils.cpp src/encryption/MegOlmSession.cpp
    src/Room.cpp)
add_library(libmatrix-client SHARED ${source_files})
add_library(libmatrix-client-static STATIC ${source_files})


set_target_properties(libmatrix-client PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(libmatrix-client PROPERTIES CXX_STANDARD 17)
set_target_properties(libmatrix-client-static PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(libmatrix-client-static PROPERTIES CXX_STANDARD 17)

target_include_directories(libmatrix-client PUBLIC "include/libmatrix-client")
target_include_directories(libmatrix-client-static PUBLIC "include/libmatrix-client")

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


if(USE_SYSTEM_OLM)
    find_package(Olm 3)
	set_package_properties(Olm PROPERTIES
		DESCRIPTION "An implementation of the Double Ratchet cryptographic ratchet"
		URL "https://git.matrix.org/git/olm/about/"
		TYPE REQUIRED
    )
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "3.14.5")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
    SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})

    include(FetchContent)
    FetchContent_Declare(
        Olm
        GIT_REPOSITORY https://gitlab.matrix.org/matrix-org/olm.git
        GIT_TAG        3.1.4
    )
    FetchContent_MakeAvailable(Olm)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
    SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR})
    SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR})
else()
    message( FATAL_ERROR "CMake Version insufficient!  Either install libolm manually or update CMake!")
endif()

if(BUILD_DRIVERS)
	add_executable(driver-static test.cpp)
	add_executable(driver-shared test.cpp)

	set_target_properties(driver-static PROPERTIES LINKER_LANGUAGE CXX)
	set_target_properties(driver-static PROPERTIES CXX_STANDARD 17)
	target_include_directories(driver-static PUBLIC "include/libmatrix-client")
	set_target_properties(driver-shared PROPERTIES LINKER_LANGUAGE CXX)
	set_target_properties(driver-shared PROPERTIES CXX_STANDARD 17)
	target_include_directories(driver-shared PUBLIC "include/libmatrix-client")

	target_link_libraries(driver-static PRIVATE libmatrix-client-static)
	target_link_libraries(driver-shared PRIVATE libmatrix-client)

	target_link_libraries(driver-shared PRIVATE Olm::Olm)
	target_link_libraries(driver-static PRIVATE Olm::Olm)
endif()

target_link_libraries(libmatrix-client PRIVATE Olm::Olm)
target_link_libraries(libmatrix-client-static PRIVATE Olm::Olm)

if(USE_CONAN)
    include(${CMAKE_CURRENT_SOURCE_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)

    target_include_directories(libmatrix-client PRIVATE ${CONAN_INCLUDE_DIRS})
    target_include_directories(libmatrix-client-static PRIVATE ${CONAN_INCLUDE_DIRS})
    if(BUILD_DRIVERS)
        target_include_directories(driver-static PRIVATE ${CONAN_INCLUDE_DIRS})
        conan_target_link_libraries(driver-static)
        target_include_directories(driver-shared PRIVATE ${CONAN_INCLUDE_DIRS})
        conan_target_link_libraries(driver-shared)
    endif()

    conan_target_link_libraries(libmatrix-client)
    conan_target_link_libraries(libmatrix-client-static)
else()
    if (USE_CPPRESTSDK)
        find_package(cpprestsdk CONFIG REQUIRED)
        target_link_libraries(libmatrix-client PRIVATE cpprestsdk::cpprest)
        target_link_libraries(libmatrix-client-static PRIVATE cpprestsdk::cpprest)
	if(BUILD_DRIVERS)
		target_link_libraries(driver-static PRIVATE cpprestsdk::cpprest)
		target_link_libraries(driver-shared PRIVATE cpprestsdk::cpprest)
	endif()
    endif()

    find_package(nlohmann_json CONFIG REQUIRED)
    target_link_libraries(libmatrix-client PRIVATE nlohmann_json nlohmann_json::nlohmann_json)
    target_link_libraries(libmatrix-client-static PRIVATE nlohmann_json nlohmann_json::nlohmann_json)
    if(BUILD_DRIVERS)
	    target_link_libraries(driver-static PRIVATE nlohmann_json nlohmann_json::nlohmann_json)
	    target_link_libraries(driver-shared PRIVATE nlohmann_json nlohmann_json::nlohmann_json)
    endif()

    find_package(fmt CONFIG REQUIRED)
    target_link_libraries(libmatrix-client PRIVATE fmt::fmt fmt::fmt-header-only)
    target_link_libraries(libmatrix-client-static PRIVATE fmt::fmt fmt::fmt-header-only)
    if(BUILD_DRIVERS)
        target_link_libraries(driver-static PRIVATE fmt::fmt fmt::fmt-header-only)
        target_link_libraries(driver-shared PRIVATE fmt::fmt fmt::fmt-header-only)
    endif()
endif()

if (WIN32)
    target_compile_definitions(libmatrix-client PUBLIC BUILD_SHARED_LIBS)
    target_compile_definitions(libmatrix-client PUBLIC LibMatrix_EXPORT)
endif()

if (USE_CPPRESTSDK)
    add_compile_definitions(LIBMATRIX_USE_CPPRESTSDK)
elseif (USE_CUSTOM_HTTP)
    add_compile_definitions(LIBMATRIX_USE_CUSTOM_HTTP_SESSION)
endif()
