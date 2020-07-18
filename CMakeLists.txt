cmake_minimum_required (VERSION 2.8.11)
project (libmatrix-client)

# Options
option(USE_CONAN "Use the Conan package manager" on)
option(USE_CPPRESTSDK "Use CPPRESTSDK for HTTP" on)

set(source_files test.cpp src/CPPRESTSDKClient.cpp src/HTTP.cpp src/MatrixSession.cpp)
add_executable(libmatrix-client ${source_files})

set_target_properties(libmatrix-client PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(libmatrix-client PROPERTIES CXX_STANDARD 17)

target_include_directories(libmatrix-client PUBLIC "include/libmatrix-client")

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

    target_include_directories(libmatrix-client PRIVATE ${CONAN_INCLUDE_DIRS})
    conan_target_link_libraries(libmatrix-client)

    if (USE_CPPRESTSDK)
        add_compile_definitions(USE_CPPRESTSDK)
    endif()
endif()
