#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>
#include <logging/log.h>
#include <settings/settings.h>

/* Include mesh stack header for direct access to mesh access layer messaging. */
#include "mesh/net.h"
#include "mesh/access.h"

#include "btmesh.h"
#include "gateway.h"
#ifdef CONFIG_SHELL
#include "cli.h"
#endif
#include "util.h"


LOG_MODULE_REGISTER(app_btmesh, CONFIG_NRF_CLOUD_MESH_GATEWAY_LOG_LEVEL);


#define BEACON_TIMEOUT_DURATION K_SECONDS(6) /* One second more than the zephyr defined unprovisioned device beacon period */
#define MAX_BEACON 32
#define BEACON_BLOCKED_SIZE 32
#define COMP_DATA_PAGE 0x00
#define COMP_DATA_BUF_SIZE 64
#define MESH_RETRY_COUNT 3
#define PERIOD_PUB_100MS_STR "100ms"
#define PERIOD_PUB_1S_STR "1s"
#define PERIOD_PUB_10S_STR "10s"
#define PERIOD_PUB_10M_STR "10m"

/* SIG Model Strings */
#define CONFIG_SRV_STR \
        "Configuration Server"
#define CONFIG_CLI_STR \
        "Configuration Client"
#define HEALTH_SRV_STR \
        "Health Server"
#define HEALTH_CLI_STR \
        "Health Client"
#define GEN_ONOFF_SRV_STR \
        "Generic On Off Server"
#define GEN_ONOFF_CLI_STR \
        "Generic On Off Client"
#define GEN_LVL_SRV_STR \
        "Generic Level Server"
#define GEN_LVL_CLI_STR \
        "Generic Level Client"
#define GEN_DEF_TRANS_TM_SRV_STR \
        "Generic Default Transition Time Server"
#define GEN_DEF_TRANS_TM_CLI_STR \
        "Generic Default Transition Time Client"
#define GEN_PWR_ONOFF_SRV_STR \
        "Generic Power On Off Server"
#define GEN_PWR_ONOFF_SETUP_SRV_STR \
        "Generic Power On Off Setup Server"
#define GEN_PWR_ONOFF_CLI_STR \
        "Generic Power On Off Client"
#define GEN_PWR_LVL_SRV_STR \
        "Generic Power Level Server"
#define GEN_PWR_LVL_SETUP_SRV_STR \
        "Generic Power Level Setup Server"
#define GEN_PWR_LVL_CLI_STR \
        "Generic Power Level Client"
#define GEN_BATT_SRV_STR \
        "Generic Battery Server"
#define GEN_BATT_CLI_STR \
        "Generic Battery Client"
#define GEN_LOC_SRV_STR \
        "Generic Location Server"
#define GEN_LOC_CLI_STR \
        "Generic Location Client"
#define GEN_ADMN_PROP_SRV_STR \
        "Generic Admin Property Server"
#define GEN_MAN_PROP_SRV_STR \
        "Generic Manufacterer Property Server"
#define GEN_USR_PROP_SRV_STR \
        "Generic User Property Server"
#define GEN_CLI_PROP_SRV_STR \
        "Generic Client Property Server"
#define GEN_PROP_CLI_STR \
        "Generic Property Client"
#define SEN_SRV_STR \
        "Sensor Server"
#define SEN_SETUP_SRV_STR \
        "Sensor Setup Server"
#define SEN_CLI_STR \
        "Sensor Client"
#define TIME_SRV_STR \
        "Time Server"
#define TIME_SETUP_SRV_STR \
        "Time Setup Server"
#define TIME_CLI_STR \
        "Time Client"
#define SCN_SRV_STR \
        "Scene Server"
#define SCN_SETUP_SRV_STR \
        "Scene Setup Server"
#define SCN_CLI_STR \
        "Scene Client"
#define SCD_SRV_STR \
        "Schedule Server"
#define SCD_SETUP_SRV_STR \
        "Schedule Setup Server"
#define SCD_CLI_STR \
        "Schedule Client"
#define LT_LTNESS_SRV_STR \
        "Light Lightness Server"
#define LT_LTNESS_SETUP_SRV_STR \
        "Light Lightness Setup Server"
#define LT_LTNESS_CLI_STR \
        "Light Lightness Client"
#define LT_CTL_SRV_STR \
        "Light CTL Server"
#define LT_CTL_SETUP_SRV_STR \
        "Light CTL Setup Server"
#define LT_CTL_CLI_STR \
        "Light CTL Client"
#define LT_CTL_TEMP_SRV_STR \
        "Light CTL Temperature Server"
#define LT_HSL_SRV_STR \
        "Light HSL Server"
#define LT_HSL_SETUP_SRV_STR \
        "Light HSL Setup Server"
#define LT_HSL_CLI_STR \
        "Light HSL Client"
#define LT_HSL_HUE_SRV_STR \
        "Light HSL Hue Server"
#define LT_HSL_SAT_SRV_STR \
        "Light HSL Saturation Server"
#define LT_XYL_SRV_STR \
        "Light xyL Server"
#define LT_XYL_SETUP_SRV_STR \
        "Light xyL Setup Server"
#define LT_XYL_CLI_STR \
        "Light xy: Client"
#define LT_LC_SRV_STR \
        "Light LC Server"
#define LT_LC_SETUP_SRV_STR \
        "Light LC Setup Server"
#define LT_LC_CLI_STR \
        "Light LC Client"
#define NULL_STR \
        ""

static const char OOB_OTHER[]       = "OTHER";
static const char OOB_URI[]         = "URI";
static const char OOB_2D_CODE[]     = "2D CODE";
static const char OOB_BAR_CODE[]    = "BAR CODE";
static const char OOB_NFC[]         = "NFC";
static const char OOB_NUMBER[]      = "NUMBER";
static const char OOB_STRING[]      = "STRING";
static const char OOB_ON_BOX[]      = "ON BOX";
static const char OOB_IN_BOX[]      = "IN BOX";
static const char OOB_ON_PAPER[]    = "ON PAPER";
static const char OOB_IN_MANUAL[]   = "IN MANUAL";
static const char OOB_ON_DEV[]      = "ON DEVICE";

enum btmesh_sig_model_id {
        CONFIG_SRV = 0x0000,
        CONFIG_CLI,
        HEALTH_SRV,
        HEALTH_CLI,
        GEN_ONOFF_SRV = 0x1000,
        GEN_ONOFF_CLI,
        GEN_LVL_SRV,
        GEN_LVL_CLI,
        GEN_DEF_TRANS_TM_SRV,
        GEN_DEF_TRANS_TM_CLI,
        GEN_PWR_ONOFF_SRV,
        GEN_PWR_ONOFF_SETUP_SRV,
        GEN_PWR_ONOFF_CLI,
        GEN_PWR_LVL_SRV,
        GEN_PWR_LVL_SETUP_SRV,
        GEN_PWR_LVL_CLI,
        GEN_BATT_SRV,
        GEN_BATT_CLI,
        GEN_LOC_SRV,
        GEN_LOC_CLI,
        GEN_ADMN_PROP_SRV,
        GEN_MAN_PROP_SRV,
        GEN_USR_PROP_SRV,
        GEN_CLI_PROP_SRV,
        GEN_PROP_CLI,
        SEN_SRV = 0x1100,
        SEN_SETUP_SRV,
        SEN_CLI,
        TIME_SRV = 0x1200,
        TIME_SETUP_SRV,
        TIME_CLI,
        SCN_SRV,
        SCN_SETUP_SRV,
        SCN_CLIR,
        SCD_SRV,
        SCD_SETUP_SRV,
        SCD_CLI,
        LT_LTNESS_SRV = 0x1300,
        LT_LTNESS_SETUP_SRV,
        LT_LTNESS_CLI,
        LT_CTL_SRV,
        LT_CTL_SETUP_SRV,
        LT_CTL_CLI,
        LT_CTL_TEMP_SRV,
        LT_HSL_SRV,
        LT_HSL_SETUP_SRV,
        LT_HSL_CLI,
        LT_HSL_HUE_SRV,
        LT_HSL_SAT_SRV,
        LT_XYL_SRV,
        LT_XYL_SETUP_SRV,
        LT_XYL_CLI,
        LT_LC_SRV,
        LT_LC_SETUP_SRV,
        LT_LC_CLI
};

