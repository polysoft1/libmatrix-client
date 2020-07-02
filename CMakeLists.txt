cmake_minimum_required (VERSION 2.8.11)
project (libmatrix-client)

set(source_files test.cpp)
add_executable(libmatrix-client ${source_files})

if(USE_CONAN)
    include(${CMAKE_CURRENT_SOURCE_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)

    target_include_directories(libmatrix-client PRIVATE ${CONAN_INCLUDE_DIRS})
    conan_target_link_libraries(libmatrix-client)
    add_compile_definitions(CPPRESTSDK)
endif()
