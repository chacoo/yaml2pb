cmake_minimum_required(VERSION 3.12)

project(yaml2pb
    VERSION 1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_EXPORT_COMPILECOMMANDS ON)

include(FetchContent)
FetchContent_Declare(
    yaml-cpp
    URL https://github.com/jbeder/yaml-cpp/archive/refs/tags/yaml-cpp-0.7.0.tar.gz
)
FetchContent_Declare(
    protobuf
    URL https://github.com/protocolbuffers/protobuf/releases/download/v21.4/protobuf-cpp-3.21.4.tar.gz
)
FetchContent_MakeAvailable(yaml-cpp protobuf)

aux_source_directory(src YAML2PB_SRC)
add_library(libyaml2pb ${YAML2PB_SRC})
target_include_directories(libyaml2pb PRIVATE
    ${yaml-cpp_SOURCE_DIR}/include
)
target_include_directories(libyaml2pb PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${protobuf_SOURCE_DIR}/src
)

aux_source_directory(test YAML2PB_TEST_SRC)
add_executable(yaml2pb_test ${YAML2PB_TEST_SRC})
target_include_directories(yaml2pb_test PRIVATE
    ${protobuf_SOURCE_DIR}/third_party/googletest/googletest/include
)
target_link_libraries(yaml2pb_test libyaml2pb libprotobuf yaml-cpp gmock_main)

enable_testing()
add_test(NAME yaml2pb_test
    COMMAND $<TARGET_FILE:yaml2pb_test>
)
