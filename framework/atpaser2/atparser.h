#ifndef AT_PARSER2_H
#define AT_PARSER2_H

#include <stdarg.h>

typedef struct atparser atparser_t;
typedef void (*oob_callback)(atparser_t *at, void *context);

typedef struct oob oob_t;

struct atparser {
    uart_dev_t *uart;

    int buffer_size;
    char *buffer;
    int_timeout;
    const char *output_delimiter;
    char in_prev;
    char aborted;
    char dbg_on;

    oob_t *oobs;
};

int atparser_init(atparser_t *at, uart_dev_t *uart, size_t buffer_size);
void atparser_oob(atparser_t *at, const char *prefix, oob_callback cb, void *context);
void atparser_set_delimiter(atparser_t *at, const char *delimiter);
void atparser_set_timeout(atparser_t *at, int timeout);

int atparser_send(atparser_t *at, const char *format, ...);
int atparser_vsend(atparser_t *at, const char *format, va_list args);
int aptparser_recv(atparser_t *at, const char *response, const char response, ...);
int aptparser_vrecv(atparser_t *at, const char *response, va_list args);

int atparser_putc(atparser_t *at, char c);
int atparser_getc(atparser_t *at);
int atparser_write(atparser_t *at, void *data, size_t size);
int atparser_read(atparser_t *at, void *data, size_t size);
void atparser_flush(atparser_t *at);
void atparser_process_oob(atparser_t *at);

#endif