const char* btmesh_parse_opcode(uint32_t opcode)
{
        if (opcode == 0x8201) {
                return "Generic OnOff Get";
        } else if (opcode == 0x8202) {
                return "Generic OnOff Set";
        } else if (opcode == 0x8203) {
                return "Generic OnOff Set Unacknowledged";
        } else if (opcode == 0x8204) {
                return "Generic OnOff Status";
        } else if (opcode == 0x8205) {
                return "Generic Level Get";
        } else if (opcode == 0x8206) {
                return "Generic Level Set";
        } else if (opcode == 0x8207) {
                return "Generic Level Set Unacknowledged";
        } else if (opcode == 0x8208) {
                return "Generic Level Status";
        } else if (opcode == 0x8209) {
                return "Generic Delta Set";
        } else if (opcode == 0x820A) {
                return "Generic Delta Set Unacknowledged";
        } else if (opcode == 0x820B) {
                return "Generic Move Set";
        } else if (opcode == 0x820C) {
                return "Generic Move Set Unacknowledged";
        } else if (opcode == 0x820D) {
                return "Generic Default Transition Time Get";
        } else if (opcode == 0x820E) {
                return "Generic Default Transition Time Set";
        } else if (opcode == 0x820F) {
                return "Generic Default Transition Time Set";
        } else if (opcode == 0x8210) {
                return "Generic Default Transition Time Status";
        } else if (opcode == 0x8211) {
                return "Generic OnPowerUp Get";
        } else if (opcode == 0x8212) {
                return "Generic OnPowerUp Status";
        } else if (opcode == 0x8213) {
                return "Generic OnPowerUp Set";
        } else if (opcode == 0x8214) {
                return "Generic OnPowerUp Set Unacknowledged";
        } else if (opcode == 0x8215) {
                return "Generic Power Level Get";
        } else if (opcode == 0x8216) {
                return "Generic Power Level Set";
        } else if (opcode == 0x8217) {
                return "Generic Power Level Set Unacknowledged";
        } else if (opcode == 0x8218) {
                return "Generic Power Level Status";
        } else if (opcode == 0x8219) {
                return "Generic Power Last Get";
        } else if (opcode == 0x821A) {
                return "Generic Power Last Status";
        } else if (opcode == 0x821B) {
                return "Generic Power Default Get";
        } else if (opcode == 0x821C) {
                return "Generic Power Default Status";
        } else if (opcode == 0x821D) {
                return "Generic Power Range Get";
        } else if (opcode == 0x821E) {
                return "Generic Power Range Status";
        } else if (opcode == 0x821F) {
                return "Generic Power Default Set";
        } else if (opcode == 0x8220) {
                return "Generic Power Default Set Unacknowledged";
        } else if (opcode == 0x8221) {
                return "Generic Power Range Set";
        } else if (opcode == 0x8222) {
                return "Generic Power Range Set Unacknowledged";
        } else if (opcode == 0x8223) {
                return "Generic Battery Get";
        } else if (opcode == 0x8224) {
                return "Generic Battery Status";
        } else if (opcode == 0x8225) {
                return "Generic Location Global Get";
        } else if (opcode == 0x40) {
                return "Generic Location Global Status";
        } else if (opcode == 0x8226) {
                return "Generic Location Local Get";
        } else if (opcode == 0x8227) {
                return "Generic Location Local Status";
        } else if (opcode == 0x41) {
                return "Generic Location Global Set";
        } else if (opcode == 0x42) {
                return "Generic Location Global Set Unacknowledged";
        } else if (opcode == 0x8228) {
                return "Generic Location Local Set";
        } else if (opcode == 0x8229) {
                return "Generic Location Local Set Unacknowledged";
        } else if (opcode == 0x822A) {
                return "Generic Manufacturer Properties Get";
        } else if (opcode == 0x43) {
                return "Generic Manufacturer Properties Status";
        } else if (opcode == 0x822B) {
                return "Generic Manufacturer Property Get";
        } else if (opcode == 0x44) {
                return "Generic Manufacturer Property Set";
        } else if (opcode == 0x45) {
                return "Generic Manufacturer Property Set Unacknowledged";
        } else if (opcode == 0x46) {
                return "Generic Manufacturer Property Status";
        } else if (opcode == 0x822C) {
                return "Generic Admin Properties Get";
        } else if (opcode == 0x47) {
                return "Generic Admin Properties Status";
        } else if (opcode == 0x822D) {
                return "Generic Admin Property Get";
        } else if (opcode == 0x48) {
                return "Generic Admin Property Set";
        } else if (opcode == 0x49) {
                return "Generic Admin Property Set Unacknowledged";
        } else if (opcode == 0x4A) {
                return "Generic Admin Property Status";
        } else if (opcode == 0x822E) {
                return "Generic User Properties Get";
        } else if (opcode == 0x4B) {
                return "Generic User Properties Status";
        } else if (opcode == 0x822F) {
                return "Generic User Property Get";
        } else if (opcode == 0x4C) {
                return "Generic User Property Set";
        } else if (opcode == 0x4D) {
                return "Generic User Property Set Unacknowledged";
        } else if (opcode == 0x4E) {
                return "Generic User Property Status";
        } else if (opcode == 0x4F) {
                return "Generic Client Properties Get";
        } else if (opcode == 0x50) {
                return "Generic Client Properties Status";
        } else {
                return "";
        }
}

const char *btmesh_get_sig_model_str(uint16_t model_id)
{
        switch (model_id) {
                case CONFIG_SRV:
                        return CONFIG_SRV_STR;
                case CONFIG_CLI:
                        return CONFIG_CLI_STR;
                case HEALTH_SRV:
                        return HEALTH_SRV_STR;
                case HEALTH_CLI:
                        return HEALTH_CLI_STR;
                case GEN_ONOFF_SRV:
                        return GEN_ONOFF_SRV_STR;
                case GEN_ONOFF_CLI:
                        return GEN_ONOFF_CLI_STR;
                case GEN_LVL_SRV:
                        return GEN_LVL_SRV_STR;
                case GEN_LVL_CLI:
                        return GEN_LVL_CLI_STR;
                case GEN_DEF_TRANS_TM_SRV:
                        return GEN_DEF_TRANS_TM_SRV_STR;
                case GEN_DEF_TRANS_TM_CLI:
                        return GEN_DEF_TRANS_TM_CLI_STR;
                case GEN_PWR_ONOFF_SRV:
                        return GEN_PWR_ONOFF_SRV_STR;
                case GEN_PWR_ONOFF_SETUP_SRV:
                        return GEN_PWR_ONOFF_SETUP_SRV_STR;
                case GEN_PWR_ONOFF_CLI:
                        return GEN_PWR_ONOFF_CLI_STR;
                case GEN_PWR_LVL_SRV:
                        return GEN_PWR_LVL_SRV_STR;
                case GEN_PWR_LVL_SETUP_SRV:
                        return GEN_PWR_LVL_SETUP_SRV_STR;
                case GEN_PWR_LVL_CLI:
                        return GEN_PWR_LVL_CLI_STR;
                case GEN_BATT_SRV:
                        return GEN_BATT_SRV_STR;
                case GEN_BATT_CLI:
                        return GEN_BATT_CLI_STR;
                case GEN_LOC_SRV:
                        return GEN_LOC_SRV_STR;
                case GEN_LOC_CLI:
                        return GEN_LOC_CLI_STR;
                case GEN_ADMN_PROP_SRV:
                        return GEN_ADMN_PROP_SRV_STR;
                case GEN_MAN_PROP_SRV:
                        return GEN_MAN_PROP_SRV_STR;
                case GEN_USR_PROP_SRV:
                        return GEN_USR_PROP_SRV_STR;
                case GEN_CLI_PROP_SRV:
                        return GEN_CLI_PROP_SRV_STR;
                case GEN_PROP_CLI:
                        return GEN_PROP_CLI_STR;
                case SEN_SRV:
                        return SEN_SRV_STR;
                case SEN_SETUP_SRV:
                        return SEN_SETUP_SRV_STR;
                case SEN_CLI:
                        return SEN_CLI_STR;
                case TIME_SRV:
                        return TIME_SRV_STR;
                case TIME_SETUP_SRV:
                        return TIME_SETUP_SRV_STR;
                case TIME_CLI:
                        return TIME_CLI_STR;
                case SCN_SRV:
                        return SCN_SRV_STR;
                case SCN_SETUP_SRV:
                        return SCN_SETUP_SRV_STR;
                case SCN_CLIR:
                        return SCN_CLI_STR;
                case SCD_SRV:
                        return SCD_SRV_STR;
                case SCD_SETUP_SRV:
                        return SCD_SETUP_SRV_STR;
                case SCD_CLI:
                        return SCD_CLI_STR;
                case LT_LTNESS_SRV:
                        return LT_LTNESS_SRV_STR;
                case LT_LTNESS_SETUP_SRV:
                        return LT_LTNESS_SETUP_SRV_STR;
                case LT_LTNESS_CLI:
                        return LT_LTNESS_CLI_STR;
                case LT_CTL_SRV:
                        return LT_CTL_SRV_STR;
                case LT_CTL_SETUP_SRV:
                        return LT_CTL_SETUP_SRV_STR;
                case LT_CTL_CLI:
                        return LT_CTL_CLI_STR;
                case LT_CTL_TEMP_SRV:
                        return LT_CTL_TEMP_SRV_STR;
                case LT_HSL_SRV:
                        return LT_HSL_SRV_STR;
                case LT_HSL_SETUP_SRV:
                        return LT_HSL_SETUP_SRV_STR;
                case LT_HSL_CLI:
                        return LT_HSL_CLI_STR;
                case LT_HSL_HUE_SRV:
                        return LT_HSL_HUE_SRV_STR;
                case LT_HSL_SAT_SRV:
                        return LT_HSL_SAT_SRV_STR;
                case LT_XYL_SRV:
                        return LT_XYL_SRV_STR;
                case LT_XYL_SETUP_SRV:
                        return LT_XYL_SETUP_SRV_STR;
                case LT_XYL_CLI:
                        return LT_XYL_CLI_STR;
                case LT_LC_SRV:
                        return LT_LC_SRV_STR;
                case LT_LC_SETUP_SRV:
                        return LT_LC_SETUP_SRV_STR;
                case LT_LC_CLI:
                        return LT_LC_CLI_STR;
                default:
                        return NULL_STR;
        }
}

