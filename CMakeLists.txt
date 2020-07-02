cmake_minimum_required (VERSION 2.8.11)
project (libmatrix-client)

set(source_files test.cpp)
add_executable(libmatrix-client ${source_files})

set_target_properties(libmatrix-client PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(libmatrix-client PROPERTIES CXX_STANDARD 17)

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
    add_compile_definitions(CPPRESTSDK)
endif()
