#include <zephyr.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <power/reboot.h>
#include <string.h>
#include <shell/shell_uart.h>

#include "btmesh.h"
#ifdef CONFIG_SHELL
#include "cli.h"
#endif
#include "error.h"
#include "gw_cloud.h"
#include "config.h"
#include "lte.h"
#include "watchdog.h"

LOG_MODULE_REGISTER(app_main, CONFIG_NRF_CLOUD_MESH_GATEWAY_LOG_LEVEL);

K_THREAD_STACK_DEFINE(application_stack_area,
        CONFIG_APPLICATION_WORKQUEUE_STACK_SIZE);
static struct k_work_q application_work_q;


static void error_handler(enum error_type err_type, int err_code)
{
    gw_cloud_error_callback();

    if (err_type == ERROR_CLOUD) {
        lte_shutdown_modem();
    }

#if !defined(CONFIG_DEBUG) && defined(CONFIG_REBOOT)
    LOG_PANIC();
    sys_reboot(0);
#else
        
    switch (err_type) {
    case ERROR_CLOUD:
        /* Blinking all LEDs ON/OFF in pairs (1 and 4, 2 and 3)
         * if there is an application error.
         */
        // TODO ui_led_set_pattern(UI_LED_ERROR_CLOUD);
        LOG_ERR("Error of type ERROR_CLOUD: %d", err_code);
        break;
    case ERROR_MODEM_RECOVERABLE:
        /* Blinking all LEDs ON/OFF in pairs (1 and 3, 2 and 4)
         * if there is a recoverable error.
         */
        // TODO ui_led_set_pattern(UI_LED_ERROR_MODEM_REC);
        LOG_ERR("Error of type ERROR_MODEM_RECOVERABLE: %d", err_code);
        break;
    case ERROR_MODEM_IRRECOVERABLE:
        /* Blinking all LEDs ON/OFF if there is an irrecoverable error.
         */
        //TODO ui_led_set_pattern(UI_LED_ERROR_BSD_IRREC);
        LOG_ERR("Error of type ERROR_MODEM_IRRECOVERABLE: %d", err_code);
        break;
    default:
        /* Blinking all LEDs ON/OFF in pairs (1 and 2, 3 and 4)
         * undefined error.
         */
        // TODO ui_led_set_pattern(UI_LED_ERROR_UNKNOWN);
        LOG_ERR("Unknown error type: %d, code: %d", err_type, err_code);
          break;
    }

    while (true) {
        k_cpu_idle();
    }
#endif /* CONFIG_DEBUG */
}

void main(void)
{
    int err;
    printk("\n***********************************************\n");
    printk("nRF Cloud Mesh Gateway Starting Up...\n");
    printk("Ver:%s Built:%s\n", FW_REV_STRING, BUILT_STRING);
    printk("***********************************************\n\n");
    k_sleep(K_SECONDS(1));

#ifdef CONFIG_SHELL
    cli_init();
#endif
    err = btmesh_init();

    if (err) {
        LOG_ERR("Failed to initialize Bluetooth Mesh. Error: %d", err);
        return;
    }

    k_work_queue_start(&application_work_q, application_stack_area,
            K_THREAD_STACK_SIZEOF(application_stack_area),
            CONFIG_APPLICATION_WORKQUEUE_PRIORITY, NULL);
    k_thread_name_set(&application_work_q.thread, "application_work_q");

    if (IS_ENABLED(CONFIG_WATCHDOG)) {
        watchdog_init_and_start(&application_work_q);
    }

    err = gateway_init(&application_work_q);

    if (err) {
        LOG_ERR("Failed to initialize Gateway. Error: %d", err);
        return;
    }

    err = lte_init(&application_work_q, &error_handler);

    if (err) {
        LOG_ERR("Failed to initialize LTE. Error: %d", err);
        return;
    }

    err = gw_cloud_init(&application_work_q, &error_handler);

    if (err) {
        LOG_ERR("Failed to initialize Gateway Cloud. Error: %d", err);
        return;
    }
}