static uint16_t cli_sub_list[CONFIG_BT_MESH_GATEWAY_SUB_LIST_LEN];
static uint16_t gateway_sub_list[CONFIG_BT_MESH_GATEWAY_SUB_LIST_LEN];

int btmesh_subscribe(enum btmesh_sub_type type, uint16_t addr)
{
        int i;
        uint16_t *sub_list;

        if (type == BTMESH_SUB_TYPE_CLI) {
                sub_list = cli_sub_list;
        } else if (type == BTMESH_SUB_TYPE_GATEWAY) {
                sub_list = gateway_sub_list;
        } else {
                return -EINVAL;
        }

        for (i = 0; i < CONFIG_BT_MESH_GATEWAY_SUB_LIST_LEN; i++) {
                if (sub_list[i] == addr) {
                        return 0;
                }

                if (sub_list[i] == BT_MESH_ADDR_UNASSIGNED) {
                        sub_list[i] = addr;
                        return 0;
                }
        }

        return -ENOMEM;
}

void btmesh_unsubscribe(enum btmesh_sub_type type, uint16_t addr)
{
        int i;
        uint16_t *sub_list;

        if (type == BTMESH_SUB_TYPE_CLI) {
                sub_list = cli_sub_list;
        } else if (type == BTMESH_SUB_TYPE_GATEWAY) {
                sub_list = gateway_sub_list;
        } else {
                return;
        }

        for (i = 0; i < CONFIG_BT_MESH_GATEWAY_SUB_LIST_LEN; i++) {
                if (sub_list[i] == addr) {
                        sub_list[i] = BT_MESH_ADDR_UNASSIGNED;
                }
        }
}

const uint16_t *btmesh_get_subscribe_list(enum btmesh_sub_type type)
{
        if (type == BTMESH_SUB_TYPE_CLI) {
                 return cli_sub_list;
        } else if (type == BTMESH_SUB_TYPE_GATEWAY) {
                return gateway_sub_list;
        } else {
                return NULL;
        }
}

struct beacon {
        uint8_t uuid[UUID_LEN];
        char uuid_str[UUID_STR_LEN];
        bt_mesh_prov_oob_info_t oob_info;
        uint32_t uri_hash;
        struct k_timer *timer;
};

struct blocked_beacon {
        uint8_t uuid[UUID_LEN];
        char uuid_str[UUID_STR_LEN];
};

static struct beacon beacon_list[MAX_BEACON];
static struct beacon *beacon_tail = beacon_list;
static struct k_timer beacon_timer_list[MAX_BEACON];
static struct blocked_beacon blocked_list[BEACON_BLOCKED_SIZE];
static struct blocked_beacon *blocked_tail = blocked_list;

static bool beacon_is_blocked(uint8_t uuid[UUID_LEN])
{
        struct blocked_beacon *blocked_ptr;

        blocked_ptr = blocked_list;
        while (blocked_ptr != blocked_tail) {
                if (!util_uuid_cmp(uuid, blocked_ptr->uuid)) {
                        return true;
                }
                blocked_ptr++;
        }

        return false;
}

const char *btmesh_get_blocked_beacon(size_t idx)
{
        if (&blocked_list[idx] >= blocked_tail) {
                return NULL;
        }

        return blocked_list[idx].uuid_str;
}

int btmesh_block_beacon(uint8_t uuid[UUID_LEN])
{
        struct blocked_beacon *blocked_ptr;

        /* Beacon block list is full. */
        if (blocked_tail == blocked_list + BEACON_BLOCKED_SIZE) {
                return -ENOMEM;
        }

        /* Beacon is already blocked. */
        if (beacon_is_blocked(uuid)) {
                return -EAGAIN;
        }

        /* Add to blocked list while maintaining order of uuid strings within the
         * list.
         */
        blocked_ptr = blocked_tail;
        while (blocked_ptr != blocked_list) {
                blocked_ptr--;

                if (util_uuid_cmp(uuid, blocked_ptr->uuid) < 0) {
                        memcpy(blocked_ptr + 1, blocked_ptr, sizeof(struct blocked_beacon));
                } else {
                        blocked_ptr++;
                        break;
                }
        }

        util_uuid_cpy(blocked_ptr->uuid, uuid);
        util_uuid2str(blocked_ptr->uuid, blocked_ptr->uuid_str);
        blocked_tail++;
        return 0;
}

int btmesh_unblock_beacon(uint8_t uuid[UUID_LEN])
{
        struct blocked_beacon *blocked_ptr;
        bool found = false;

        blocked_ptr = blocked_list;
        while (blocked_ptr != blocked_tail) {
                if (found) {
                        memcpy(blocked_ptr, blocked_ptr + 1, sizeof(struct blocked_beacon));
                } else {
                        if (!util_uuid_cmp(uuid, blocked_ptr->uuid)) {
                                found = true;
                                memcpy(blocked_ptr, blocked_ptr + 1, sizeof(struct blocked_beacon));
                        }
                }
                blocked_ptr++;
        }

        if (found) {
                blocked_tail--;
                return 0;
        }

        return -ESRCH;
}

static struct beacon *beacon_get(uint8_t uuid[UUID_LEN])
{
        struct beacon *beacon_ptr = beacon_list;

        while (beacon_ptr != beacon_tail) {
                if (!util_uuid_cmp(beacon_ptr->uuid, uuid)) {
                        return beacon_ptr;
                }
                beacon_ptr++;
        }

        return NULL;
}

static void beacon_remove_inactive(struct k_work *work)
{
        struct beacon *beacon_ptr_i;
        struct beacon *beacon_ptr_j;

        beacon_ptr_i = beacon_list;

        /* Remove all expired beacons, not just just the first one */
        while (beacon_ptr_i != beacon_tail) {
                /* Timer has expired. Remove beacon from list */
                if (k_timer_status_get(beacon_ptr_i->timer) > 0) {
                        /* Reset the beacon timer user data */
                        k_timer_user_data_set(beacon_ptr_i->timer, NULL);

                        /* Move all other beacons up in the list to preserve order */
                        beacon_ptr_j = beacon_ptr_i;
                        while (beacon_ptr_j + 1 != beacon_tail) {
                                memcpy(beacon_ptr_j, beacon_ptr_j + 1, sizeof(struct beacon));
                                beacon_ptr_j++;
                        }

                        /* Decrement the size of the beacon list */
                        beacon_tail--;

                        /* Continue without incrementing beacon_ptr_i becuase it now points
                         * to the nest beacon in the list and must also be checked */
                        continue;
                }
                beacon_ptr_i++;
        }
}

K_WORK_DEFINE(beacon_work, beacon_remove_inactive);

extern void beacon_timer_handler(struct k_timer *timer)
{
        /* Let the system work que find and remove the expired beacon */
        k_work_submit(&beacon_work);
}

static void beacon_recv(uint8_t uuid[UUID_LEN], bt_mesh_prov_oob_info_t oob_info, uint32_t *uri_hash)
{
        struct beacon* beacon_ptr;
        struct k_timer *timer_ptr;

        /* Make sure the beacon list isn't already full */
        if (beacon_tail == beacon_list + MAX_BEACON) {
                return;
        };

        /* Make sure the beacon isn't blocked.
         * Note that for beacons that are listed before they are blocked, this will
         * allow their timers to expire then they will be removed from the list.
         */
        if (beacon_is_blocked(uuid)) {
                return;
        }

        /* If device beacon is already listed, reset its timer so the beacon stays valid */
        beacon_ptr = beacon_get(uuid);
        if (beacon_ptr != NULL) {
                k_timer_start(beacon_ptr->timer, BEACON_TIMEOUT_DURATION, K_NO_WAIT);
                return;
        }

        /* Add the beacon to the unprovisioned device list while maintaining list is sorted,
         * by comparing the new uuid against the largest (last) listed uuid, and moving larger
         * listed uuids down untill we find the sorted location for the new uuid */
        beacon_ptr = beacon_tail;
        while (beacon_ptr != beacon_list) {
                beacon_ptr--;

                if (util_uuid_cmp(uuid, beacon_ptr->uuid) < 0) {
                        /* The listed uuid is greater than the new uuid so move the listed down one
                         * spot in the list and move on to compare the next listed beacon */
                        memcpy(beacon_ptr + 1, beacon_ptr, sizeof(struct beacon));
                } else {
                        /* The listed uuid is less than the new uuid, so the new uuid goes in this list spot */
                        beacon_ptr++;
                        break;
                }
        }

        /* Enter new beacon into list */
        util_uuid_cpy(beacon_ptr->uuid, uuid);
        util_uuid2str(beacon_ptr->uuid, beacon_ptr->uuid_str);
        beacon_ptr->oob_info = oob_info;
        beacon_ptr->uri_hash = *uri_hash;

        /* Increase size of beacon list */
        beacon_tail++;

        /* Find an unused timer for the new beacon */
        timer_ptr = beacon_timer_list;
        /* If the beacon isn't full we will find a free timer within range fors ure */
        while (k_timer_user_data_get(timer_ptr) != NULL) {
                timer_ptr++;
        }
        beacon_ptr->timer = timer_ptr;

        k_timer_user_data_set(beacon_ptr->timer, beacon_ptr);

        /* Start the expiry timer for the new beacon */
        k_timer_start(beacon_ptr->timer, BEACON_TIMEOUT_DURATION, K_NO_WAIT);
}

