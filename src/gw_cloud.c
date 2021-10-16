#include <zephyr.h>
#include <kernel_structs.h>
#include <stdio.h>
#include <string.h>
#include <sys/reboot.h>
#include <net/cloud.h>
#include <net/socket.h>
#include <net/nrf_cloud.h>
#include <logging/log.h>
#if defined(CONFIG_NRF_MODEM_LIB)
#include <nrf_socket.h>
#endif

#include "error.h"
#include "lte.h"
#include "nrf_cloud_transport.h"
#include "gw_cloud.h"
#include "gateway.h"

LOG_MODULE_REGISTER(app_gw_cloud, CONFIG_NRF_CLOUD_MESH_GATEWAY_LOG_LEVEL);

#define CLOUD_CONNACK_WAIT_DURATION (CONFIG_CLOUD_WAIT_DURATION * MSEC_PER_SEC)

/* Interval in milliseconds after which the device will reboot
 * if the disconnect event has not been handled.
 */
#define REBOOT_AFTER_DISCONNECT_WAIT_MS (15 * MSEC_PER_SEC)

/* Interval in milliseconds after which the device will
 * disconenct and reconnect if association was not completed.
 */
#define CONN_CYCLE_AFTER_ASSOCIATION_REQ_MS K_MINUTES(5)

#define AT_CMNG_READ_LEN 97
#define GET_PSK_ID "AT%%CMNG=2,%d,4"
#define GET_PSK_ID_ERR "ERROR"

char gateway_id[NRF_CLOUD_CLIENT_ID_LEN+1];

static char device_id[64];

static void(*error_handler)(enum error_type type, int err);

static struct k_work_q *work_q;

K_SEM_DEFINE(cloud_disconnected, 0, 1);

static atomic_t carrier_requested_disconnect;
static atomic_t cloud_connect_attempts;

static struct cloud_backend *cloud_backend;

static struct k_work_delayable cloud_reboot_work;
static struct k_work_delayable cycle_cloud_connection_work;
static struct k_work_delayable cloud_connect_work;

enum cloud_association_state {
    CLOUD_ASSOCIATION_STATE_INIT,
    CLOUD_ASSOCIATION_STATE_REQUESTED,
    CLOUD_ASSOCIATION_STATE_PAIRED,
    CLOUD_ASSOCIATION_STATE_RECONNECT,
    CLOUD_ASSOCIATION_STATE_READY
};

static atomic_val_t cloud_association =
    ATOMIC_INIT(CLOUD_ASSOCIATION_STATE_INIT);


static void cloud_connect_error_handler(enum cloud_connect_result err)
{
    bool reboot = true;
    char *backend_name = "invalid";

    if (err == CLOUD_CONNECT_RES_SUCCESS) {
        return;
    }

    LOG_ERR("Failed to connect to cloud, error %d", err);

    switch (err) {
        case CLOUD_CONNECT_RES_ERR_NOT_INITD:
            LOG_ERR("CLOUD backend has not been initialized");
            /* no need to reboot, program error */
            reboot = false;
            break;

        case CLOUD_CONNECT_RES_ERR_NETWORK:
            LOG_ERR("Network error, check cloud configuration");
            break;

        case CLOUD_CONNECT_RES_ERR_BACKEND:
            if (cloud_backend && cloud_backend->config &&
                    cloud_backend->config->name) {
                backend_name = cloud_backend->config->name;
            }

            LOG_ERR("An error occured specific to the cloud backend: %s",
                    backend_name);
            break;

        case CLOUD_CONNECT_RES_ERR_PRV_KEY:
            LOG_ERR("Ensure device has a valid private key");
            break;

        case CLOUD_CONNECT_RES_ERR_CERT:
            LOG_ERR("Ensure device has a valid CA and client certificate");
            break;

        case CLOUD_CONNECT_RES_ERR_CERT_MISC:
            LOG_ERR("A certificate/authorization error has occured");
            break;

        case CLOUD_CONNECT_RES_ERR_TIMEOUT_NO_DATA:
            LOG_ERR("Connect timeout. SIM card may be out of data");
            break;

        case CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED:
            LOG_ERR("Connection already exists.");
            break;

        case CLOUD_CONNECT_RES_ERR_MISC:
            break;
        
        default:
            LOG_ERR("Unhandled connect error");
            break;
    }

    if (reboot) {
        LOG_ERR("Device will reboot in %d seconds",
                CONFIG_CLOUD_CONNECT_ERR_REBOOT_S);
        k_work_reschedule_for_queue(work_q, &cloud_reboot_work,
                K_SECONDS(CONFIG_CLOUD_CONNECT_ERR_REBOOT_S));
    }

    // TODO ui_led_set_pattern(UI_LED_ERROR_CLOUD);
    lte_shutdown_modem();
    k_thread_suspend(k_current_get());
}

