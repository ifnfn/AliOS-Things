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

typedef struct {
    int running;
    int task_num;
    
    aos_queue_t queue;
    aos_mutex_t mutex;
    slist_t     service_lists;
    aos_sem_t   runing_wait;
    rpc_t       *rpc_cache;
    char        queue_buffer[QUEUE_COUNT];
} service_task_t;

struct service {
    int running;
    char            name[32];
    void            *context;
    int             max_cmd_id;
    process_t       *cmd_callbacks;

    service_task_t  *task;
    aos_mutex_t     mutex;
} ;

void service_task_init(service_task_t *task, int task_num);
void service_task_run(service_task_t *task);
int  service_task_add(service_task_t *task, service_t *srv);
int  service_task_remove(service_task_t *task, service_t *srv);
service_t *service_task_get(service_task_t *task, const char *name);

int  service_init(service_t *srv, const char *name, void *context, int max_cmd_id);
void service_destroy(service_t *srv);
void service_add_cmd(service_t *srv, int cmd, process_t process);

void service_add_event(service_t *srv, aos_event_cb cb);
void service_remove_event(service_t *srv, aos_event_cb cb);

int service_call(service_t *srv, int cmd, void *param, void *resp, size_t timeout, int sync);

#endif
