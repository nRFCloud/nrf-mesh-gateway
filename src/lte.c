#include <zephyr.h>
#include <date_time.h>
#if defined(CONFIG_NRF_MODEM_LIB)
#include <modem/nrf_modem_lib.h>
#include <nrf_modem.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#endif /* CONFIG_NRF_MODEM_LIB */
#include <logging/log.h>
#include <power/reboot.h>
#include <stdio.h>

#include "error.h"


LOG_MODULE_REGISTER(app_lte, CONFIG_NRF_CLOUD_MESH_GATEWAY_LOG_LEVEL);

static void (*error_handler)(enum error_type type, int err);

static struct k_work_q *work_q;

static struct k_work no_sim_go_offline_work;

static void no_sim_go_offline(struct k_work *work)
{
#if defined(CONFIG_NRF_MODEM_LIB)
    lte_lc_offline();

    /* Wait for lte_lc events to be processes before printing info message */
    k_sleep(K_MSEC(100));
    LOG_INF("No SIM card detected.");
    LOG_INF("Insert SIM and reset device to run the mesh gateway.");
    // TODO: ui_led_set_pattern(UI_LED_ERROR_LTE_LC);
#endif /* CONFIG_NRF_MODEM_LIB */
}

#if defined(CONFIG_LTE_LINK_CONTROL)
static void lte_handler(const struct lte_lc_evt *const evt)
{
  switch (evt->type) {
      case LTE_LC_EVT_NW_REG_STATUS:
          if (evt->nw_reg_status == LTE_LC_NW_REG_UICC_FAIL) {
              k_work_submit_to_queue(work_q, &no_sim_go_offline_work);
          } else if ((evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
              LOG_INF("Network registration status: %s",
                      evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
                      "Connected - home network" : "Connected - roaming");
          }
          break;

      case LTE_LC_EVT_PSM_UPDATE:
          LOG_INF("PSM paramater update: TAU:%d, Active time: %d",
                  evt->psm_cfg.tau, evt->psm_cfg.active_time);
          break;

      case LTE_LC_EVT_EDRX_UPDATE: {
          char log_buf[60];
          size_t len;

          len = snprintf(log_buf, sizeof(log_buf),
                  "eDRX parameter update: eDRX: %0.2f, PTW: %0.2f",
                  evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);

          if ((len >0) && (len < sizeof(log_buf))) {
              LOG_INF("%s", log_strdup(log_buf));
          }

          break;
      }

      case LTE_LC_EVT_RRC_UPDATE:
          LOG_INF("RRC mode: %s",
                  evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
                  "Connected" : "Idle");
          break;

      case LTE_LC_EVT_CELL_UPDATE:
          LOG_INF("LTE cell changed: Cell ID: %d, Tracking area: %d",
                  evt->cell.id, evt->cell.tac);
          break;

      default:
          break;
  }
}
#endif /* defined(CONFIG_LTE_LINK_CONTROL) */


#if defined(CONFIG_NRF_MODEM_LIB)
static void handle_nrf_modem_lib_init_ret(void)
{
  int ret = nrf_modem_lib_get_init_ret();

  switch (ret) {
      case 0:
          /* Initialization successful, no action required. */
          break;

      case MODEM_DFU_RESULT_OK:
          LOG_INF("MODEM UPDATE OK. Will run new firmware");
          sys_reboot(SYS_REBOOT_COLD);
          break;

      case MODEM_DFU_RESULT_UUID_ERROR:
      case MODEM_DFU_RESULT_AUTH_ERROR:
          LOG_ERR("MODEM UPDATE ERROR %d. Will run old firmware", ret);
          sys_reboot(SYS_REBOOT_COLD);
          break;

      case MODEM_DFU_RESULT_HARDWARE_ERROR:
      case MODEM_DFU_RESULT_INTERNAL_ERROR:
          LOG_ERR("MODEM UPDATE FATAL ERROR %d. Modem failiure", ret);
          sys_reboot(SYS_REBOOT_COLD);
          break;

      default:
          /* All non-zero return codes other than DFU result codes are
           * considered irrecoverable and a reboot is needed.
           */
          LOG_ERR("BSDlib initialization failed, error: %d", ret);
          error_handler(ERROR_MODEM_IRRECOVERABLE, ret);

          CODE_UNREACHABLE;
  }
}


void nrf_modem_recoverable_error_handler(uint32_t err)
{
    error_handler(ERROR_MODEM_RECOVERABLE, (int)err);
}


static int modem_configure(void)
{
  if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
      /* Do nothing, modem is already turned on and connected */
      goto connected;
  }

  // TODO ui_led_set_pattern(UI_LTE_CONNECTING);
  LOG_INF("Connecting to LTE network.");
  LOG_INF("This may take several minutes.");

  int err = lte_lc_init_and_connect();

  if (err) {
      LOG_ERR("LTE link could not be established.");
      return err;
  }

connected:
  LOG_INF("Connected to LTE network.");
  // TODO ui_led_set_pattern(UI_LTE_CONNECTED);

  return 0;
}
#endif /* defined(CONFIG_NRF_MODEM_LIB */


static void date_time_event_handler(const struct date_time_evt *evt)
{
  switch (evt->type) {
      case DATE_TIME_OBTAINED_MODEM:
          LOG_INF("DATE_TIME_OBTAINED_MODEM");
          break;

      case DATE_TIME_OBTAINED_NTP:
          LOG_INF("DATE_TIME_OBTAINED_NTP");
          break;

      case DATE_TIME_OBTAINED_EXT:
          LOG_INF("DATE_TIME_OBTAINED_EXT");
          break;

      case DATE_TIME_NOT_OBTAINED:
          LOG_INF("DATE_TIME_NOT_OBTAINED");
          break;

      default:
          break;
  }
}


void lte_shutdown_modem(void)
{
#if defined(CONFIG_LTE_LINK_CONTROL)
  /* Turn off and shutdown modem */
  LOG_ERR("LTE link disconnect");
  int err = lte_lc_power_off();

  if (err) {
      LOG_ERR("lte_lc_power_off failed: %d", err);
  }
#endif /* CONFIG_LTE_LINK_CONTROL */

#if defined(CONFIG_NRF_MODEM_LIB)
  LOG_ERR("Shutdown modem");
  nrf_modem_lib_shutdown();
#endif
}


int lte_init(struct k_work_q* _work_q,
        void(*_error_handler)(enum error_type type, int err))
{
    if (_work_q == NULL) {
        LOG_ERR("NULL Workqueue");
        return -EINVAL;
    }
   
    work_q = _work_q;

    if (_error_handler == NULL) {
        LOG_ERR("NULL Error Handler");
        return -EINVAL;
    }

    error_handler = _error_handler;

#if defined(CONFIG_NRF_MODEM_LIB)
    handle_nrf_modem_lib_init_ret();
#endif /* CONFIG_NRF_MODEM_LIB */

    k_work_init(&no_sim_go_offline_work, no_sim_go_offline);

#if defined(CONFIG_LTE_LINK_CONTROL)
    lte_lc_register_handler(lte_handler);
#endif /* CONFIG_LTE_LINK_CONTROL */

#if defined(CONFIG_NRF_MODEM_LIB)
    while(modem_configure() != 0) {
        LOG_WRN("Failed to establish LTE connection.");
        LOG_WRN("Will retry in %d seconds.", CONFIG_LTE_CONNECT_RETRY_DELAY);
        k_sleep(K_SECONDS(CONFIG_LTE_CONNECT_RETRY_DELAY));
    }
#endif /* CONFIG_NRF_MODEM_LIB */

    date_time_update_async(date_time_event_handler);

    return 0;
}
