#ifndef UBUS_HANDLER_H
#define UBUS_HANDLER_H

#include <libubus.h>
#include <libubox/blobmsg.h>
#include <libubox/uloop.h>
#include <libubox/blobmsg_json.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*pmicctrl_call_cb_t)(struct ubus_request *, int, struct blob_attr*);

/**
 * Initialize the UBUS handler: connect to ubus and integrate with uloop.
 * Returns 0 on success, or a negative value on error.
 */
int pmicctrl_handler_init(void);

/**
 * Register a UBUS object (for receiving events/method calls).
 * Returns 0 on success, or a negative value on error.
 */
int pmicctrl_handler_register_object(struct ubus_object *object);

/**
 * Send a deferred event (simulate an event reply when no incoming request exists).
 * Instead of using ubus_send_event (which has a regression on some devices),
 * this function creates a dummy deferred request and schedules a uloop timeout
 * to send the reply.
 *
 * @event_name: The name of the event (used for debugging).
 * @b: A blob buffer containing the event payload.
 *
 * Returns 0 on success, or a negative value on error.
 */
int pmicctrl_send_event(const char *event_name, struct blob_buf *b);

/**
 * Run the UBUS event loop.
 */
void pmicctrl_handler_loop(void);

/**
 * Clean up and free UBUS resources.
 */
void pmicctrl_handler_cleanup(void);

/**
 * Retrieve the global ubus context.
 */
struct ubus_context *pmicctrl_handler_get_context(void);

/**
 * @brief UBUS-Call
 * 
 * @param module Module name (pmic)
 * @param func Function name (shutdown)
 * @param b Arguements
 * @param str Output JSON string
 * @return 
 */
int pmicctrl_call(const char* module, const char* func, struct blob_buf *b, char** str);

#ifdef __cplusplus
}
#endif

#endif