static void cloud_error_handler(int err)
{
    error_handler(ERROR_CLOUD, err);
}

void connect_to_cloud(const int32_t connect_delay_s)
{
    static bool initial_connect = true;

    /* Ensure no data can be sent to cloud before connection is established. */
    atomic_set(&cloud_association, CLOUD_ASSOCIATION_STATE_INIT);

    if (atomic_get(&carrier_requested_disconnect)) {
        /* A dosconenct was requested to free up the TLS socket
         * used by the cloud. If enabled, the carrier library
         * (CONFIG_LWM2M_CARRIER) will perform FOTA updates in
         * the backlground and reboot the device when complete.
         */
        return;
    }

    atomic_inc(&cloud_connect_attempts);

    /* Check if max cloud connect retry count is exceeded. */
    if (atomic_get(&cloud_connect_attempts) > CONFIG_CLOUD_CONNECT_COUNT_MAX) {
        LOG_ERR("The max cloud connection attempts count exceeded.");
        cloud_error_handler(-ETIMEDOUT);
    }

    if (!initial_connect) {
        LOG_INF("Attempting reconnect in %d seconds...", connect_delay_s);
        k_work_cancel_delayable(&cloud_reboot_work);
    } else {
        initial_connect = false;
    }

    k_work_reschedule_for_queue(work_q, &cloud_connect_work,
            K_SECONDS(connect_delay_s));
}

static void connection_event_handler(const struct cloud_event *const evt)
{
    if (evt->type == CLOUD_EVT_CONNECTING) {
        LOG_INF("CLOUD_EVT_CONNECTING");
        // TODO ui_led_set_pattern(UI_CLOUD_CONNECTING):
        k_work_cancel_delayable(&cloud_reboot_work);

        if (evt->data.err != CLOUD_CONNECT_RES_SUCCESS) {
            cloud_connect_error_handler(evt->data.err);
        }

        return;

    } else if (evt->type == CLOUD_EVT_CONNECTED) {
        LOG_INF("CLOUD_EVT_CONNECTED");
        k_work_cancel_delayable(&cloud_reboot_work);
        k_sem_take(&cloud_disconnected, K_NO_WAIT);
        atomic_set(&cloud_connect_attempts, 0);
#if !IS_ENABLED(CONFIG_MQTT_CLEAN_SESSION)
        LOG_INF("Persistant Sessions = %u", evt->data.persistent_session);
#endif
    } else if (evt->type == CLOUD_EVT_DISCONNECTED) {
        int32_t connect_wait_s = CONFIG_CLOUD_CONNECT_RETRY_DELAY;

        LOG_INF("CLOUD_EVT_DISCONNECTED: %d", evt->data.err);
        // TODO ui_led_set_pattern(UI_LTE_CONNECTED);
        
        switch (evt->data.err) {
            case CLOUD_DISCONNECT_INVALID_REQUEST:
                LOG_INF("Cloud connection closed.");
                break;
            
            case CLOUD_DISCONNECT_USER_REQUEST:
                if (atomic_get(&cloud_association) == 
                    CLOUD_ASSOCIATION_STATE_RECONNECT ||
                    atomic_get(&cloud_association) ==
                    CLOUD_ASSOCIATION_STATE_REQUESTED ||
                    (atomic_get(&carrier_requested_disconnect))) {
                        connect_wait_s = 10;
                }

                break;

            case CLOUD_DISCONNECT_CLOSED_BY_REMOTE:
                LOG_INF("Disconnected by the cloud.");

                if ((atomic_get(&cloud_connect_attempts) == 1) &&
                    (atomic_get(&cloud_association) ==
                     CLOUD_ASSOCIATION_STATE_INIT)) {
                        LOG_INF("This can occure during initial nRF Cloud provisioning/");
                } else {
                    LOG_INF("This can occur if the device has the wrong nRF Cloud certificates");
                    LOG_INF("or if the device has been removed from nRF Cloud.");
                }

                break;

            case CLOUD_DISCONNECT_MISC:
            default:
                break;
        }
        k_sem_give(&cloud_disconnected);
        connect_to_cloud(connect_wait_s);
    }
}

static void cycle_cloud_connection(struct k_work *work)
{
    int32_t reboot_wait_ms = REBOOT_AFTER_DISCONNECT_WAIT_MS;

    LOG_INF("Disconnecting from cloud...");

    if (cloud_disconnect(cloud_backend) != 0) {
        reboot_wait_ms = 5 * MSEC_PER_SEC;
        LOG_INF("Disconnect failed. Device will reboot in  %d seconds",
                (reboot_wait_ms / MSEC_PER_SEC));
    }

    /* Reboot fail-safe on disconnect */
    k_work_reschedule_for_queue(work_q, &cloud_reboot_work,
            K_MSEC(reboot_wait_ms));
}

