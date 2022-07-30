cmake_minimum_required(VERSION 3.10)

project(transport_catalogue CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto)

file(GLOB CXX_FILES "*.h" "*.cpp")
# message(STATUS "found source files ${CXX_FILES}")

add_executable(transport_catalogue ${PROTO_HDRS} ${PROTO_SRCS} ${CXX_FILES})
target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_link_libraries(transport_catalogue ${Protobuf_LIBRARY_DEBUG} Threads::Threads)
else()
    target_link_libraries(transport_catalogue ${Protobuf_LIBRARY} Threads::Threads)
endif()