const char *btmesh_get_beacon_uuid(size_t idx)
{
        if (&beacon_list[idx] >= beacon_tail) {
                return NULL;
        }

        return beacon_list[idx].uuid_str;
}

const char *btmesh_get_beacon_oob(size_t idx)
{
        if (&beacon_list[idx] >= beacon_tail) {
                return NULL;
        }

        switch (beacon_list[idx].oob_info) {
                case BT_MESH_PROV_OOB_OTHER:
                        return OOB_OTHER;
                case BT_MESH_PROV_OOB_URI:
                        return OOB_URI;
                case BT_MESH_PROV_OOB_2D_CODE:
                        return OOB_2D_CODE;
                case BT_MESH_PROV_OOB_BAR_CODE:
                        return OOB_BAR_CODE;
                case BT_MESH_PROV_OOB_NFC:
                        return OOB_NFC;
                case BT_MESH_PROV_OOB_NUMBER:
                        return OOB_NUMBER;
                case BT_MESH_PROV_OOB_STRING:
                        return OOB_STRING;
                case BT_MESH_PROV_OOB_ON_BOX:
                        return OOB_ON_BOX;
                case BT_MESH_PROV_OOB_IN_BOX:
                        return OOB_IN_BOX;
                case BT_MESH_PROV_OOB_ON_PAPER:
                        return OOB_ON_PAPER;
                case BT_MESH_PROV_OOB_IN_MANUAL:
                        return OOB_IN_MANUAL;
                case BT_MESH_PROV_OOB_ON_DEV:
                        return OOB_ON_DEV;
                default:
                        return NULL;
        }
}

const uint32_t *btmesh_get_beacon_uri_hash(size_t idx)
{
        if (&beacon_list[idx] >= beacon_tail) {
                return NULL;
        }

        return &(beacon_list[idx].uri_hash);
}

uint8_t btmesh_get_pub_period(uint8_t period)
{
        return period & 0x3F;
}

char * btmesh_get_pub_period_unit_str(uint8_t period)
{
        if ((period & 0xC0) == 0x00) {
                return PERIOD_PUB_100MS_STR;
        } else if ((period & 0xC0) == 0x40) {
                return PERIOD_PUB_1S_STR;
        } else if ((period & 0xC0) == 0x80) {
                return PERIOD_PUB_10S_STR;
        } else if ((period & 0xC0) == 0xC0) {
                return PERIOD_PUB_10M_STR;
        } else {
                return NULL;
        }
}

void btmesh_free_node(struct btmesh_node *node)
{
        uint8_t i;
        uint8_t j;

        if (node->elems != NULL) {
                for (i = 0; i < node->elem_count; i++) {
                        if (node->elems[i].sig_models != NULL) {
                                for (j = 0; j < node->elems[i].sig_model_count; j++) {
                                        k_free(node->elems[i].sig_models[j].appkey_idxs);
                                        k_free(node->elems[i].sig_models[j].sub_addrs);
                                }

                                k_free(node->elems[i].sig_models);
                        }

                        if (node->elems[i].vnd_models != NULL) {
                                for (j = 0; j < node->elems[i].vnd_model_count; j++) {
                                        k_free(node->elems[i].vnd_models[j].appkey_idxs);
                                        k_free(node->elems[i].vnd_models[j].sub_addrs);
                                }

                                k_free(node->elems[i].vnd_models);
                        }
                }

                k_free(node->elems);
                node->elems = NULL;
        }
}

static const uint16_t cid = 0xBABE;

static const uint8_t dev_uuid[16] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

static const uint16_t self_addr = 0x0001;

static struct bt_mesh_cfg_cli cfg_cli = {
        /* TODO */
};

static void health_current_status(struct bt_mesh_health_cli *cli, uint16_t addr,
                uint8_t test_id, uint16_t cid, uint8_t *faults, size_t fault_count)
{
	int i;

	LOG_INF("Received Current Health Faults:");
	LOG_INF("    - Address   : 0x%04x", addr);
	LOG_INF("    - Test ID   : 0x%02x", test_id);
	LOG_INF("    - Company ID: 0x%04x", cid);
	LOG_INF("    - Faults");

	for (i = 0; i < fault_count; i++) {
		LOG_INF("          0x%02x", faults[i]);
	}

	if (i == 0) {
		LOG_INF("        None");
	}

	cli_hlth_cb(addr, test_id, cid, faults, fault_count);
	gateway_hlth_cb(addr, test_id, cid, faults, fault_count);
}

static struct bt_mesh_health_cli health_cli = {
        .current_status = health_current_status
};

static struct bt_mesh_model root_models[] = {
        BT_MESH_MODEL_CFG_SRV,
        BT_MESH_MODEL_CFG_CLI(&cfg_cli),
        BT_MESH_MODEL_HEALTH_CLI(&health_cli)
};

static struct bt_mesh_elem elements[] = {
        BT_MESH_ELEM(0, root_models, BT_MESH_MODEL_NONE)
};

static struct bt_mesh_comp mesh_comp = {
        .cid = cid,
        .elem = elements,
        .elem_count = ARRAY_SIZE(elements)
};

static void provision_link_open(bt_mesh_prov_bearer_t bearer)
{
        LOG_INF("Provisioning link open");
}

static void provision_link_close(bt_mesh_prov_bearer_t bearer)
{
        LOG_INF("Provisioning link closed");
}

static void provision_complete(uint16_t net_idx, uint16_t addr)
{
        LOG_INF("Provisioning Complete:");
        LOG_INF("   - Net Index: 0x%04x", net_idx);
        LOG_INF("   - Address  : 0x%04x\n", addr);
}

static void node_added(uint16_t net_idx, uint8_t uuid[UUID_LEN], uint16_t addr, uint8_t num_elem)
{
        char uuid_str[33];

        util_uuid2str(uuid, uuid_str);

        LOG_INF(" New node addes:");
        LOG_INF("- Net Index: 0x%04x", net_idx);
        LOG_INF("- UUID     : %s", log_strdup(uuid_str));
        LOG_INF("- Address  : 0x%04x", addr);
        LOG_INF("- Elements : %d", num_elem);

        gateway_node_added(net_idx, uuid, addr, num_elem); 
}

static const struct bt_mesh_prov prov = {
        .uuid = dev_uuid,
        .unprovisioned_beacon = beacon_recv,
        .link_open = provision_link_open,
        .link_close = provision_link_close,
        .complete = provision_complete,
        .node_added = node_added
};

#if defined(CONFIG_BT_MESH_ACCESS_LAYER_MSG)
static void btmesh_msg_cb(uint32_t opcode, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
        int i;

        /* Ignore configuration opcodes as we have the configuration client model instantiated */
        if ((opcode & 0xFF00) == 0x8000) {
                return;
        }

        /* Ignore health opcodes as we have the health client model instantiated */
        if (opcode <= 0x06) {
                return;
        }

        LOG_DBG("mesh message callback");
        LOG_DBG("    opcode : 0x%08x", opcode);
        LOG_DBG("    net_idx: 0x%04x", ctx->net_idx);
        LOG_DBG("    app_idx: 0x%04x", ctx->app_idx);
        LOG_DBG("    Source Address: 0x%04x", ctx->addr);
        LOG_DBG("    Destination Address: 0x%04x", ctx->recv_dst);
        LOG_HEXDUMP_DBG(buf->data, buf->len, "Payload");

        for (i = 0; i < CONFIG_BT_MESH_GATEWAY_SUB_LIST_LEN; i++) {
                if (cli_sub_list[i] == ctx->recv_dst) {
#ifdef CONFIG_SHELL
                        cli_msg_callback(opcode, ctx, buf);
#endif
                }
                
                if (gateway_sub_list[i] == ctx->recv_dst) {
                        gateway_msg_callback(opcode, ctx, buf);
                }
        }
}
#endif // defined(CONFIG_BT_MESH_ACCESS_LAYER_MSG)