/**@brief nRF Cloud specific callback for cloud assiciation event. */
static void on_user_pairing_req(const struct cloud_event *evt)
{
    if (atomic_get(&cloud_association) != CLOUD_ASSOCIATION_STATE_REQUESTED) {
        atomic_set(&cloud_association, CLOUD_ASSOCIATION_STATE_REQUESTED);
        // TODO ui_led_set_pattern(UI_CLOUD_PAIRING);
        LOG_INF("Add device to cloud account.");
        LOG_INF("Wating for cloud association...");

        /* If the association is not done soon enough (< ~5 min?)
         * a connection cycle is needed... TBDwhy.
         */
        k_work_reschedule_for_queue(work_q,
                &cycle_cloud_connection_work,
                CONN_CYCLE_AFTER_ASSOCIATION_REQ_MS);
    }
}

static void on_pairing_done(void)
{
    if (atomic_get(&cloud_association) == CLOUD_ASSOCIATION_STATE_REQUESTED) {
        k_work_cancel_delayable(&cycle_cloud_connection_work);

        /* After successful association, the device must
         * reconnect to the cloud.
         */
        LOG_INF("Device associated with cloud.");
        LOG_INF("Reconnecting for cloud policy to take effect.");
        atomic_set(&cloud_association, CLOUD_ASSOCIATION_STATE_RECONNECT);
        k_work_reschedule_for_queue(work_q,
                &cycle_cloud_connection_work, K_NO_WAIT);
    } else {
        atomic_set(&cloud_association, CLOUD_ASSOCIATION_STATE_PAIRED);
    }
}

static void cloud_event_handler(const struct cloud_backend *const backend,
        const struct cloud_event *const evt, void *user_data)
{
    ARG_UNUSED(user_data);

    switch (evt->type) {
        case CLOUD_EVT_CONNECTED:
        case CLOUD_EVT_CONNECTING:
        case CLOUD_EVT_DISCONNECTED:
            connection_event_handler(evt);
            break;

        case CLOUD_EVT_READY:
            LOG_INF("CLOUD_EVT_READY");
            // TODO ui_led_set_pattern(UI_CLOUD_CONNECTED);
            // TODO boot_write_img_confirmed
            atomic_set(&cloud_association, CLOUD_ASSOCIATION_STATE_READY);
            break;

        case CLOUD_EVT_ERROR:
            LOG_INF("CLOUD_EVT_ERROR");
            break;

        case CLOUD_EVT_DATA_SENT:
            LOG_INF("CLOUD_EVT_DATA_SENT");
            break;

        case CLOUD_EVT_DATA_RECEIVED:
            LOG_INF("CLOUD_EVT_DATA_RECEIVED");
	    gateway_handler(&evt->data.msg);
            break;
        
        case CLOUD_EVT_PAIR_REQUEST:
            LOG_INF("CLOUD_EVT_PAIR__REQUEST");
            on_user_pairing_req(evt);
            break;

        case CLOUD_EVT_PAIR_DONE:
            LOG_INF("CLOUD_EVT_PAIR_DONE");
            on_pairing_done();
            break;
        
        default:
            LOG_WRN("Unknown cloud event type: %d", evt->type);
            break;
    }
}

