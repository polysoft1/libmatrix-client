cmake_minimum_required (VERSION 2.8.11)
project (libmatrix-tests)

# Options
option(USE_CONAN "Use the Conan package manager" on)
option(USE_CPPRESTSDK "Use CPPRESTSDK for HTTP" on)

set(source_files HTTPTests.cpp ../src/CPPRESTSDKClient.cpp ../src/HTTP.cpp)
add_executable(libmatrix-tests ${source_files})

set_target_properties(libmatrix-tests PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(libmatrix-tests PROPERTIES CXX_STANDARD 17)

target_include_directories(libmatrix-tests PUBLIC "../include/libmatrix-client")

# define the project
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
elseif(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif()

if(USE_CONAN)
    include(${CMAKE_CURRENT_SOURCE_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)

    target_include_directories(libmatrix-tests PRIVATE ${CONAN_INCLUDE_DIRS})
    conan_target_link_libraries(libmatrix-tests)

    add_compile_definitions(LIBMATRIX_USE_CPPRESTSDK)
endif()
