#include <assert.h>
#include <libubox/blobmsg.h>
#include <libubox/blobmsg_json.h>
#include <libubox/uloop.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "daemon.h"
#include "ubus.h"

/* Global pointer to the I2C device (used in callbacks) */
static struct I2cDevice *g_dev = NULL;

int set_led_color(struct I2cDevice *dev, uint8_t r, uint8_t g, uint8_t b);
int shutdown_device(struct I2cDevice *dev);

typedef union
{
    uint8_t raw;
    struct
    {
        uint8_t charge : 1;
        uint8_t stdby : 1;
        uint8_t lte : 1;
        uint8_t pwr : 1;
        uint8_t bat_low : 1;
        uint8_t : 3;
    };
} pmic_state_t;

/* --- LED Command Policy & Callback --- */
enum
{
    LED_R,
    LED_G,
    LED_B,
    __LED_MAX,
};

static const struct blobmsg_policy led_policy[__LED_MAX] = {
    [LED_R] = {.name = "r", .type = BLOBMSG_TYPE_INT32},
    [LED_G] = {.name = "g", .type = BLOBMSG_TYPE_INT32},
    [LED_B] = {.name = "b", .type = BLOBMSG_TYPE_INT32},
};

static int ubus_set_led(struct ubus_context *ctx, struct ubus_object *obj,
                        struct ubus_request_data *req, const char *method,
                        struct blob_attr *msg)
{
    (void)ctx;
    (void)obj;
    (void)req;
    (void)method;

    struct blob_attr *tb[__LED_MAX];
    blobmsg_parse(led_policy, __LED_MAX, tb, blobmsg_data(msg), blobmsg_len(msg));
    if (!tb[LED_R] || !tb[LED_G] || !tb[LED_B]) {
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    int r = blobmsg_get_u32(tb[LED_R]);
    int g = blobmsg_get_u32(tb[LED_G]);
    int b_val = blobmsg_get_u32(tb[LED_B]);

    printf("Received ubus set_led command: r=%d, g=%d, b=%d\n", r, g, b_val);
    return set_led_color(g_dev, (uint8_t)r, (uint8_t)g, (uint8_t)b_val);
}

static int ubus_dev_shutdown(struct ubus_context *ctx, struct ubus_object *obj,
    struct ubus_request_data *req, const char *method,
    struct blob_attr *msg)
{
    (void)ctx;
    (void)obj;
    (void)req;
    (void)method;
    (void)msg;
    shutdown_device(g_dev);
    return UBUS_STATUS_OK;;
}


static const struct ubus_method pmic_methods[] = {
    UBUS_METHOD_NOARG("shutdown", ubus_dev_shutdown),
    UBUS_METHOD("set_led",  ubus_set_led, led_policy),
};

static struct ubus_object_type pmic_object_type =
    UBUS_OBJECT_TYPE("pmic", pmic_methods);

static struct ubus_object pmic_object = {
    .name = "pmic",
    .type = &pmic_object_type,
    .methods = pmic_methods,
    .n_methods = ARRAY_SIZE(pmic_methods),
};

/* --- Polling Callback Example --- */
static pmic_state_t current_state;
static unsigned int pressed_count = 0;

static void blobmsg_add_float(struct blob_buf *buffer, const char *name, float value)
{
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "%.4f", value);
    blobmsg_add_string(buffer, name, tmp);
}

static void power_btn_hnd(pmic_state_t *state)
{
    static struct blob_buf b;
    if (state->pwr != current_state.pwr) {
        blob_buf_init(&b, 0);
        blobmsg_add_u32(&b, "power", !state->pwr);
        if (pmicctrl_send_event("pmic", &b) != 0) {
            fprintf(stderr, "pmicctrl_send_event failed\n");
        }
    }

    if (!state->pwr) {
        pressed_count++;
    } else {
        pressed_count = 0;
    }

    if (pressed_count >= 10) { /* 10 polls * 100ms = 1 second */
        blob_buf_init(&b, 0);
        blobmsg_add_string(&b, "action", "poweroff");
        if (pmicctrl_send_event("pmic", &b) != 0) {
            fprintf(stderr, "pmicctrl_send_event failed\n");
        }
        pressed_count = 0;
    }
}