#if defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME)
static int gw_client_id_get(char **id, size_t *id_len)
{
	char psk_buf[AT_CMNG_READ_LEN];
	char get_psk_id[32];
	int bytes_written;
	int bytes_read;
	int at_socket_fd;
	int ret = 0;
	int err;
	int len;

	at_socket_fd = nrf_socket(NRF_AF_LTE, NRF_SOCK_DGRAM, NRF_PROTO_AT);
	if (at_socket_fd < 0) {
		LOG_ERR("Cannot open socket: %d", at_socket_fd);
		return -EIO;
	}

	len = snprintf(get_psk_id, sizeof(get_psk_id), GET_PSK_ID, CONFIG_NRF_CLOUD_SEC_TAG);
	bytes_written = nrf_write(at_socket_fd, get_psk_id, len);
	if (bytes_written != len) {
		LOG_ERR("Cannot send %s: %d", GET_PSK_ID, bytes_written);
		ret = -EIO;
		goto cleanup;
	}
	bytes_read = nrf_read(at_socket_fd, psk_buf, AT_CMNG_READ_LEN);
	if (bytes_read <= 0) {
		LOG_ERR("Cannot read AT command response: %d", bytes_read);
		ret = -EIO;
		goto cleanup;
	}

	if (!strncmp(psk_buf, GET_PSK_ID_ERR, strlen(GET_PSK_ID_ERR))) {
		strcpy(gateway_id, "no-psk-ids");
		*id = gateway_id;
		*id_len = strlen(*id);
		LOG_ERR("Error returned from AT command; PSK ID not set");
	} else {
/*
 * below, we extract the 'nrf-124578' portion as the gateway_id
 * AT%CMNG=2,16842753,4 returns:
 * %CMNG: 16842753,
 * 4,
 * "0404040404040404040404040404040404040404040404040404040404040404",
 * "nrf-124578"
 */
		int ofs;
		int i;
		int len = strlen(psk_buf);
		char *ptr = psk_buf;
		const char *delimiters = ",";

		LOG_DBG("ID is inside this: %s", log_strdup(psk_buf));
		for (i = 0; i < 3; i++) {
			ofs = strcspn(ptr, delimiters) + 1;
			ptr += ofs;
			len -= ofs;
			if (len <= 0) {
				break;
			}
		}
		if (len > 0) {
			if (*ptr == '"') {
				ptr++;
			}
			memcpy(gateway_id, ptr, NRF_CLOUD_CLIENT_ID_LEN);
			gateway_id[NRF_CLOUD_CLIENT_ID_LEN] = 0;
			*id = gateway_id;
			*id_len = strlen(gateway_id);

		} else {
			LOG_ERR("No GWID set in PSK!");
			ret = -EINVAL;
		}
	}

cleanup:
	err = nrf_close(at_socket_fd);
	if (err != 0) {
		ret = -EIO;
	}
	return ret;
}
#endif // CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME

static void cloud_api_init(void)
{
    int ret;

    cloud_backend = cloud_get_binding("NRF_CLOUD");
    __ASSERT(cloud_backend != NULL, "nRF Cloud backend not found");

#if defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME)
    ret = gw_client_id_get(&cloud_backend->config->id, &cloud_backend->config->id_len);

    if (ret) {
	    LOG_ERR("Could not get gateway client ID, error: %d", ret);
    }
#endif

    ret = cloud_init(cloud_backend, cloud_event_handler);

    if (ret) {
        LOG_ERR("Cloud backend could not be initialized, error: %d", ret);
        cloud_error_handler(ret);
    }

    if (ret) {
        LOG_ERR("Cloud command decoder could not be initialized, error: %d",
                ret);
        cloud_error_handler(ret);
    }
}

/**@brief Reboot the device if CONNACK has not arrived. */
static void cloud_reboot_handler(struct k_work *work)
{
    LOG_ERR("Cloud CONNACK Timeout");
    error_handler(ERROR_CLOUD, -ETIMEDOUT);
}

static void cloud_connect_work_fn(struct k_work *work)
{
    int ret;

    LOG_INF("Connecting to cloud, attempt %d of %d",
            atomic_get(&cloud_connect_attempts),
            CONFIG_CLOUD_CONNECT_COUNT_MAX);

    k_work_reschedule_for_queue(work_q, &cloud_reboot_work,
            K_MSEC(CLOUD_CONNACK_WAIT_DURATION));

    // TODO ui_led_set_pattern(UI_CLOUD_CONNECTING);

    /* Attempt cloud connection */
    ret = cloud_connect(cloud_backend);
    if (ret != CLOUD_CONNECT_RES_SUCCESS) {
        k_work_cancel_delayable(&cloud_reboot_work);
        /* Will not return from this function.
         * If the connect failes here, it is likely
         * that user intervention is required.
         */
        cloud_connect_error_handler(ret);
    } else {
        LOG_INF("Cloud connection request sent.");
        LOG_INF("Connection response timeout is set to %d seconds.",
                CLOUD_CONNACK_WAIT_DURATION / MSEC_PER_SEC);
        k_work_reschedule_for_queue(work_q, &cloud_reboot_work,
                K_MSEC(CLOUD_CONNACK_WAIT_DURATION));
    }
}

static void work_init(void)
{
    k_work_init_delayable(&cloud_reboot_work, cloud_reboot_handler);
    k_work_init_delayable(&cycle_cloud_connection_work, cycle_cloud_connection);
    k_work_init_delayable(&cloud_connect_work, cloud_connect_work_fn);
}

void gw_cloud_error_callback(void)
{
    atomic_set(&cloud_association, CLOUD_ASSOCIATION_STATE_INIT);
}

const char *gw_cloud_get_id(void)
{
    int err;

    err = cloud_get_id(cloud_backend, device_id, sizeof(device_id));

    if (err) {
        LOG_ERR("Failed to get device ID from cloud backend. Error: %d", err);
        return NULL;
    }

    return device_id;
}

int gw_cloud_init(struct k_work_q *_work_q,
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

    cloud_api_init();

    work_init();

    connect_to_cloud(0);

    return 0;
}
