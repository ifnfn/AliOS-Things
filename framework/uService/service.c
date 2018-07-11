
#include <string.h>

#include "aos/aos.h"
#include "service.h"

#define EV_SERVICE 0X200

struct _rpc_t {
    aos_sem_t sem;
    cmd_t cmd;
    int sync;
    void *param;
    void *resp;
};

static int rpc_set(rpc_t *rpc, int cmd, int timeout, int sync, void *param, void*resp)
{
    rpc->cmd.cmd = cmd;
    rpc->cmd.timeout = cmd;

    if (sync) {
        if (!aos_sem_is_valid(&rpc->sem))
            if (aos_sem_new(&rpc->sem, 0) != 0)
                return -1;
    } else if (aos_sem_is_valid(&rpc->sem)) {
            aos_sem_free(&rpc->sem);
    }

    rpc->cmd.cmd = cmd;
    rpc->cmd.timeout = cmd;

    rpc->sync = sync;
    rpc->param = param;
    rpc->resp = resp;

    return 0;
}

static rpc_t *rpc_new(int cmd, int timeout, int sync, void *param, void*resp)
{
    rpc_t *rpc = (rpc_t*)calloc(sizeof(rpc_t), 1);
    if (rpc == NULL)
        return NULL;

    if (sync) {
        if (aos_sem_new(&rpc->sem, 0) != 0) {
            free(rpc);

            return NULL;
        }
    }
    rpc->cmd.cmd = cmd;
    rpc->cmd.timeout = cmd;

    rpc->sync = sync;
    rpc->param = param;
    rpc->resp = resp;

    return rpc;
}

static void rpc_destroy(rpc_t *rpc)
{
    if (aos_sem_is_valid(&rpc->sem))
        aos_sem_free(&rpc->sem);

    free(rpc);
}

int service_call(service_t *srv, int cmd, void *param, void *resp, size_t timeout, int sync)
{
    check(srv);

    int ret = -1;
    rpc_t *rpc = rpc_new(cmd, timeout, sync, param, resp);

    // aos_mutex_lock(&srv->task->mutex);
    // rpc = srv->task->rpc_cache;
    // srv->task->rpc_cache = NULL;
    // aos_mutex_unlock(&srv->task->mutex);

    if (rpc) {
        if (aos_post_event(EV_SERVICE, cmd, (unsigned long)&rpc) == 0) {
            if (sync) {
                if (aos_sem_wait(&rpc->sem, AOS_WAIT_FOREVER) == 0)
                    ret = 0;
            } else
                ret = 0;
        }

        if (ret == -1 || sync)
            rpc_destroy(rpc);
    }

    return ret;
}

void service_add_cmd(service_t *srv, int cmd, process_t process)
{
    check(srv);

    if (cmd < srv->max_cmd_id) {
        aos_mutex_lock(&srv->mutex, AOS_WAIT_FOREVER);
        srv->cmd_callbacks[cmd] = process;
        aos_mutex_unlock(&srv->mutex);
    }
}

void service_add_event(service_t *srv, aos_event_cb cb)
{
    aos_register_event_filter(EV_SYS, cb, srv);
}

void service_remove_event(service_t *srv, aos_event_cb cb)
{
    aos_unregister_event_filter(EV_SYS, cb, srv);
}

static void service_eloop_event(input_event_t *event, service_t *srv)
{
    rpc_t *rpc = (rpc_t*)event->value;

    aos_mutex_lock(&srv->mutex, AOS_WAIT_FOREVER);
    if (event->type == EV_SERVICE && event->code >= 0 && event->code < srv->max_cmd_id) {
        process_t process = srv->cmd_callbacks[rpc->cmd.cmd];
        if (process) {
            process(srv, &rpc->cmd, rpc->param, rpc->resp);
            if (rpc->sync)
                aos_sem_wait(&rpc->sem, AOS_WAIT_FOREVER);
            else
                rpc_destroy(rpc);
        }
    }
    aos_mutex_unlock(&srv->mutex);
}


int service_init(service_t *srv, const char *name, const char *group_name, void *context, int max_cmd_id)
{
    check(srv);

    srv->cmd_callbacks = (process_t*)calloc(max_cmd_id, sizeof(process_t));
    if (srv->cmd_callbacks == NULL)
        return -1; 

    if (aos_mutex_new(&srv->mutex) != 0) {
        free(srv->cmd_callbacks);
        return -1; 
    }

    strncpy(srv->name, name, sizeof(srv->name) - 1);
    srv->context    = context;
    srv->max_cmd_id = max_cmd_id;

    aos_register_event_filter(EV_SERVICE, (aos_event_cb)service_eloop_event, srv);

    return 0;
}

void service_destroy(service_t *srv)
{
    check(srv);

    aos_unregister_event_filter(EV_SERVICE, (aos_event_cb)service_eloop_event, srv);
    aos_mutex_free(&srv->mutex);
    free(srv->cmd_callbacks);
}

#if 0
static void rpc_process(service_t *srv, rpc_t *rpc)
{
    aos_mutex_lock(&srv->mutex, AOS_WAIT_FOREVER);
    if (rpc->cmd.cmd < srv->max_cmd_id) {
        process_t process = srv->cmd_callbacks[rpc->cmd.cmd];
        if (process) {
            process(srv, &rpc->cmd, rpc->param, rpc->resp);
            aos_sem_wait(&rpc->sem, AOS_WAIT_FOREVER);
        }
    }
    aos_mutex_unlock(&srv->mutex);
}

int service_task_init(service_task_t *task, int task_num)
{
    memset(task, 0, sizeof(service_task_t));
    task->task_num = task_num;

    aos_queue_t queue;
    aos_mutex_t mutex;
    slist_t     callback_list;
    aos_sem_t   runing_wait;

    if (aos_queue_new(&task->queue, task->queue_buffer, QUEUE_COUNT, QUEUE_MSG_SIZE) == 0)
        return 0;

    if (aos_mutex_new(&task->mutex) != 0)
        goto out1;


out1:
    aos_queue_free(&task->queue);
    return -1;
}

static void service_task_entry(void *data)
{
    service_task_t *task = (service_task_t*)data;
    uint32_t ms = 500;
    rpc_t rpc;
    size_t size;

    while (task->running) {
        if (aos_queue_recv(&task->queue, ms, &rpc, &size) == 0) {
            // rpc_process(rpc.srv, rpc);
        }
    }
}

void service_task_run(service_task_t *task)
{
    int i;

    for (i=0; i < task->task_num; i++) {
        char name[8];
        sprintf(name, "srv_%d", i);
        aos_task_new(name, service_task_entry, task, 1024);
    }
}

int service_task_add(service_task_t *task, service_t *srv)
{
    check(task);
    check(srv);

    aos_mutex_lock(&srv->mutex, AOS_WAIT_FOREVER);
    srv->task = task;
    aos_mutex_unlock(&srv->mutex);
}

int  service_task_remove(service_task_t *task, service_t *srv)
{
    check(task);
    check(srv);

    aos_mutex_lock(&srv->mutex, AOS_WAIT_FOREVER);
    srv->task = NULL;
    aos_mutex_unlock(&srv->mutex);
}

service_t *service_task_get(service_task_t *task, const char *name)
{
    

    slist_for_each_entry(task->service_lists, node, type, member) {

    }
    ...;
}

#endif