int btmesh_perform_op(enum btmesh_op op, union btmesh_op_args* args)
{
        int i;
        int err;

        for (i = 0; i < MESH_RETRY_COUNT; i++)
        {
                switch (op) {
                case BTMESH_OP_PROV_ADV:
                        err = bt_mesh_provision_adv(args->prov_adv.uuid,
                                        args->prov_adv.net_idx, args->prov_adv.addr,
                                        args->prov_adv.attn);
                        break;

                case BTMESH_OP_NODE_RESET:
                        err = bt_mesh_cfg_node_reset(args->node_reset.net_idx,
                                        args->node_reset.addr, &(args->node_reset.status));
                        break;

                case BTMESH_OP_COMP_GET:
                        err = bt_mesh_cfg_comp_data_get(args->comp_get.net_idx,
                                        args->comp_get.addr, args->comp_get.page,
                                        &(args->comp_get.status), args->comp_get.comp);
                        break;

                case BTMESH_OP_BEACON_GET:
                        err = bt_mesh_cfg_beacon_get(args->beacon_get.net_idx,
                                        args->beacon_get.addr, &(args->beacon_get.status));
                        break;

                case BTMESH_OP_BEACON_SET:
                        err = bt_mesh_cfg_beacon_set(args->beacon_set.net_idx,
                                        args->beacon_set.addr, args->beacon_set.val,
                                        &(args->beacon_set.status));
                        break;

                case BTMESH_OP_TTL_GET:
                        err = bt_mesh_cfg_ttl_get(args->ttl_get.net_idx,
                                        args->ttl_get.addr, &(args->ttl_get.ttl));
                        break;

                case BTMESH_OP_TTL_SET:
                        err = bt_mesh_cfg_ttl_set(args->ttl_set.net_idx,
                                        args->ttl_set.addr, args->ttl_set.val,
                                        &(args->ttl_set.ttl));
                        break;

                case BTMESH_OP_FRIEND_GET:
                        err = bt_mesh_cfg_friend_get(args->friend_get.net_idx,
                                        args->friend_get.addr, &(args->friend_get.status));
                        break;

                case BTMESH_OP_FRIEND_SET:
                        err = bt_mesh_cfg_friend_set(args->friend_set.net_idx,
                                        args->friend_set.addr, args->friend_set.val,
                                        &(args->friend_set.status));
                        break;

                case BTMESH_OP_PROXY_GET:
                        err = bt_mesh_cfg_gatt_proxy_get(args->proxy_get.net_idx,
                                        args->proxy_get.addr, &(args->proxy_get.status));
                        break;

                case BTMESH_OP_PROXY_SET:
                        err = bt_mesh_cfg_gatt_proxy_set(args->proxy_set.net_idx,
                                        args->proxy_set.addr, args->proxy_set.val,
                                        &(args->proxy_set.status));
                        break;

                case BTMESH_OP_RELAY_GET:
                        err = bt_mesh_cfg_relay_get(args->relay_get.net_idx,
                                        args->relay_get.addr, &(args->relay_get.status),
                                        &(args->relay_get.transmit));
                        break;

                case BTMESH_OP_RELAY_SET:
                        err = bt_mesh_cfg_relay_set(args->relay_set.net_idx,
                                        args->relay_set.addr, args->relay_set.new_relay,
                                        args->relay_set.new_transmit,
                                        &(args->relay_set.status),
                                        &(args->relay_set.transmit));
                        break;

                case BTMESH_OP_NET_KEY_ADD:
                        err = bt_mesh_cfg_net_key_add(args->net_key_add.net_idx,
                                        args->net_key_add.addr,
                                        args->net_key_add.key_net_idx,
                                        args->net_key_add.net_key,
                                        &(args->net_key_add.status));
                        break;

                case BTMESH_OP_NET_KEY_GET:
                        err = bt_mesh_cfg_net_key_get(args->net_key_get.net_idx,
                                        args->net_key_get.addr, args->net_key_get.keys,
                                        &(args->net_key_get.key_cnt));
                        break;

                case BTMESH_OP_NET_KEY_DEL:
                        err = bt_mesh_cfg_net_key_del(args->net_key_get.net_idx,
                                        args->net_key_del.addr,
                                        args->net_key_del.key_net_idx,
                                        &(args->net_key_del.status));
                        break;

                case BTMESH_OP_APP_KEY_ADD:
                        err = bt_mesh_cfg_app_key_add(args->app_key_add.net_idx,
                                        args->app_key_add.addr,
                                        args->app_key_add.key_net_idx,
                                        args->app_key_add.key_app_idx,
                                        args->app_key_add.app_key,
                                        &(args->app_key_add.status));
                        break;

                case BTMESH_OP_APP_KEY_GET:
                        err = bt_mesh_cfg_app_key_get(args->app_key_get.net_idx,
                                        args->app_key_get.addr,
                                        args->app_key_get.key_net_idx,
                                        &(args->app_key_get.status),
                                        args->app_key_get.keys,
                                        &(args->app_key_get.key_cnt));
                        break;

                case BTMESH_OP_APP_KEY_DEL:
                        err = bt_mesh_cfg_app_key_del(args->app_key_del.net_idx,
                                        args->app_key_del.addr,
                                        args->app_key_del.key_net_idx,
                                        args->app_key_del.key_app_idx,
                                        &(args->app_key_del.status));
                        break;

                case BTMESH_OP_MOD_APP_BIND:
                        err = bt_mesh_cfg_mod_app_bind(args->mod_app_bind.net_idx,
                                        args->mod_app_bind.addr,
                                        args->mod_app_bind.elem_addr,
                                        args->mod_app_bind.mod_app_idx,
                                        args->mod_app_bind.mod_id,
                                        &(args->mod_app_bind.status));
                        break;

                case BTMESH_OP_MOD_APP_BIND_VND:
                        err = bt_mesh_cfg_mod_app_bind_vnd(args->mod_app_bind_vnd.net_idx,
                                        args->mod_app_bind_vnd.addr,
                                        args->mod_app_bind_vnd.elem_addr,
                                        args->mod_app_bind_vnd.mod_app_idx,
                                        args->mod_app_bind_vnd.mod_id,
                                        args->mod_app_bind_vnd.cid,
                                        &(args->mod_app_bind_vnd.status));
                        break;

                case BTMESH_OP_MOD_APP_UNBIND:
                        err = bt_mesh_cfg_mod_app_unbind(
                                        args->mod_app_unbind.net_idx,
                                        args->mod_app_unbind.addr,
                                        args->mod_app_unbind.elem_addr,
                                        args->mod_app_unbind.mod_app_idx,
                                        args->mod_app_unbind.mod_id,
                                        &(args->mod_app_unbind.status));
                        break;

                case BTMESH_OP_MOD_APP_UNBIND_VND:
                        err = bt_mesh_cfg_mod_app_unbind_vnd(
                                        args->mod_app_unbind_vnd.net_idx,
                                        args->mod_app_unbind_vnd.addr,
                                        args->mod_app_unbind_vnd.elem_addr,
                                        args->mod_app_unbind_vnd.mod_app_idx,
                                        args->mod_app_unbind_vnd.mod_id,
                                        args->mod_app_unbind_vnd.cid,
                                        &(args->mod_app_unbind_vnd.status));
                        break;

                case BTMESH_OP_MOD_APP_GET:
                        err = bt_mesh_cfg_mod_app_get(
					args->mod_app_get.net_idx,
                                        args->mod_app_get.addr,
                                        args->mod_app_get.elem_addr,
                                        args->mod_app_get.mod_id,
                                        &(args->mod_app_get.status),
                                        args->mod_app_get.apps,
                                        &(args->mod_app_get.app_cnt));
                        break;

		case BTMESH_OP_MOD_APP_GET_VND:
                        err = bt_mesh_cfg_mod_app_get_vnd(
                                        args->mod_app_get_vnd.net_idx,
                                        args->mod_app_get_vnd.addr,
                                        args->mod_app_get_vnd.elem_addr,
                                        args->mod_app_get_vnd.mod_id,
                                        args->mod_app_get_vnd.cid,
                                        &(args->mod_app_get_vnd.status),
                                        args->mod_app_get_vnd.apps,
                                        &(args->mod_app_get_vnd.app_cnt));
                        break;

                case BTMESH_OP_MOD_PUB_GET:
                        err = bt_mesh_cfg_mod_pub_get(args->mod_pub_get.net_idx,
                                        args->mod_pub_get.addr, args->mod_pub_get.elem_addr,
                                        args->mod_pub_get.mod_id, &(args->mod_pub_get.pub),
                                        &(args->mod_pub_get.status));
                        break;

                case BTMESH_OP_MOD_PUB_GET_VND:
                        err = bt_mesh_cfg_mod_pub_get_vnd(
                                        args->mod_pub_get_vnd.net_idx,
                                        args->mod_pub_get_vnd.addr,
                                        args->mod_pub_get_vnd.elem_addr,
                                        args->mod_pub_get_vnd.mod_id,
                                        args->mod_pub_get_vnd.cid,
                                        &(args->mod_pub_get_vnd.pub),
                                        &(args->mod_pub_get_vnd.status));
                        break;

                case BTMESH_OP_MOD_PUB_SET:
                        err = bt_mesh_cfg_mod_pub_set(args->mod_pub_set.net_idx,
                                        args->mod_pub_set.addr,
                                        args->mod_pub_set.elem_addr,
                                        args->mod_pub_set.mod_id,
                                        &(args->mod_pub_set.pub),
                                        &(args->mod_pub_set.status));
                        break;

                case BTMESH_OP_MOD_PUB_SET_VND:
                        err = bt_mesh_cfg_mod_pub_set_vnd(args->mod_pub_set_vnd.net_idx,
                                        args->mod_pub_set_vnd.addr,
                                        args->mod_pub_set_vnd.elem_addr,
                                        args->mod_pub_set_vnd.mod_id,
                                        args->mod_pub_set_vnd.cid,
                                        &(args->mod_pub_set_vnd.pub),
                                        &(args->mod_pub_set_vnd.status));
                        break;

                case BTMESH_OP_MOD_SUB_ADD:
                        err = bt_mesh_cfg_mod_sub_add(args->mod_sub_add.net_idx,
                                        args->mod_sub_add.addr,
                                        args->mod_sub_add.elem_addr,
                                        args->mod_sub_add.sub_addr,
                                        args->mod_sub_add.mod_id,
                                        &(args->mod_sub_add.status));
                        break;
                
                case BTMESH_OP_MOD_SUB_ADD_VND:
                        err = bt_mesh_cfg_mod_sub_add_vnd(args->mod_sub_add_vnd.net_idx,
                                        args->mod_sub_add_vnd.addr,
                                        args->mod_sub_add_vnd.elem_addr,
                                        args->mod_sub_add_vnd.sub_addr,
                                        args->mod_sub_add_vnd.mod_id,
                                        args->mod_sub_add_vnd.cid,
                                        &(args->mod_sub_add_vnd.status));
                        break;

                case BTMESH_OP_MOD_SUB_DEL:
                        err = bt_mesh_cfg_mod_sub_del(args->mod_sub_del.net_idx,
                                        args->mod_sub_del.addr,
                                        args->mod_sub_del.elem_addr,
                                        args->mod_sub_del.sub_addr,
                                        args->mod_sub_del.mod_id,
                                        &(args->mod_sub_del.status));
                        break;
                
                case BTMESH_OP_MOD_SUB_DEL_VND:
                        err = bt_mesh_cfg_mod_sub_del_vnd(args->mod_sub_del_vnd.net_idx,
                                        args->mod_sub_del_vnd.addr,
                                        args->mod_sub_del_vnd.elem_addr,
                                        args->mod_sub_del_vnd.sub_addr,
                                        args->mod_sub_del_vnd.mod_id,
                                        args->mod_sub_del_vnd.cid,
                                        &(args->mod_sub_del_vnd.status));
                        break;

                case BTMESH_OP_MOD_SUB_OVRW:
                        err = bt_mesh_cfg_mod_sub_overwrite(
                                        args->mod_sub_ovrw.net_idx,
                                        args->mod_sub_ovrw.addr,
                                        args->mod_sub_ovrw.elem_addr,
                                        args->mod_sub_ovrw.sub_addr,
                                        args->mod_sub_ovrw.mod_id,
                                        &(args->mod_sub_ovrw.status));
                        break;
                
                case BTMESH_OP_MOD_SUB_OVRW_VND:
                        err = bt_mesh_cfg_mod_sub_overwrite_vnd(
                                        args->mod_sub_ovrw_vnd.net_idx,
                                        args->mod_sub_ovrw_vnd.addr,
                                        args->mod_sub_ovrw_vnd.elem_addr,
                                        args->mod_sub_ovrw_vnd.sub_addr,
                                        args->mod_sub_ovrw_vnd.mod_id,
                                        args->mod_sub_ovrw_vnd.cid,
                                        &(args->mod_sub_ovrw_vnd.status));
                        break;

                case BTMESH_OP_MOD_SUB_GET:
                        err = bt_mesh_cfg_mod_sub_get(args->mod_sub_get.net_idx,
                                        args->mod_sub_get.addr,
                                        args->mod_sub_get.elem_addr,
                                        args->mod_sub_get.mod_id,
                                        &(args->mod_sub_get.status),
                                        args->mod_sub_get.subs,
                                        &(args->mod_sub_get.sub_cnt));
                        break;

                case BTMESH_OP_MOD_SUB_GET_VND:
                        err = bt_mesh_cfg_mod_sub_get_vnd(
                                        args->mod_sub_get_vnd.net_idx,
                                        args->mod_sub_get_vnd.addr,
                                        args->mod_sub_get_vnd.elem_addr,
                                        args->mod_sub_get_vnd.mod_id,
                                        args->mod_sub_get_vnd.cid,
                                        &(args->mod_sub_get_vnd.status),
                                        args->mod_sub_get_vnd.subs,
                                        &(args->mod_sub_get_vnd.sub_cnt));
                        break;

                case BTMESH_OP_HLTH_FAULT_GET:
                        err = bt_mesh_health_fault_get(
                                        args->hlth_fault_get.addr,
                                        args->hlth_fault_get.app_idx,
                                        args->hlth_fault_get.cid,
                                        &(args->hlth_fault_get.test_id),
                                        args->hlth_fault_get.faults,
                                        &(args->hlth_fault_get.fault_count));
                        break;

                case BTMESH_OP_HLTH_FAULT_CLR:
                        err = bt_mesh_health_fault_clear(
                                        args->hlth_fault_clear.addr,
                                        args->hlth_fault_clear.app_idx,
                                        args->hlth_fault_clear.cid,
                                        &(args->hlth_fault_clear.test_id),
                                        args->hlth_fault_clear.faults,
                                        &(args->hlth_fault_clear.fault_count));
                        break;

                case BTMESH_OP_HLTH_FAULT_TEST:
                        err = bt_mesh_health_fault_test(
                                        args->hlth_fault_test.addr,
                                        args->hlth_fault_test.app_idx,
                                        args->hlth_fault_test.cid,
                                        args->hlth_fault_test.test_id,
                                        args->hlth_fault_test.faults,
                                        &(args->hlth_fault_test.fault_count));
                        break;

                case BTMESH_OP_HLTH_PERIOD_GET:
                        err = bt_mesh_health_period_get(
                                        args->hlth_period_get.addr,
                                        args->hlth_period_get.app_idx,
                                        &(args->hlth_period_get.divisor));
                        break;

                case BTMESH_OP_HLTH_PERIOD_SET:
                        err = bt_mesh_health_period_set(
                                        args->hlth_period_set.addr,
                                        args->hlth_period_set.app_idx,
                                        args->hlth_period_set.divisor,
                                        &(args->hlth_period_set.updated_divisor));
                        break;

                case BTMESH_OP_HLTH_ATTN_GET:
                        err = bt_mesh_health_attention_get(
                                        args->hlth_attn_get.addr,
                                        args->hlth_attn_get.app_idx,
                                        &(args->hlth_attn_get.attn));
                        break;

                case BTMESH_OP_HLTH_ATTN_SET:
                        err = bt_mesh_health_attention_set(
                                        args->hlth_attn_set.addr,
                                        args->hlth_attn_set.app_idx,
                                        args->hlth_attn_set.attn,
                                        &(args->hlth_attn_set.updated_attn));
                        break;

                case BTMESH_OP_HLTH_TIMEOUT_GET:
                        args->hlth_timeout_get.timeout = bt_mesh_health_cli_timeout_get();
                        err = 0;
                        break;

                case BTMESH_OP_HLTH_TIMEOUT_SET:
                        bt_mesh_health_cli_timeout_set(args->hlth_timeout_set.timeout);
                        err = 0;
                        break;

		case BTMESH_OP_HB_SUB_GET:
			err = bt_mesh_cfg_hb_sub_get(
					args->hb_sub_get.net_idx,
					args->hb_sub_get.addr,
					&(args->hb_sub_get.sub),
					&(args->hb_sub_get.status));
			break;

		case BTMESH_OP_HB_SUB_SET:
			err = bt_mesh_cfg_hb_sub_set(
					args->hb_sub_set.net_idx,
					args->hb_sub_set.addr,
					&(args->hb_sub_set.sub),
					&(args->hb_sub_set.status));
			break;

		case BTMESH_OP_HB_PUB_GET:
			err = bt_mesh_cfg_hb_pub_get(
					args->hb_pub_get.net_idx,
					args->hb_pub_get.addr,
					&(args->hb_pub_get.pub),
					&(args->hb_pub_get.status));
			break;

		case BTMESH_OP_HB_PUB_SET:
			err = bt_mesh_cfg_hb_pub_set(
					args->hb_pub_set.net_idx,
					args->hb_pub_set.addr,
					&(args->hb_pub_set.pub),
					&(args->hb_pub_set.status));
			break;

                default:
                        err = -EINVAL;
                        break;
                }

                if (!err) {
                        return 0;
                }
        }

        return err;
}

