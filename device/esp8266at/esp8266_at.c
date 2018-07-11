#include "esp8266_api.h"
#include "atpasrser.h"

struct packet {
    struct packet *next;
    int id;
    uint32_t len;
    // data follows
};

static struct esp8266_at {
    uart_dev_t uart;
    atparser_t at;
    ring_buffer_t buffer;
    struct packet *_packets, **_packets_end;
} esp8266;

static void _packet_handler(atparser_t *at, void *context)
{
    int id;
    int amount;

    // parse out the packet
    if (!_parser.recv(",%d,%d:", &id, &amount)) {
        return;
    }

    struct packet *packet = (struct packet*)malloc(sizeof(struct packet) + amount);
    if (!packet) {
        debug("ESP8266: could not allocate memory for RX data\n");
        return;
    }

    packet->id = id;
    packet->len = amount;
    packet->next = NULL;

    if (atparser_read(at, (char*)(packet + 1), amount) < amount) {
        free(packet);
        return;
    }

    // append to packet list
    *_packets_end = packet;
    _packets_end = &packet->next;
}

static void _connect_error_handler(atparser_t *at, void *context)
{

}
static void _oob_socket0_closed_handler(atparser_t *at, void *context)
{
    
}
static void _oob_socket1_closed_handler(atparser_t *at, void *context)
{
    
}
static void _oob_socket2_closed_handler(atparser_t *at, void *context)
{
    
}
static void _oob_socket3_closed_handler(atparser_t *at, void *context)
{
    
}
static void _oob_socket4_closed_handler(atparser_t *at, void *context)
{
    
}
static void _connection_status_handler(atparser_t *at, void *context)
{
    
}
static void _oob_socket_close_error(atparser_t *at, void *context)
{
    
}


int esp8266_init(int port)
{
    memset(&esp8266, 0, sizeof(esp8266));
    esp8266.uart.port = port;

    esp8266.uart.config.baud_rate    = CONFIG_NETM_BAUD;
    esp8266.uart.config.data_width   = USART_DATA_BITS_8;
    esp8266.uart.config.parity       = USART_PARITY_NONE;
    esp8266.uart.config.stop_bits    = USART_STOP_BITS_1;
    esp8266.uart.config.flow_control = 0;
    esp8266.uart.config.mode         = USART_MODE_ASYNCHRONOUS;
    if (aos_uart_init(&esp8266.uart) != 0) {
        return -1;
    }

    esp8266.rb = ringbuffer_create(CONFIG_NETM_RDBUFSIZE);

    atparser_init(&esp8266.at, &esp8266.uart, 256);
    atparser_set_delimiter(&esp8266.at, "\r\n");
      _packets(0), 
      _packets_end(&_packets),

    atparser_oob((&esp8266.at, "+IPD", _packet_handler, &esp8266);
    //Note: espressif at command document says that this should be +CWJAP_CUR:<error code>
    //but seems that at least current version is not sending it
    //https://www.espressif.com/sites/default/files/documentation/4a-esp8266_at_instruction_set_en.pdf
    //Also seems that ERROR is not sent, but FAIL instead
    atparser_oob((&esp8266.at, "+CWJAP:", _connect_error_handler, &esp8266);
    atparser_oob((&esp8266.at, "0,CLOSED", _oob_socket0_closed_handler, &esp8266);
    atparser_oob((&esp8266.at, "1,CLOSED", _oob_socket1_closed_handler, &esp8266);
    atparser_oob((&esp8266.at, "2,CLOSED", _oob_socket2_closed_handler, &esp8266);
    atparser_oob((&esp8266.at, "3,CLOSED", _oob_socket3_closed_handler, &esp8266);
    atparser_oob((&esp8266.at, "4,CLOSED", _oob_socket4_closed_handler, &esp8266);
    atparser_oob((&esp8266.at, "WIFI ", _connection_status_handler, &esp8266);
    atparser_oob((&esp8266.at, "UNLINK", _oob_socket_close_error, &esp8266);

    return 0;
}

int esp8266_info(char net_ipaddr[16], char net_mask[16], char net_gw[16])
{
    int ret = -1;
    atparser_set_timeout(&esp8266.at, ESP8266_CONNECT_TIMEOUT);

    if (atparser_send(&esp8266.at, "AT+CIPSTA?") == 0) {
        if (atparser_recv(&esp8266.at, "+CIPSTA:ip:%15", net_ipaddr) == 0 \\
                && atparser_recv(&esp8266.at, "+CIPSTA:gateway:%15", net_gw) == 0 \\
                && atparser_recv(&esp8266.at, "+CIPSTA:netmask:%15", net_mask) == 0)
                && atparser_recv(&esp8266.at, "OK\n"))
            {
                ret = 0;
            }
    }

    return ret;
}

int esp8266_connect(const char *ap, const char *passPhrase)
{
    int ret = -1;
    
    if (atparser_send(&esp8266.at, "AT+CWJAP_CUR=\"%s\",\"%s\"", ap, passPhrase) == 0)
        ret = atparser_recv(&esp8266.at, "OK\n")

    return ret;
}

int esp8266_disconnect()
{
    int ret = -1;

    if (atparser_send(&esp8266.at, "AT+CWQAP") == 0)
        ret = atparser_recv(&esp8266.at, "OK\n");
    
    return ret;
}

int esp8266_dns_lookup(const char* name, char* ip)
{
    int ret = -1;

    if (atparser_send(&esp8266.at, "AT+CIPDOMAIN=\"%s\"", name) == 0)
        ret = atparser_recv(&esp8266.at, "+CIPDOMAIN:%s%*[\r]%*[\n]", ip);

    return ret;
}

int esp8266_send(int id, const void *data, uint32_t amount)
{
    //May take a second try if device is busy
    int ret = -1;

    int i;
    for (i = 0; i < 2; i++) {
        atparser_set_timeout(&esp8266.at, ESP8266_SEND_TIMEOUT);

        if (atparser_send(&esp8266.at, "AT+CIPSEND=%d,%lu", id, amount) == 0
            && atparser_recv(&esp8266.at, ">") == 0
            && atparser_write(&esp8266.at, (char*)data, (int)amount) >= 0) {
            while (atparser_process_oob(at) != 0); // multiple sends in a row require this
            ret = 0;
            break;
        }
        atparser_set_timeout(&esp8266.at, ESP8266_MISC_TIMEOUT);
    }

    return ret;
}

int32_t esp8266_recv_tcp(int id, void *data, uint32_t amount, uint32_t timeout)
{
    _smutex.lock();
    setTimeout(timeout);
    atparser_set_timeout(at, timeout);

    // Poll for inbound packets
    while (atparser_process_oob(at) != 0);

    atparser_set_timeout(at, ESP8266_MISC_TIMEOUT);

    // check if any packets are ready for us
    for (struct packet **p = &_packets; *p; p = &(*p)->next) {
        if ((*p)->id == id) {
            struct packet *q = *p;

            if (q->len <= amount) { // Return and remove full packet
                memcpy(data, q+1, q->len);

                if (_packets_end == &(*p)->next) {
                    _packets_end = p;
                }
                *p = (*p)->next;
                _smutex.unlock();

                uint32_t len = q->len;
                free(q);
                return len;
            } else { // return only partial packet
                memcpy(data, q+1, amount);

                q->len -= amount;
                memmove(q+1, (uint8_t*)(q+1) + amount, q->len);

                _smutex.unlock();
                return amount;
            }
        }
    }
    if(!_socket_open[id]) {
        _smutex.unlock();
        return 0;
    }
    _smutex.unlock();

    return NSAPI_ERROR_WOULD_BLOCK;
}