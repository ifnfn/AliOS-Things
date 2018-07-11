#ifndef SERVICE_H
#define SERVICE_H

#include <stdlib.h>

#include "aos/aos.h"

typedef struct {
    int cmd;
    int timeout;
} cmd_t;

typedef struct service service_t;
typedef void (*process_t)(service_t *, cmd_t*, void*, void*);

typedef struct _rpc_t rpc_t;


#define QUEUE_MSG_SIZE   (sizeof(rpc_t*))
#define QUEUE_MSG_COUNT  8
#define QUEUE_COUNT      (QUEUE_MSG_SIZE*QUEUE_MSG_COUNT)

struct service {
    char            name[32];
    void            *context;
    int             max_cmd_id;
    process_t       *cmd_callbacks;

    aos_mutex_t     mutex;
} ;

int  service_init(service_t *srv, const char *name, const char *group_name, void *context, int max_cmd_id);
void service_destroy(service_t *srv);
void service_add_cmd(service_t *srv, int cmd, process_t process);
int service_call(service_t *srv, int cmd, void *param, void *resp, size_t timeout, int sync);

void service_register_event(service_t *srv, uint16_t event_id, aos_event_cb cb, void *priv);
void service_unregister_event(service_t *srv, uint16_t event_id, aos_event_cb cb, void *priv);


#endif
