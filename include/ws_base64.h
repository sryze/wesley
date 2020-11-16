#ifndef WS_BASE64_H
#define WS_BASE64_H

#include <stddef.h>

size_t ws_base64_encode(const void *data,
                        size_t len,
                        char *buf,
                        size_t buf_size);

#endif /* WS_BASE64_H */
