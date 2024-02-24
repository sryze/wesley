cmake_minimum_required(VERSION 3.0)

project(wesley)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

add_library(wesley STATIC
  include/ws.h
  include/ws_string.h
  include/ws_base64.h
  include/ws_http.h
  include/ws_sha1.h
  include/ws_socket.h
  include/ws_types.h
  src/ws.c
  src/ws_base64.c
  src/ws_http.c
  src/ws_sha1.c
  src/ws_socket.c
  src/ws_string.c
)

if(WIN32)
  target_link_libraries(wesley ws2_32)
endif()

include_directories(include)

add_executable(example_server examples/server.c)
target_link_libraries(example_server wesley)

add_executable(example_client examples/client.c)
target_link_libraries(example_client wesley)
