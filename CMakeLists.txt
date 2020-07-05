project(wesley)

cmake_minimum_required(VERSION 3.0)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

add_library(wesley STATIC
    src/ws.c
    src/ws.h
    src/ws_base64.c
    src/ws_base64.h
    src/ws_http.c
    src/ws_http.h
    src/ws_sha1.c
    src/ws_sha1.h
    src/ws_socket.c
    src/ws_socket.h
    src/ws_string.c
    src/ws_string.h
    src/ws_types.h
)

if(WIN32)
  target_link_libraries(wesley ws2_32)
endif()

include_directories(src)

add_executable(example_server examples/server.c)
target_link_libraries(example_server wesley)

add_executable(example_client examples/client.c)
target_link_libraries(example_client wesley)
