

add_executable(EasyBotScripts
        EasyBot_Scripts.cpp
        ${CMAKE_SOURCE_DIR}/proto_functions_client.cpp
)

target_link_libraries(EasyBotScripts PRIVATE
        bot_proto
        gRPC::grpc++
)

set_target_properties(EasyBotScripts PROPERTIES
        OUTPUT_NAME "Bot/EasyBot_Scripts"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
