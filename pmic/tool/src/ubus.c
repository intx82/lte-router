#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ubus.h"

static struct ubus_context *g_ubus_ctx = NULL;

int pmicctrl_handler_init(void)
{
    g_ubus_ctx = ubus_connect(NULL);
    if (!g_ubus_ctx) {
        fprintf(stderr, "Failed to connect to ubus\n");
        return -1;
    }
    ubus_add_uloop(g_ubus_ctx);
    return 0;
}

int pmicctrl_handler_register_object(struct ubus_object *object)
{
    int ret = ubus_add_object(g_ubus_ctx, object);
    if (ret) {
        fprintf(stderr, "Failed to add ubus object: %s\n", ubus_strerror(ret));
    }
    return ret;
}

int pmicctrl_send_event(const char *event_name,
                                       struct blob_buf *b)
{
    ubus_send_event(g_ubus_ctx, event_name, b->head);
    return 0;
}

void pmicctrl_handler_loop(void)
{
    uloop_run();
}

void pmicctrl_handler_cleanup(void)
{
    if (g_ubus_ctx) {
        ubus_free(g_ubus_ctx);
        g_ubus_ctx = NULL;
    }
    uloop_done();
}

struct ubus_context *pmicctrl_handler_get_context(void)
{
    return g_ubus_ctx;
}

static void __invoke_complete(struct ubus_request* req, int type, struct blob_attr* msg)
{
    (void)type;
    char** str = (char**)req->priv;

    if (msg && str) {
        *str = blobmsg_format_json(msg, true);
    }
}

int pmicctrl_call(const char* module, const char* func, struct blob_buf *b, char** str)
{
    uint32_t module_id;
    int lookup_err = ubus_lookup_id(g_ubus_ctx, module, &module_id);

    if (lookup_err) {
        return lookup_err;
    }

    return ubus_invoke(g_ubus_ctx, module_id, func, b->head, __invoke_complete, (void*)str, 2000);
}
