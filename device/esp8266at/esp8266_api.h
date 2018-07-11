#ifndef ESP8266_API_H
#define ESP8266_API_H

#include <stdint.h>

int esp8266_softreset(void);
int esp8266_close_echo(uint8_t isAsync);
int esp8266_set_mode(uint8_t mode);
int esp8266_set_mux_mode(uint8_t mode, uint8_t isAsync);
int esp8266_connect(int id, netm_conn_e type, char *srvname, uint16_t port);
int esp8266_mux_send(int id, const uint8_t *pdata, int len);
int esp8266_disconnect(int id);
int esp8266_parse_domain(const char *servername, char ip[16]);
int esp8266_get_local_ipaddr(char ip[16]);
int esp8266_get_link_status(void);
int esp8266_set_default_ap(const char *ssid, const char *psw);

#endif