static void low_bat_hnd(void)
{
    if (current_state.bat_low) {
        static struct blob_buf b;
        blob_buf_init(&b, 0);
        blobmsg_add_u32(&b, "battery-low", current_state.bat_low);
        blobmsg_add_string(&b, "action", "poweroff");
        if (pmicctrl_send_event("pmic", &b) != 0) {
            fprintf(stderr, "pmicctrl_send_event failed\n");
        }
    }
}

#if 0
static void lte_hnd(pmic_state_t *state)
{
    if (state->lte != current_state.lte) {
        static struct blob_buf b;
        blob_buf_init(&b, 0);
        blobmsg_add_u32(&b, "lte", state->lte);
        if (pmicctrl_send_event("pmic", &b) != 0) {
            fprintf(stderr, "pmicctrl_send_event failed\n");
        }
    }
}
#endif

static void charge_hnd(pmic_state_t *state)
{
    if (state->charge != current_state.charge) {
        static struct blob_buf b;
        blob_buf_init(&b, 0);
        blobmsg_add_u32(&b, "charge", !state->charge);
        if (pmicctrl_send_event("pmic", &b) != 0) {
            fprintf(stderr, "pmicctrl_send_event failed\n");
        }
    }
}

static void standby_hnd(pmic_state_t *state)
{
    if (state->stdby != current_state.stdby) {
        static struct blob_buf b;
        blob_buf_init(&b, 0);
        blobmsg_add_u32(&b, "standby", state->stdby);
        if (pmicctrl_send_event("pmic", &b) != 0) {
            fprintf(stderr, "pmicctrl_send_event failed\n");
        }
    }
}

static void vbat_poll_cb(struct uloop_timeout *t)
{
    static struct blob_buf b;
    uint8_t regs[2];
    int rc = i2c_readn_reg(g_dev, 12, regs, 2);
    if (rc <= 0) {
        fprintf(stderr, "Failed to read PMIC registers\n");
        return;
    }

    uint16_t adc_val = (regs[1] << 8) | regs[0];
    float vbat = DIV_RATIO * (VREF * ((float)adc_val / ADC_MAX));

    blob_buf_init(&b, 0);
    blobmsg_add_float(&b, "battery", vbat);
    if (pmicctrl_send_event("pmic", &b) != 0) {
        fprintf(stderr, "pmicctrl_send_event failed\n");
    }

    low_bat_hnd();
    uloop_timeout_set(t, VBAT_POLL_INTERVAL);
}

static void status_poll_cb(struct uloop_timeout *t)
{
    (void)t;
    pmic_state_t state;
    assert(g_dev);

    state.raw = i2c_read_reg(g_dev, 14);

    power_btn_hnd(&state);
    //lte_hnd(&state);
    charge_hnd(&state);
    standby_hnd(&state);

    current_state.raw = state.raw;

    /* Reschedule the poll callback */
    uloop_timeout_set(t, STATUS_POLL_INTERVAL);
}

/* --- Main Daemon Function --- */
int run_daemon(struct I2cDevice *dev)
{
    int ret;
    static struct uloop_timeout status_poll_timer = {
        .cb = status_poll_cb,
    };

    static struct uloop_timeout vbat_poll_timer = {
        .cb = vbat_poll_cb,
    };

    current_state.raw = 0;
    g_dev = dev;

    /* Initialize the UBUS handler */
    if ((ret = pmicctrl_handler_init()) != 0) {
        return ret;
    }

    /* Register the pmic object (which supports set_led) */
    ret = pmicctrl_handler_register_object(&pmic_object);
    if (ret) {
        return ret;
    }

    /* Initialize uloop and set up a polling timer */
    uloop_init();

    uloop_timeout_set(&status_poll_timer, STATUS_POLL_INTERVAL);
    uloop_timeout_set(&vbat_poll_timer, VBAT_POLL_INTERVAL);

    printf("Daemon started. Polling PMIC power button every 100ms and listening for ubus messages...\n");
    pmicctrl_handler_loop();

    pmicctrl_handler_cleanup();
    return 0;
}

