#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Start a receive-only IPv6 UDP test server on port 4242. */
esp_err_t uart_ipv6_udp_test_start(void);

#ifdef __cplusplus
}
#endif