int btmesh_clean_node_key(uint16_t addr, uint16_t app_idx)
{
        int i;
        int j;
        int k;
        int err;
        uint8_t num_elem;
        uint8_t num_sig_models;
        uint8_t num_vnd_models;
        union btmesh_op_args args;
        struct net_buf_simple *comp_data;

        args.comp_get.net_idx = PRIMARY_SUBNET;
        args.comp_get.addr = addr;
        args.comp_get.page = 0x00;
        args.comp_get.comp = NET_BUF_SIMPLE(COMP_DATA_BUF_SIZE);
        net_buf_simple_init(args.comp_get.comp, 0);
        err = btmesh_perform_op(BTMESH_OP_COMP_GET, &args);

        if (err) {
                LOG_ERR("Failed to get compositin data. Error: %d", err);
                return err;
        }

        /* Parse through comp data until element info */
        comp_data = args.comp_get.comp;
        net_buf_simple_pull_mem(comp_data, 10);
        num_elem = bt_mesh_cdb_node_get(addr)->num_elem;

        for (i = 0; i < num_elem; i++) {
                /* Parse through LOC field */
                net_buf_simple_pull_le16(comp_data);

                /* Get number of models */
                num_sig_models = net_buf_simple_pull_u8(comp_data);
                num_vnd_models = net_buf_simple_pull_u8(comp_data);
                args.mod_app_get.elem_addr = addr + i;

                for (j = 0; j < num_sig_models; j++) {
                        args.mod_app_get.mod_id = net_buf_simple_pull_le16(comp_data);
                        args.mod_app_get.app_cnt = sizeof(args.mod_app_get.apps);
                        err = btmesh_perform_op(BTMESH_OP_MOD_APP_GET, &args);

                        if (err) {
                                LOG_ERR("Failed to get model app keys. Error: %d", err);
                                return err;
                        }

                        for (k = 0; k < args.mod_app_get.app_cnt; k++) {
                                if (args.mod_app_get.apps[k] == app_idx) {
                                        /* At least one model on this node is still using this app key so we must NOT
                                         * delete it */
                                        break;
                                }
                        }

                        if (k != args.mod_app_get.app_cnt) {
                                break;
                        }
                        /* If we make it here, this model is NOT using this app key */
                }

                if (j != num_sig_models) {
                        break;
                }
                /* If we make it here, NO SIG models on this element are using this app key */

                for (j = 0; j < num_vnd_models; j++) {
                        args.mod_app_get_vnd.cid = net_buf_simple_pull_le16(comp_data);
                        args.mod_app_get_vnd.mod_id = net_buf_simple_pull_le16(comp_data);
                        args.mod_app_get_vnd.app_cnt = sizeof(args.mod_app_get_vnd.apps);
                        err = btmesh_perform_op(BTMESH_OP_MOD_APP_GET_VND, &args);

                        if (err) {
                                LOG_ERR("Failed to get vendor model app keys. Error: %d", err);
                                return err;
                        }

                        for (k = 0; k < args.mod_app_get_vnd.app_cnt; k++) {
                                if (args.mod_app_get_vnd.apps[k] == app_idx) {
                                        /* At least one model on this node is still using this app key so we must NOT
                                         * delete it */
                                        break;
                                }
                        }

                        if (k != args.mod_app_get_vnd.app_cnt) {
                                break;
                        }
                        /* If we make it here, this model is NOT using this app key */
                }

                if (j != num_vnd_models) {
                        break;
                }
                /* If we make it here, NO vendor models on this element are using this app key */
        }

        if (i != num_elem) {
                /* Some model on some element is still using this app key so no need to delete it */
                return 0;
        }
        /* If we make it here, NO element has amodel that is using this app key so we can go ahead and
         * delete it */

        args.app_key_del.key_net_idx = bt_mesh_cdb_app_key_get(app_idx)->net_idx;
        args.app_key_del.key_app_idx = app_idx;
        err = btmesh_perform_op(BTMESH_OP_APP_KEY_DEL, &args);

        if (err) {
                LOG_ERR("Failed to delete unused app key from node. Error: %d", err);
                return err;
        }

        return 0;
}

