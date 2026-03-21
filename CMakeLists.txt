cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_COMPILER "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.44.35207/bin/HostX64/x86/cl.exe")
set(CMAKE_CXX_COMPILER "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.44.35207/bin/HostX64/x86/cl.exe")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/third_party/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(VCPKG_BUILD_TYPE release)

set(VCPKG_TARGET_TRIPLET "x86-windows-static" CACHE STRING "VCPKG target triplet" FORCE)
set(VCPKG_HOST_TRIPLET "x86-windows-static" CACHE STRING "VCPKG host triplet" FORCE)
set(GRPC_CPP_PLUGIN_EXECUTABLE "${CMAKE_SOURCE_DIR}/third_party/vcpkg/packages/grpc_x86-windows-static/tools/grpc/grpc_cpp_plugin.exe")

project(EasyBot)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)


set(PROTO_FILE ${CMAKE_SOURCE_DIR}/bot.proto)
set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GEN_DIR})

set(GENERATED_SRCS ${GEN_DIR}/bot.pb.cc ${GEN_DIR}/bot.grpc.pb.cc)
set(GENERATED_HDRS ${GEN_DIR}/bot.pb.h ${GEN_DIR}/bot.grpc.pb.h)


add_custom_command(
        OUTPUT ${GENERATED_SRCS} ${GENERATED_HDRS}
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        --proto_path=${CMAKE_SOURCE_DIR}
        --cpp_out=${GEN_DIR}
        --grpc_out=${GEN_DIR}
        --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}
        ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        VERBATIM
)

add_library(bot_proto STATIC ${GENERATED_SRCS} ${GENERATED_HDRS})
target_include_directories(bot_proto PUBLIC ${GEN_DIR})
target_link_libraries(bot_proto PUBLIC protobuf::libprotobuf gRPC::grpc++)

add_subdirectory(src/EasyBot_DLL)
add_subdirectory(src/EasyBot_Scripts)



