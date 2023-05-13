#ifndef _FIREBASE_H_
#define _FIREBASE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <string.h>
#include "lwip/pbuf.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"
#include "pico/cyw43_arch.h"


#define FIREBASE_URL                "fall-detection-f49a7-default-rtdb.firebaseio.com"
#define FIREBASE_API_KEY            "tu4kj1uQfVVrT0gQFXvX1cPpHtor9QtNd3qSj5E9"

#define WIFI_SSID                   "NhanCao"
#define WIFI_PASSWORD               "12345678"
#define TLS_CLIENT_TIMEOUT_SECS     15

typedef struct TLS_CLIENT_T_ {
    struct altcp_pcb *pcb;
    bool complete;
    const char *tls_client_http_request;
} TLS_CLIENT_T;

void sendDataToFireBase(uint8_t data);

#ifdef __cplusplus
}
#endif


#endif