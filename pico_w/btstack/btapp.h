#ifndef _BT_APP_H_
#define _BT_APP_H_

#ifdef __cplusplus
extern "C" {
#endif

int btstack_main(void);
uint8_t spp_send_packet(uint8_t *data, uint16_t len);
bool isConnected(void);

#ifdef __cplusplus
}
#endif


#endif