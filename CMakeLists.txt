project(ws)

cmake_minimum_required(VERSION 3.0)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

add_library(ws STATIC
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
  target_link_libraries(ws ws2_32)
endif()

include_directories(src)

add_executable(ws_server examples/ws_server.c)
target_link_libraries(ws_server ws)

add_executable(ws_client examples/ws_client.c)
target_link_libraries(ws_client ws)
