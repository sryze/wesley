#ifndef WS_TYPES_H
#define WS_TYPES_H

#ifdef _MSC_VER
  typedef __int8 ws_int8_t;
  typedef unsigned __int8 ws_uint8_t;
  typedef __int16 ws_int16_t;
  typedef unsigned __int16 ws_uint16_t;
  typedef __int32 ws_int32_t;
  typedef unsigned __int32 ws_uint32_t;
  typedef __int64 ws_int64_t;
  typedef unsigned __int64 ws_uint64_t;
#else
  #include <stdint.h>
  typedef int8_t ws_int8_t;
  typedef uint8_t ws_uint8_t;
  typedef int16_t ws_int16_t;
  typedef uint16_t ws_uint16_t;
  typedef int32_t ws_int32_t;
  typedef uint32_t ws_uint32_t;
  typedef int64_t ws_int64_t;
  typedef uint64_t ws_uint64_t;
#endif

#endif /* WS_TYPES_H */
