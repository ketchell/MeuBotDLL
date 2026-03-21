include_directories(
        ${CMAKE_SOURCE_DIR}/src/EasyBot_DLL/include
        ${CMAKE_SOURCE_DIR}/src/EasyBot_DLL/BotCore
        ${CMAKE_SOURCE_DIR}
)

link_directories(${CMAKE_SOURCE_DIR}/src/EasyBot_DLL/lib)

file(GLOB_RECURSE BOTCORE
        "${CMAKE_SOURCE_DIR}/src/EasyBot_DLL/*.cpp"
)

add_library(EasyBotDLL SHARED
        EasyBot_DLL.cpp
        ${BOTCORE}
        ${CMAKE_SOURCE_DIR}/OffsetResolver.cpp
        ${PROJECT_SOURCE_DIR}/proto_functions_server.cpp
)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_link_libraries(EasyBotDLL PRIVATE
            libMinHook.x64.lib
    )
else()
    target_link_libraries(EasyBotDLL PRIVATE
            libMinHook.x86.lib
    )
endif()

target_link_libraries(EasyBotDLL PRIVATE
        bot_proto
        gRPC::grpc++
        psapi
)

target_include_directories(EasyBotDLL PRIVATE ${GEN_DIR} ${CMAKE_SOURCE_DIR})

set_target_properties(EasyBotDLL PROPERTIES
        OUTPUT_NAME "Bot/EasyBot"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