int btmesh_discover_node(struct btmesh_node *node, uint8_t *status)
{
        int err;
        uint8_t i;
        uint8_t j;
        uint16_t features;
        union btmesh_op_args args;
        struct bt_mesh_cdb_node *cdb_node;
        struct net_buf_simple *comp_data = NET_BUF_SIMPLE(COMP_DATA_BUF_SIZE);

        *status = 0;

        /* Initialize node structure pointers */
        node->subnet_idxs = NULL;
        node->elems = NULL;

        /* Get the node's UUID, net index, and element count from CDB */
        cdb_node = bt_mesh_cdb_node_get(node->addr);

        if (cdb_node == NULL) {
                return -ESRCH;
        }

        memcpy(node->uuid, cdb_node->uuid, UUID_LEN);
        node->net_idx = cdb_node->net_idx;
        node->elem_count = cdb_node->num_elem;

        /* Get the node's composition data */
        net_buf_simple_init(comp_data, 0);
        args.comp_get.net_idx = PRIMARY_SUBNET;
        args.comp_get.addr = node->addr;
        args.comp_get.page = COMP_DATA_PAGE;
        args.comp_get.comp = comp_data;
        err = btmesh_perform_op(BTMESH_OP_COMP_GET, &args);

        if (err || args.comp_get.status) {
                *status = args.comp_get.status;
                return err;
        }

        /* Get all IDs from composition data */
        node->cid = net_buf_simple_pull_le16(comp_data);
        node->pid = net_buf_simple_pull_le16(comp_data);
        node->vid = net_buf_simple_pull_le16(comp_data);
        node->crpl = net_buf_simple_pull_le16(comp_data);

        /* Get node feature support from composition data */
        features = net_buf_simple_pull_le16(comp_data);
        node->relay.support = (bool)(features & 0x0001);
        node->proxy.support = (bool)(features & 0x0002);
        node->friend.support = (bool)(features & 0x0004);
        node->lpn = (bool)(features & 0x0008);

        /* Get node's network beacon state */
        err = btmesh_perform_op(BTMESH_OP_BEACON_GET, &args);

        if (err) {
                return err;
        }

        if (args.beacon_get.status == BT_MESH_BEACON_DISABLED) {
                node->net_beacon_state = false;
        } else if (args.beacon_get.status == BT_MESH_BEACON_ENABLED) {
                node->net_beacon_state = true;
        } else {
                return err;
        }

        /* Get node's ttl value */
        err = btmesh_perform_op(BTMESH_OP_TTL_GET, &args);

        if (err) {
                return err;
        }

        node->ttl = args.ttl_get.ttl;

	/* Get node's heartbeat subscribe parameters */
	err = btmesh_perform_op(BTMESH_OP_HB_SUB_GET, &args);

	if (err || args.hb_sub_get.status) {
		*status = args.hb_sub_get.status;
		return err;
	}

	node->hb_sub = args.hb_sub_get.sub;

	/* Get node's heartbeat publish parameters */
	err = btmesh_perform_op(BTMESH_OP_HB_PUB_GET, &args);

	if (err || args.hb_pub_get.status) {
		*status = args.hb_pub_get.status;
		return err;
	}

	node->hb_pub = args.hb_pub_get.pub;

        /* Get node's relay feature state */
        err = btmesh_perform_op(BTMESH_OP_RELAY_GET, &args);

        if (err) {
                return err;
        }

        if (args.relay_get.status == BT_MESH_RELAY_DISABLED ||
                        args.relay_get.status == BT_MESH_RELAY_NOT_SUPPORTED) {
                node->relay.state = false;
        } else if (args.relay_get.status == BT_MESH_RELAY_ENABLED) {
                node->relay.state = true;
        } else {
                return err;
        }

        node->relay.count = BT_MESH_TRANSMIT_COUNT(args.relay_get.transmit);
        node->relay.interval = BT_MESH_TRANSMIT_INT(args.relay_get.transmit);

        /* Get node's proxy feature state */
        err = btmesh_perform_op(BTMESH_OP_PROXY_GET, &args);

        if (err) {
                return err;
        }

        if (args.proxy_get.status == BT_MESH_GATT_PROXY_DISABLED ||
                        args.proxy_get.status == BT_MESH_GATT_PROXY_NOT_SUPPORTED) {
                node->proxy.state = false;
        } else if (args.proxy_get.status == BT_MESH_GATT_PROXY_ENABLED) {
                node->proxy.state = true;
        } else {
                return err;
        }

        /* Get node's friend feature state */
        err = btmesh_perform_op(BTMESH_OP_FRIEND_GET, &args);

        if (err) {
                return err;
        }

        if (args.friend_get.status == BT_MESH_FRIEND_DISABLED ||
                        args.friend_get.status == BT_MESH_FRIEND_NOT_SUPPORTED) {
                node->friend.state = false;
        } else if (args.friend_get.status == BT_MESH_FRIEND_ENABLED) {
                node->friend.state = true;
        } else {
                return err;
        }

        /* Get node's subnets */
        args.net_key_get.key_cnt = sizeof(args.net_key_get.keys);
        err = btmesh_perform_op(BTMESH_OP_NET_KEY_GET, &args);

        if (err) {
                return err;
        };

        node->subnet_count = args.net_key_get.key_cnt;
        node->subnet_idxs = (uint16_t *)k_malloc(sizeof(uint16_t) *
                        node->subnet_count);

        if (node->subnet_idxs == NULL) {
                return -ENOMEM;
        }

        memcpy(node->subnet_idxs, args.net_key_get.keys,
                        sizeof(uint16_t) * node->subnet_count);

        /* Get the node's elements */
        node->elems = (struct btmesh_elem *)k_malloc(sizeof(struct btmesh_elem)
                        * node->elem_count);

        if (node->elems == NULL) {
                return -ENOMEM;
        }

        /* Initialize all pointers so they can be free'd later even if unused */
        for (i = 0; i < node->elem_count; i++) {
                node->elems[i].sig_models = NULL;
                node->elems[i].vnd_models = NULL;
        }

        for (i = 0; i < node->elem_count; i++)
        {
                node->elems[i].addr = node->addr + i;
                node->elems[i].loc = net_buf_simple_pull_le16(comp_data);
                node->elems[i].sig_model_count = net_buf_simple_pull_u8(comp_data);
                node->elems[i].vnd_model_count = net_buf_simple_pull_u8(comp_data);

                node->elems[i].sig_models =
                        (struct btmesh_sig_model *)k_malloc(sizeof(struct btmesh_sig_model)
                                        * node->elems[i].sig_model_count);

                if (node->elems[i].sig_models == NULL) {
                        btmesh_free_node(node);
                        return -ENOMEM;
                }

                /* Initialize all pointers so they can be free'd later even if unused */
                for (j = 0; j < node->elems[i].sig_model_count; j++) {
                        node->elems[i].sig_models[j].appkey_idxs = NULL;
                        node->elems[i].sig_models[j].sub_addrs = NULL;
                }

                node->elems[i].vnd_models =
                        (struct btmesh_vnd_model *)k_malloc(sizeof(struct btmesh_vnd_model)
                                        * node->elems[i].vnd_model_count);

                if (node->elems[i].vnd_models == NULL) {
                        btmesh_free_node(node);
                        return -ENOMEM;
                }

                /* Initialize all pointers so they can be free'd later even if unused */
                for (j = 0; j < node->elems[i].vnd_model_count; j++) {
                        node->elems[i].vnd_models[j].appkey_idxs = NULL;
                        node->elems[i].vnd_models[j].sub_addrs = NULL;
                }

                /* Get all SIG models on this element */
                for (j = 0; j < node->elems[i].sig_model_count; j++) {
                        node->elems[i].sig_models[j].model_id =
                                net_buf_simple_pull_le16(comp_data);

                        /* Get SIG model's appkey indexes */
                        args.mod_app_get.elem_addr = node->addr + i;
                        args.mod_app_get.mod_id = node->elems[i].sig_models[j].model_id;
                        args.mod_app_get.app_cnt = sizeof(args.mod_app_get.apps);
                        err = btmesh_perform_op(BTMESH_OP_MOD_APP_GET, &args);

                        if (err || args.mod_app_get.status) {
                                *status = args.mod_app_get.status;
                                btmesh_free_node(node);
                                return err;
                        }

                        node->elems[i].sig_models[j].appkey_count =
                                args.mod_app_get.app_cnt;
                        node->elems[i].sig_models[j].appkey_idxs =
                                (uint16_t *)k_malloc(sizeof(uint16_t) *
                                                node->elems[i].sig_models[j].appkey_count);

                        if (node->elems[i].sig_models[j].appkey_idxs == NULL) {
                                btmesh_free_node(node);
                                return -ENOMEM;
                        }

                        memcpy(node->elems[i].sig_models[j].appkey_idxs,
                                        args.mod_app_get.apps, sizeof(uint16_t) *
                                        node->elems[i].sig_models[j].appkey_count);

                        /* Get SIG model's subscribe addresses */
                        args.mod_sub_get.sub_cnt = sizeof(args.mod_sub_get.subs);
                        err = btmesh_perform_op(BTMESH_OP_MOD_SUB_GET, &args);

                        if (err || args.mod_sub_get.status) {
                                *status = args.mod_sub_get.status;
                                btmesh_free_node(node);
                                return err;
                        }

                        node->elems[i].sig_models[j].sub_addr_count =
                                args.mod_sub_get.sub_cnt;
                        node->elems[i].sig_models[j].sub_addrs =
                                (uint16_t *)k_malloc(sizeof(uint16_t) * 
                                                node->elems[i].sig_models[j].sub_addr_count);

                        if (node->elems[i].sig_models[j].sub_addrs == NULL) {
                                btmesh_free_node(node);
                                return -ENOMEM;
                        }

                        memcpy(node->elems[i].sig_models[j].sub_addrs,
                                        args.mod_sub_get.subs, sizeof(uint16_t) *
                                        node->elems[i].sig_models[j].sub_addr_count);

                        /* Get SIG model's publish parameters */
                        /* SIG model ID 0x0000 is configuration server model and has no
                         * user definable publish parameters */
                        if (node->elems[i].sig_models[j].model_id == 0x0000) {
                                continue;
                        }

                        err = btmesh_perform_op(BTMESH_OP_MOD_PUB_GET, &args);

                        if (err || args.mod_pub_get.status) {
                                *status = args.mod_pub_get.status;
                                btmesh_free_node(node);
                                return err;
                        }

                        memcpy(&node->elems[i].sig_models[j].pub, &args.mod_pub_get.pub,
                                        sizeof(struct bt_mesh_cfg_mod_pub));
                }

                /* Get all vendor models on this element */
                for (j = 0; j < node->elems[i].vnd_model_count; j++) {
                        node->elems[i].vnd_models[j].company_id =
                                net_buf_simple_pull_le16(comp_data);
                        node->elems[i].vnd_models[j].model_id =
                                net_buf_simple_pull_le16(comp_data);

                        /* Get vendor model's appkey indexes */
                        args.mod_app_get_vnd.mod_id = node->elems[i].vnd_models[j].model_id;
                        args.mod_app_get_vnd.cid = node->elems[i].vnd_models[j].company_id;
                        args.mod_app_get_vnd.app_cnt = sizeof(args.mod_app_get_vnd.apps);
                        err = btmesh_perform_op(BTMESH_OP_MOD_APP_GET_VND, &args);

                        if (err || args.mod_app_get_vnd.status) {
                                *status = args.mod_app_get_vnd.status;
                                btmesh_free_node(node);
                                return err;
                        }

                        node->elems[i].vnd_models[j].appkey_count =
                                args.mod_app_get_vnd.app_cnt;
                        node->elems[i].vnd_models[j].appkey_idxs =
                                (uint16_t *)k_malloc(sizeof(uint16_t) *
                                                node->elems[i].vnd_models[j].appkey_count);

                        if (node->elems[i].vnd_models[j].appkey_idxs == NULL) {
                                btmesh_free_node(node);
                                return -ENOMEM;
                        }

                        memcpy(node->elems[i].vnd_models[j].appkey_idxs,
                                        args.mod_app_get_vnd.apps, sizeof(uint16_t) *
                                        node->elems[i].vnd_models[j].appkey_count);

                        /* Get vendor model's subscribe addresses */
                        args.mod_sub_get_vnd.sub_cnt = sizeof(args.mod_sub_get_vnd.subs);
			args.mod_sub_get_vnd.cid = node->elems[i].vnd_models[j].company_id;
                        err = btmesh_perform_op(BTMESH_OP_MOD_SUB_GET_VND, &args);

                        if (err || args.mod_sub_get_vnd.status) {
                                *status = args.mod_sub_get_vnd.status;
                                btmesh_free_node(node);
                                return err;
                        }

                        node->elems[i].vnd_models[j].sub_addr_count =
                                args.mod_sub_get_vnd.sub_cnt;
                        node->elems[i].vnd_models[j].sub_addrs =
                                (uint16_t *)k_malloc(sizeof(uint16_t) *
                                                node->elems[i].vnd_models[j].sub_addr_count);

                        if (node->elems[i].vnd_models[j].sub_addrs == NULL) {
                                btmesh_free_node(node);
                                return -ENOMEM;
                        }

                        memcpy(node->elems[i].vnd_models[j].sub_addrs,
                                        args.mod_sub_get_vnd.subs, sizeof(uint16_t) *
                                        node->elems[i].vnd_models[j].sub_addr_count);

                        /* Get vendor model publish parameters */
			args.mod_pub_get_vnd.cid = node->elems[i].vnd_models[j].company_id;
                        err = btmesh_perform_op(BTMESH_OP_MOD_PUB_GET_VND, &args);

                        if (err || args.mod_pub_get_vnd.status) {
                                *status = args.mod_pub_get_vnd.status;
                                btmesh_free_node(node);
                                return err;
                        }

                        memcpy(&node->elems[i].vnd_models[j].pub, &args.mod_pub_get_vnd.pub,
                                        sizeof(struct bt_mesh_cfg_mod_pub));
                }
        } 

        return err;
}

int btmesh_init(void)
{
        int err;
        uint16_t i;
        uint8_t net_key[16];
        uint8_t dev_key[16];
        struct bt_mesh_cdb_node *self;

        /* Initialize beacon timer structures */
        for (i = 0; i < MAX_BEACON; i++) {
                k_timer_init(&beacon_timer_list[i], beacon_timer_handler, NULL);
                k_timer_user_data_set(&beacon_timer_list[i], NULL);
        }

        err = bt_enable(NULL);

        if (err)
        {
                LOG_ERR("Failed to enable Bluetooth. Error: %d", err);
                return err;
        }

        LOG_INF("Bluetooth initialized");

        /* Initialize Bluetooth Mesh */
        err = bt_mesh_init(&prov, &mesh_comp);

        if (err) {
                LOG_ERR("Failed to initialize Bluetooth Mesh. Error: %d", err);
                return err;
        }

        LOG_INF("Bluetooth Mesh initialized");

        /* Load stored Bluetooth settings */
        if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
                LOG_INF("Loading stored Bluetooth settings...");
                settings_load();
        }

        /* Use stored CDB if it exists, otherwise create new CDB */
        bt_rand(net_key, 16);
        err = bt_mesh_cdb_create(net_key);

        if (err == -EALREADY) {
                LOG_INF("Using stored CDB");
        } else if (err) {
                LOG_ERR("Failed to create CDB. Error: %d", err);
                return err;
        } else {
                LOG_INF("Created new CDB");
        }

        /* Provision self if necessary */
        bt_rand(dev_key, 16);

        if (bt_mesh_is_provisioned()) {
                LOG_INF("Self already provisioned");
        } else {
                LOG_INF("Provisioning self");
                err = bt_mesh_provision(net_key, BT_MESH_NET_PRIMARY, 0, 0, self_addr, dev_key);

                if (err) {
                        LOG_ERR("Failed to provision self. Error: %d", err);
                        return err;
                }
        }

        /* Mark self as configured if needed */ 
        self = bt_mesh_cdb_node_get(self_addr);

        if (!atomic_test_bit(self->flags, BT_MESH_CDB_NODE_CONFIGURED)) {
                atomic_set_bit(self->flags, BT_MESH_CDB_NODE_CONFIGURED);

                if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
                        bt_mesh_cdb_node_store(self);
                }

                LOG_INF("Configured self"); 
        }

#if defined(CONFIG_BT_MESH_ACCESS_LAYER_MSG)
        bt_mesh_msg_cb_set(btmesh_msg_cb);
#endif // defined(CONFIG_BT_MESH_ACCESS_LAYER_MSG)

        return 0;
}
