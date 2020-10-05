#include <zephyr.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>
#include <cJSON_os.h>
#include <logging/log.h>
#include <random/rand32.h>
#undef __XSI_VISIBLE
#define __XSI_VISIBLE 1
#include <date_time.h>
#include <time.h>
#include <posix/time.h>

#include "codec.h"
#include "btmesh.h"
#include "gw_cloud.h"
#include "util.h"


LOG_MODULE_REGISTER(app_codec, CONFIG_NRF_CLOUD_MESH_GATEWAY_LOG_LEVEL);

const char JSON_STR_TYPE[] = "type";
const char JSON_STR_EVENT[] = "event";
const char JSON_STR_DEVICE_TYPE[] = "deviceType";
const char JSON_STR_OOB_INFO[] = "oobInfo";
const char JSON_STR_URI_HASH[] = "uriHash";
const char JSON_STR_NA[] = "N/A";
const char JSON_STR_BT_MESH[] = "BT-Mesh";
const char JSON_STR_MSG_ID[] = "messageId";
const char JSON_STR_ERR[] = "error";
const char JSON_STR_SUPPORT[] = "support";
const char JSON_STR_UUID[] = "uuid";
const char JSON_STR_NET_IDX[] = "netIndex";
const char JSON_STR_APP_IDX[] = "appIndex";
const char JSON_STR_APP_IDXS[] = "appIndexes";
const char JSON_STR_PUB_PARAMS[] = "publishParameters";
const char JSON_STR_ADDR[] = "address";
const char JSON_STR_ADDR_LIST[] = "addressList";
const char JSON_STR_ATTN[] = "attention";
const char JSON_STR_NET_KEY[] = "netKey";
const char JSON_STR_APP_KEY[] = "appKey";
const char JSON_STR_TEST_ID[] = "testId";
const char JSON_STR_DIV[] = "divisor";
const char JSON_STR_TIMEOUT[] = "timeout";
const char JSON_STR_STATE[] = "state";
const char JSON_STR_TTL[] = "timeToLive";
const char JSON_STR_TX_COUNT[] = "retransmitCount";
const char JSON_STR_TX_INT[] = "retransmitInterval";
const char JSON_STR_MOD_ID[] = "modelId";
const char JSON_STR_ELEM_ADDR[] = "elementAddress";
const char JSON_STR_ELEM_COUNT[] = "elementCount";
const char JSON_STR_PUB_ADDR[] = "publishAddress";
const char JSON_STR_SUB_ADDR[] = "subscribeAddress";
const char JSON_STR_SUB_ADDRS[] = "subscribeAddresses";
const char JSON_STR_SRC_ADDR[] = "sourceAddress";
const char JSON_STR_DST_ADDR[] = "destinationAddress";
const char JSON_STR_CID[] = "companyId";
const char JSON_STR_FRIEND_CRED_FLAG[] = "friendCredentialFlag";
const char JSON_STR_PERIOD[] = "period";
const char JSON_STR_PERIOD_UNITS[] = "periodUnits";
const char JSON_STR_COUNT[] = "count";
const char JSON_STR_MIN_HOPS[] = "minimumHops";
const char JSON_STR_MAX_HOPS[] = "maximumHops";
const char JSON_STR_RELAY[] = "relayFeature";
const char JSON_STR_PROXY[] = "proxyFeature";
const char JSON_STR_FRIEND[] = "friendFeature";
const char JSON_STR_LPN[] = "lpnFeature";
const char JSON_STR_PAYLOAD[] = "payload";
const char JSON_STR_OPCODE[] = "opcode";
const char JSON_STR_BYTE[] = "byte";


static int message_id;


static char *get_time_str(char *dst, size_t len)
{
        struct timespec ts;
        struct tm tm;
        time_t t;

        if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
                t = (time_t)ts.tv_sec;
                tm = *(gmtime(&t));
                strftime(dst, len, "%Y-%m-%dT%H:%M:%S.000Z", &tm);
        }

        return dst;
}

static bool codec_init_event(cJSON **root_obj, cJSON **event_obj, const char *event)
{
        char time_str[64];

        *root_obj = cJSON_CreateObject();

        if (*root_obj == NULL) {
                return false;
        }

        if (cJSON_AddStringToObject(*root_obj, JSON_STR_TYPE, JSON_STR_EVENT) == NULL) {
                goto error;
        }

        if (cJSON_AddStringToObject(*root_obj, "gatewayId", gw_cloud_get_id()) == NULL) {
                goto error;
        }

        *event_obj = cJSON_CreateObject();

        if (*event_obj == NULL) {
                goto error;
        }

        cJSON_AddItemToObject(*root_obj, JSON_STR_EVENT, *event_obj);

        if (cJSON_AddStringToObject(*event_obj, JSON_STR_TYPE, event) == NULL) {
                goto error;
        }

        if (cJSON_AddStringToObject(*event_obj, "timestamp",
                                get_time_str(time_str, sizeof(time_str))) == NULL) {
                goto error;
        }

        return true;

error:
        cJSON_Delete(*root_obj);
        return false;
}

static bool codec_get_uint8(cJSON *obj, const char *item, uint8_t *uint8)
{
        cJSON *uint8_obj;

        uint8_obj = cJSON_GetObjectItem(obj, item);

        if (uint8_obj == NULL) {
                return false;
        }

        if (!cJSON_IsNumber(uint8_obj)) {
                return false;
        }

        if (uint8_obj->valuedouble > UINT8_MAX) {
                return false;
        }

        *uint8 = (uint8_t)uint8_obj->valuedouble;
        return true;
}

static bool codec_get_uint16(cJSON *obj, const char *item, uint16_t *uint16)
{
        cJSON *uint16_obj;

        uint16_obj = cJSON_GetObjectItem(obj, item);

        if (uint16_obj == NULL) {
                return false;
        }

        if (!cJSON_IsNumber(uint16_obj)) {
                return false;
        }

        if (uint16_obj->valuedouble > UINT16_MAX) {
                return false;
        }

        *uint16 = (uint16_t)uint16_obj->valuedouble;
        return true;
}

static bool codec_get_uint32(cJSON *obj, const char *item, uint32_t *uint32)
{
        cJSON *uint32_obj;

        uint32_obj = cJSON_GetObjectItem(obj, item);

        if (uint32_obj == NULL) {
                return false;
        }

        if (!cJSON_IsNumber(uint32_obj)) {
                return false;
        }

        if (uint32_obj->valuedouble > UINT32_MAX) {
                return false;
        }

        *uint32 = (uint32_t)uint32_obj->valuedouble;
        return true;
}

static bool codec_get_int32(cJSON *obj, const char *item, int32_t *int32)
{
	cJSON *int32_obj;

	int32_obj = cJSON_GetObjectItem(obj, item);

	if (int32_obj == NULL) {
		return false;
	}

	if (!cJSON_IsNumber(int32_obj)) {
		return false;
	}

	if (int32_obj->valuedouble > INT32_MAX) {
		return false;
	}

	*int32 = (int32_t)int32_obj->valuedouble;
	return true;
}

static bool codec_get_bool(cJSON *obj, const char *item, bool *boolean)
{
        cJSON *boolean_obj;

        boolean_obj = cJSON_GetObjectItem(obj, item);

        if (boolean_obj == NULL) {
                return false;
        }

        if (cJSON_IsTrue(boolean_obj)) {
                *boolean = true;
        } else {
                *boolean = false;
        }

        return true;
}

static bool codec_get_str(cJSON *obj, const char *item, char **str)
{
        cJSON *str_obj;

        str_obj = cJSON_GetObjectItem(obj, item);

        if (str_obj == NULL) {
                return false;
        }

        *str = cJSON_GetStringValue(str_obj);

        if (*str == NULL) {
                return false;
        }

        return true;
}

int codec_encode_beacon_list(char *buf, size_t buf_len)
{
        int err;
        size_t i;
        const char *uuid;
        const char *oob_info;
        const uint32_t *uri_hash;
        cJSON *beacon_list_obj;
        cJSON *event_obj;
        cJSON *beacons_obj;
        cJSON *beacon_obj;

        if (!codec_init_event(&beacon_list_obj, &event_obj, "beacon_list")) {
                return -ENOMEM;
        }

        err = -ENOMEM;
        beacons_obj  = cJSON_AddArrayToObject(event_obj, "beacons");

        if (beacons_obj == NULL) {
                goto cleanup;
        }

        i = 0;
        uuid = btmesh_get_beacon_uuid(i);
        oob_info = btmesh_get_beacon_oob(i);
        uri_hash = btmesh_get_beacon_uri_hash(i);

        while (uuid != NULL) {
                beacon_obj = cJSON_CreateObject();

                if (beacon_obj == NULL) {
                        goto cleanup;
                }

                cJSON_AddItemToArray(beacons_obj, beacon_obj);

                if (cJSON_AddStringToObject(beacon_obj, JSON_STR_DEVICE_TYPE, JSON_STR_BT_MESH)
				== NULL) {
                        goto cleanup;
                }

                if (cJSON_AddStringToObject(beacon_obj, JSON_STR_UUID, uuid) == NULL) {
                        goto cleanup;
                }

                if (oob_info == NULL) {
                        if (cJSON_AddStringToObject(beacon_obj, JSON_STR_OOB_INFO, JSON_STR_NA)
					== NULL) {
                                goto cleanup;
                        }
                } else {
                        if (cJSON_AddStringToObject(beacon_obj, JSON_STR_OOB_INFO, oob_info)
					== NULL) {
                                goto cleanup;
                        }
                }

                if (uri_hash == NULL) {
                        if (cJSON_AddStringToObject(beacon_obj, JSON_STR_URI_HASH, JSON_STR_NA)
					== NULL) {
                                goto cleanup;
                        }
                } else {
                        if (cJSON_AddNumberToObject(beacon_obj, JSON_STR_URI_HASH, *uri_hash)
					== NULL) {
                                goto cleanup;
                        }
                }

                i++;
                uuid = btmesh_get_beacon_uuid(i);
                oob_info = btmesh_get_beacon_oob(i);
                uri_hash = btmesh_get_beacon_uri_hash(i);
        }

        if (cJSON_AddNumberToObject(beacon_list_obj, JSON_STR_MSG_ID, message_id++) == NULL) {
                goto cleanup;
        }

        if (!cJSON_PrintPreallocated(beacon_list_obj, buf, buf_len, 0)) {
                goto cleanup;
        }

        err = 0;

cleanup:
        cJSON_Delete(beacon_list_obj);
        return err;
}


int codec_encode_prov_result(char *buf,  size_t buf_len, int prov_err,
                uint16_t net_idx, uint8_t uuid[UUID_LEN], uint16_t addr, uint8_t num_elem)
{
        int err;
        char uuid_str[UUID_STR_LEN];
        cJSON *prov_result_obj;
        cJSON *event_obj;

        if (!codec_init_event(&prov_result_obj, &event_obj, "provision_result")) {
                return -ENOMEM;
        }

        err = -ENOMEM;

        if (cJSON_AddNumberToObject(event_obj, JSON_STR_ERR, prov_err) == NULL) {
                goto cleanup;
        }

        if (uuid == NULL) {
                memset(uuid_str, '0', sizeof(uuid_str));
        } else  {
                util_uuid2str(uuid, uuid_str);
        }

        if (cJSON_AddStringToObject(event_obj, JSON_STR_UUID, uuid_str) == NULL) {
                goto cleanup;
        }

        if (prov_err) {
                /* Other items irrelevent on provision err, so just send as is */
                goto encode;
        }

        if(cJSON_AddNumberToObject(event_obj, JSON_STR_NET_IDX, net_idx) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddNumberToObject(event_obj, JSON_STR_ADDR, addr) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddNumberToObject(event_obj, JSON_STR_ELEM_COUNT, num_elem) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddNumberToObject(prov_result_obj, JSON_STR_MSG_ID, message_id++) == NULL) {
                goto cleanup;
        }

encode:
        if (!cJSON_PrintPreallocated(prov_result_obj, buf, buf_len, 0)) {
                goto cleanup;
        }

        err = 0;

cleanup:
        cJSON_Delete(prov_result_obj);
        return err;
}

int codec_parse_prov(cJSON *op_obj, uint8_t uuid[UUID_LEN], uint16_t *net_idx, uint16_t *addr,
		uint8_t *attn)
{
	char *uuid_str;

	if (!codec_get_str(op_obj, JSON_STR_UUID, &uuid_str) ||
	    !codec_get_uint16(op_obj, JSON_STR_NET_IDX, net_idx) ||
	    !codec_get_uint16(op_obj, JSON_STR_ADDR, addr) ||
	    !codec_get_uint8(op_obj, JSON_STR_ATTN, attn)) {
		return -EINVAL;
	}

	util_str2uuid(uuid_str, uuid);

	return 0;
}

int codec_encode_subnet_list(char *buf, size_t buf_len)
{
        int err;
        uint8_t i;
        cJSON *subnet_list_obj;
        cJSON *event_obj;
        cJSON *subnets_obj;
        cJSON *subnet_obj;
        struct bt_mesh_cdb_subnet *subnet;

        if (!codec_init_event(&subnet_list_obj, &event_obj, "subnet_list")) {
                return -ENOMEM;
        }

        subnets_obj = cJSON_AddArrayToObject(event_obj, "subnetList");
        err = -ENOMEM;

        if (subnets_obj == NULL) {
                goto cleanup;
        }

        subnet = bt_mesh_cdb.subnets;

        for (i = 0; i < SUBNET_COUNT; i++) {
                if (subnet[i].net_idx == BT_MESH_KEY_UNUSED) {
                        continue;
                }

                subnet_obj = cJSON_CreateObject();

                if (subnet_obj == NULL) {
                        goto cleanup;
                }

                cJSON_AddItemToArray(subnets_obj, subnet_obj);

                if (cJSON_AddNumberToObject(subnet_obj, JSON_STR_NET_IDX, subnet[i].net_idx)
				== NULL) {
                        goto cleanup;
                }
        }

        if (!cJSON_PrintPreallocated(subnet_list_obj, buf, buf_len, 0)) {
                goto cleanup;
        }

        err = 0;

cleanup:
        cJSON_Delete(subnet_list_obj);
        return err;
}

int codec_parse_subnet(cJSON *op_obj, uint16_t *net_idx)
{
	if (!codec_get_uint16(op_obj, JSON_STR_NET_IDX, net_idx)) {
		return -EINVAL;
	}

	return 0;
}

int codec_parse_subnet_add(cJSON *op_obj, uint16_t *net_idx, uint8_t net_key[KEY_LEN])
{
	char *net_key_str;

	if (!codec_get_str(op_obj, JSON_STR_NET_KEY, &net_key_str)) {
		return -EINVAL;
	}

	util_str2key(net_key_str, net_key);
		
	return codec_parse_subnet(op_obj, net_idx);
}

int codec_encode_app_key_list(char *buf, size_t buf_len)
{
        int err;
        uint8_t i;
        cJSON *app_key_list_obj;
        cJSON *event_obj;
        cJSON *app_keys_obj;
        cJSON *app_key_obj;
        struct bt_mesh_cdb_app_key *app_keys;

        if (!codec_init_event(&app_key_list_obj, &event_obj, "app_key_list")) {
                return -ENOMEM;
        }

        app_keys_obj = cJSON_AddArrayToObject(event_obj, "appKeyList");
        err = - ENOMEM;

        if (app_keys_obj == NULL) {
                goto cleanup;
        }

        app_keys = bt_mesh_cdb.app_keys;

        for (i = 0; i < APP_KEY_COUNT; i++) {
                if (app_keys[i].net_idx == BT_MESH_KEY_UNUSED) {
                        continue;
                }

                app_key_obj = cJSON_CreateObject();

                if (app_key_obj == NULL) {
                        goto cleanup;
                }

                cJSON_AddItemToArray(app_keys_obj, app_key_obj);

                if (cJSON_AddNumberToObject(app_key_obj, JSON_STR_APP_IDX, app_keys[i].app_idx)
				== NULL) {
                        goto cleanup;
                }

                if (cJSON_AddNumberToObject(app_key_obj, JSON_STR_NET_IDX, app_keys[i].net_idx)
				== NULL) {
                        goto cleanup;
                }
        }

        if (!cJSON_PrintPreallocated(app_key_list_obj, buf, buf_len, 0)) {
                goto cleanup;
        }

        err = 0;

cleanup:
        cJSON_Delete(app_key_list_obj);
        return err;
}

int codec_parse_app_key(cJSON *op_obj, uint16_t *net_idx, uint16_t *app_idx)
{
	if (!codec_get_uint16(op_obj, JSON_STR_NET_IDX, net_idx) ||
	    !codec_get_uint16(op_obj, JSON_STR_APP_IDX, app_idx)) {
		return -EINVAL;
	}

	return 0;
}

int codec_parse_app_key_add(cJSON *op_obj, uint16_t *net_idx, uint16_t *app_idx,
		uint8_t app_key[KEY_LEN])
{
	char *app_key_str;

	if (!codec_get_str(op_obj, JSON_STR_APP_KEY, &app_key_str)) {
		return -EINVAL;
	}

	util_str2key(app_key_str, app_key);

	return codec_parse_app_key(op_obj, net_idx, app_idx);
}

static uint8_t encode_node(struct bt_mesh_cdb_node *node, void *nodes)
{
        char uuid_str[UUID_STR_LEN];
        cJSON *node_array_obj;
        cJSON *node_obj;

        node_array_obj = (cJSON *)nodes;

        if (node_array_obj == NULL) {
                goto error;
        }

        node_obj = cJSON_CreateObject();

        if (node_obj == NULL) {
                goto error;
        }

        cJSON_AddItemToArray(node_array_obj, node_obj);

        if (cJSON_AddStringToObject(node_obj, JSON_STR_DEVICE_TYPE, JSON_STR_BT_MESH) == NULL) {
                goto error;
        }

        util_uuid2str(node->uuid, uuid_str);

        if (cJSON_AddStringToObject(node_obj, JSON_STR_UUID, uuid_str) == NULL) {
                goto error;
        }

        if (cJSON_AddNumberToObject(node_obj, JSON_STR_NET_IDX, node->net_idx) == NULL) {
                goto error;
        }

        if (cJSON_AddNumberToObject(node_obj, JSON_STR_ADDR, node->addr) == NULL) {
                goto error;
        }

        if (cJSON_AddNumberToObject(node_obj, JSON_STR_ELEM_COUNT, node->num_elem) == NULL) {
                goto error;
        }

        return BT_MESH_CDB_ITER_CONTINUE;

error:
        cJSON_Delete(nodes);
        return BT_MESH_CDB_ITER_STOP;
}

int codec_parse_node_disc(cJSON *op_obj, uint16_t *addr)
{
	if (!codec_get_uint16(op_obj, JSON_STR_ADDR, addr)) {
		return -EINVAL;
	}
	
	return 0;
}

int codec_encode_node_list(char *buf, size_t buf_len)
{
        int err;
        cJSON *node_list_obj;
        cJSON *event_obj;
        cJSON *nodes_obj;

        if (!codec_init_event(&node_list_obj, &event_obj, "node_list")) {
                return -ENOMEM;
        }

        err = -ENOMEM;

        nodes_obj = cJSON_AddArrayToObject(event_obj, "nodes");

        if (nodes_obj == NULL) {
                goto cleanup;
        }

        bt_mesh_cdb_node_foreach(encode_node, (void*)nodes_obj);

        if (nodes_obj == NULL) {
                goto cleanup;
        }

        if (cJSON_AddNumberToObject(node_list_obj, JSON_STR_MSG_ID, message_id++) == NULL ) {
                goto cleanup;
        }

        if (!cJSON_PrintPreallocated(node_list_obj, buf, buf_len, 0)) {
                goto cleanup;
        }

        err = 0;

cleanup:
        cJSON_Delete(node_list_obj);
        return err;
}

int codec_encode_node_disc(char *buf, size_t buf_len, struct btmesh_node *node, int disc_err,
                uint8_t status)
{
        int err;
        int *temp_int_array;
        uint8_t i;
        uint8_t j;
        uint8_t k;
        struct btmesh_elem *elem;
        struct btmesh_sig_model *sig_model;
        struct btmesh_vnd_model *vnd_model;
        char uuid_str[UUID_STR_LEN];
        cJSON *disc_obj;
        cJSON *event_obj;
        cJSON *feature_obj;
	cJSON *hb_sub_obj;
	cJSON *hb_pub_obj;
        cJSON *array_obj;
        cJSON *elem_array_obj;
        cJSON *elem_obj;
        cJSON *model_array_obj;
        cJSON *model_obj;
        cJSON *pub_obj;

        if (!codec_init_event(&disc_obj, &event_obj, "node_discover_result")) {
                return -ENOMEM;
        }

        err = -ENOMEM;

        if (cJSON_AddNumberToObject(event_obj, JSON_STR_ERR, disc_err) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddNumberToObject(event_obj, "status", status) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddNumberToObject(event_obj, JSON_STR_ADDR, node->addr) == NULL) {
                goto cleanup;
        }
        
	if (disc_err || status) {
                /* If there was an error or status performing the discovery, then all other items
                 * are irrelevant so encode the JSON as it is */
                goto encode;
        }

        util_uuid2str(node->uuid, uuid_str);

        if (cJSON_AddStringToObject(event_obj, JSON_STR_UUID, uuid_str) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddNumberToObject(event_obj, "cid", node->cid) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddNumberToObject(event_obj, "pid", node->pid) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddNumberToObject(event_obj, "vid", node->vid) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddNumberToObject(event_obj, "crpl", node->crpl) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddBoolToObject(event_obj, "networkBeaconState", node->net_beacon_state) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddNumberToObject(event_obj, JSON_STR_TTL, node->ttl) == NULL) {
                goto cleanup;
        }

        feature_obj = cJSON_AddObjectToObject(event_obj, JSON_STR_RELAY);

        if (feature_obj == NULL) {
                goto cleanup;
        }

        if (cJSON_AddBoolToObject(feature_obj, JSON_STR_SUPPORT, node->relay.support) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddBoolToObject(feature_obj, JSON_STR_STATE, node->relay.state) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddNumberToObject(feature_obj, JSON_STR_TX_COUNT, node->relay.count) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddNumberToObject(feature_obj, JSON_STR_TX_INT, node->relay.interval) == NULL) {
                goto cleanup;
        }

        feature_obj = cJSON_AddObjectToObject(event_obj, JSON_STR_PROXY);

        if (feature_obj == NULL) {
                goto cleanup;
        }

        if (cJSON_AddBoolToObject(feature_obj, JSON_STR_SUPPORT, node->proxy.support) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddBoolToObject(feature_obj, JSON_STR_STATE, node->proxy.state) == NULL) {
                goto cleanup;
        }

        feature_obj = cJSON_AddObjectToObject(event_obj, JSON_STR_FRIEND);

        if (feature_obj == NULL) {
                goto cleanup;
        }

        if (cJSON_AddBoolToObject(feature_obj, JSON_STR_SUPPORT, node->friend.support) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddBoolToObject(feature_obj, JSON_STR_STATE, node->friend.state) == NULL) {
                goto cleanup;
        }

        feature_obj = cJSON_AddObjectToObject(event_obj, JSON_STR_LPN);

        if (feature_obj == NULL) {
                goto cleanup;
        }

        if (cJSON_AddBoolToObject(feature_obj, JSON_STR_STATE, node->lpn) == NULL) {
                goto cleanup;
        }

	hb_sub_obj = cJSON_AddObjectToObject(event_obj, "heartbeatSubscribe");

	if (hb_sub_obj == NULL) {
		goto cleanup;
	}

	if (cJSON_AddNumberToObject(hb_sub_obj, JSON_STR_SRC_ADDR, node->hb_sub.src) == NULL ||
	    cJSON_AddNumberToObject(hb_sub_obj, JSON_STR_DST_ADDR, node->hb_sub.dst) == NULL ||
	    cJSON_AddNumberToObject(hb_sub_obj, JSON_STR_PERIOD, node->hb_sub.period) == NULL ||
	    cJSON_AddNumberToObject(hb_sub_obj, JSON_STR_COUNT, node->hb_sub.count) == NULL ||
	    cJSON_AddNumberToObject(hb_sub_obj, JSON_STR_MIN_HOPS, node->hb_sub.min) == NULL ||
	    cJSON_AddNumberToObject(hb_sub_obj, JSON_STR_MAX_HOPS, node->hb_sub.max) == NULL) {
		goto cleanup;
	}

	hb_pub_obj = cJSON_AddObjectToObject(event_obj, "heartbeatPublish");

	if (hb_pub_obj == NULL) {
		goto cleanup;
	}

	if (cJSON_AddNumberToObject(hb_pub_obj, JSON_STR_DST_ADDR, node->hb_pub.dst) == NULL ||
	    cJSON_AddNumberToObject(hb_pub_obj, JSON_STR_COUNT, node->hb_pub.count) == NULL ||
	    cJSON_AddNumberToObject(hb_pub_obj, JSON_STR_PERIOD, node->hb_pub.period) == NULL ||
	    cJSON_AddNumberToObject(hb_pub_obj, JSON_STR_TTL, node->hb_pub.ttl) == NULL ||
	    cJSON_AddBoolToObject(hb_pub_obj, JSON_STR_RELAY,
		    node->hb_pub.feat & BT_MESH_FEAT_RELAY) == NULL ||
	    cJSON_AddBoolToObject(hb_pub_obj, JSON_STR_PROXY,
		    node->hb_pub.feat & BT_MESH_FEAT_PROXY) == NULL ||
	    cJSON_AddBoolToObject(hb_pub_obj, JSON_STR_FRIEND,
		    node->hb_pub.feat & BT_MESH_FEAT_FRIEND) == NULL ||
	    cJSON_AddBoolToObject(hb_pub_obj, JSON_STR_LPN,
		    node->hb_pub.feat & BT_MESH_FEAT_LOW_POWER) == NULL) {
		goto cleanup;
	}

        temp_int_array = (int *)k_malloc(sizeof(int) * node->subnet_count);

        if (temp_int_array == NULL) {
                goto cleanup;
        }

        for (i = 0; i < node->subnet_count; i++) {
                temp_int_array[i] = (int)node->subnet_idxs[i];
        }

        array_obj = cJSON_CreateIntArray(temp_int_array, node->subnet_count);

        if (array_obj == NULL) {
                goto cleanup;
        }

        k_free(temp_int_array);
        cJSON_AddItemToObject(event_obj, "subnets", array_obj);
        elem_array_obj = cJSON_AddArrayToObject(event_obj, "elements");

        if (elem_array_obj == NULL) {
                goto cleanup;
        }

        for (i = 0; i < node->elem_count; i++) {
                elem_obj = cJSON_CreateObject();

                if (elem_obj == NULL) {
                        goto cleanup;
                }

                cJSON_AddItemToArray(elem_array_obj, elem_obj);
                elem = &(node->elems[i]);

                if (cJSON_AddNumberToObject(elem_obj, JSON_STR_ADDR, elem->addr) == NULL) {
                        goto cleanup;
                }

                if (cJSON_AddNumberToObject(elem_obj, "loc", elem->loc) == NULL) {
                        goto cleanup;
                }

                model_array_obj = cJSON_AddArrayToObject(elem_obj, "sigModels");

                if (model_array_obj == NULL) {
                        goto cleanup;
                }

                for (j = 0; j < elem->sig_model_count; j++) {
                        model_obj = cJSON_CreateObject();

                        if (model_obj == NULL) {
                                goto cleanup;
                        }

                        cJSON_AddItemToArray(model_array_obj, model_obj);
                        sig_model = &elem->sig_models[j];

                        if (cJSON_AddNumberToObject(model_obj, JSON_STR_MOD_ID, sig_model->model_id)
					== NULL) {
                                goto cleanup;
                        }
                        /* Add appkey indexes to model */
			if (sig_model->appkey_count > 0) {
				temp_int_array = (int *)k_malloc(sizeof(int) * sig_model->appkey_count);

				if (temp_int_array == NULL) {
					goto cleanup;
				}

				for (k = 0; k < sig_model->appkey_count; k++) {
					temp_int_array[k] = (int)sig_model->appkey_idxs[k];
				}

				array_obj = cJSON_CreateIntArray(temp_int_array, sig_model->appkey_count);

				if (array_obj == NULL) {
					goto cleanup;
				}

				k_free(temp_int_array);
			} else {
				array_obj = cJSON_CreateArray();
			}
			cJSON_AddItemToObject(model_obj, JSON_STR_APP_IDXS, array_obj);

                        /* Add subscribe addresses to model */
			if (sig_model->sub_addr_count > 0) {
				temp_int_array = (int *)k_malloc(sizeof(int) * sig_model->sub_addr_count);

				if (temp_int_array == NULL) {
					goto cleanup;
				}

				for (k = 0; k < sig_model->sub_addr_count; k++) {
					temp_int_array[k] = (int)sig_model->sub_addrs[k];
				}

				array_obj = cJSON_CreateIntArray(temp_int_array, sig_model->sub_addr_count);

				if (array_obj == NULL) {
					goto cleanup;
				}

				k_free(temp_int_array);
			} else {
				array_obj = cJSON_CreateArray();
			}
			cJSON_AddItemToObject(model_obj, JSON_STR_SUB_ADDRS, array_obj);

                        /* Add publish parameters to model */
                        pub_obj = cJSON_AddObjectToObject(model_obj, JSON_STR_PUB_PARAMS);

                        if (pub_obj == NULL) {
                                goto cleanup;
                        }

                        if (cJSON_AddNumberToObject(pub_obj, JSON_STR_ADDR, sig_model->pub.addr) == NULL) {
                                goto cleanup;
                        }

                        if (cJSON_AddNumberToObject(pub_obj, JSON_STR_APP_IDX,
						sig_model->pub.app_idx) == NULL) {
                                goto cleanup;
                        }

                        if (cJSON_AddBoolToObject(pub_obj, JSON_STR_FRIEND_CRED_FLAG,
                                                sig_model->pub.cred_flag) == NULL) {
                                goto cleanup;
                        }

                        if (cJSON_AddNumberToObject(pub_obj, JSON_STR_TTL, sig_model->pub.ttl) == NULL) {
                                goto cleanup;
                        }

                        if (cJSON_AddNumberToObject(pub_obj, JSON_STR_PERIOD, 
                                                btmesh_get_pub_period(sig_model->pub.period)) == NULL) {
                                goto cleanup;
                        }

                        if (cJSON_AddStringToObject(pub_obj, JSON_STR_PERIOD_UNITS,
                                                btmesh_get_pub_period_unit_str(sig_model->pub.period)) == NULL) {
                                goto cleanup;
                        }

                        if (cJSON_AddNumberToObject(pub_obj, JSON_STR_TX_COUNT,
                                                BT_MESH_TRANSMIT_COUNT(sig_model->pub.transmit)) == NULL) {
                                goto cleanup;
                        }

                        if (cJSON_AddNumberToObject(pub_obj, JSON_STR_TX_INT,
                                                BT_MESH_TRANSMIT_INT(sig_model->pub.transmit)) == NULL) {
                                goto cleanup;
                        }
                }

                // Get array of vendor models */
                model_array_obj = cJSON_AddArrayToObject(elem_obj, "vendorModels");

                if (model_array_obj == NULL) {
                        goto cleanup;
                }
                
		for (j = 0; j < elem->vnd_model_count; j++) {
                        model_obj = cJSON_CreateObject();

                        if (model_obj == NULL) {
                                goto cleanup;
                        }

                        cJSON_AddItemToArray(model_array_obj, model_obj);
                        vnd_model = &elem->vnd_models[j];

                        if (cJSON_AddNumberToObject(model_obj, JSON_STR_CID, vnd_model->company_id)
					== NULL) {
                                goto cleanup;
                        }

                        if (cJSON_AddNumberToObject(model_obj, JSON_STR_MOD_ID, vnd_model->model_id)
					== NULL) {
                                goto cleanup;
                        }

                        /* Add appkey indexes to model */
			if (vnd_model->appkey_count > 0) {
				temp_int_array = (int *)k_malloc(sizeof(int) * vnd_model->appkey_count);

				if (temp_int_array == NULL) {
					goto cleanup;
				}

				for (k = 0; k < vnd_model->appkey_count; k++) {
					temp_int_array[k] = (int)vnd_model->appkey_idxs[k];
				}

				array_obj = cJSON_CreateIntArray(temp_int_array, vnd_model->appkey_count);

				if (array_obj == NULL) {
					goto cleanup;
				}

				k_free(temp_int_array);
			} else {
				array_obj = cJSON_CreateArray();
			}
                        cJSON_AddItemToObject(model_obj, JSON_STR_APP_IDXS, array_obj);

                        /* Add subscribe addresses to model */
			if (vnd_model->sub_addr_count > 0) {
				temp_int_array = (int *)k_malloc(sizeof(int) * vnd_model->sub_addr_count);

				if (temp_int_array == NULL) {
					goto cleanup;
				}

				array_obj = cJSON_CreateIntArray(temp_int_array, vnd_model->sub_addr_count);

				if (array_obj == NULL) {
					goto cleanup;
				}

				k_free(temp_int_array);
			} else {
				array_obj = cJSON_CreateArray();
			}
                        cJSON_AddItemToObject(model_obj, JSON_STR_SUB_ADDRS, array_obj);

                        /* Add publish parameters to model */
                        pub_obj = cJSON_AddObjectToObject(model_obj, JSON_STR_PUB_PARAMS);

                        if (pub_obj == NULL) {
                                goto cleanup;
                        }

                        if (cJSON_AddNumberToObject(pub_obj, JSON_STR_ADDR, vnd_model->pub.addr)
					== NULL) {
                                goto cleanup;
                        }

                        if (cJSON_AddNumberToObject(pub_obj, JSON_STR_APP_IDX,
						vnd_model->pub.app_idx)
					== NULL) {
                                goto cleanup;
                        }

                        if (cJSON_AddBoolToObject(pub_obj, JSON_STR_FRIEND_CRED_FLAG,
                                                vnd_model->pub.cred_flag) == NULL) {
                                goto cleanup;
                        }

                        if (cJSON_AddNumberToObject(pub_obj, JSON_STR_TTL, vnd_model->pub.ttl)
					== NULL) {
                                goto cleanup;
                        }

                        if (cJSON_AddNumberToObject(pub_obj, JSON_STR_PERIOD,
                                                btmesh_get_pub_period(vnd_model->pub.period)) == NULL) {
                                goto cleanup;
                        }

                        if (cJSON_AddStringToObject(pub_obj, JSON_STR_PERIOD_UNITS,
                                                btmesh_get_pub_period_unit_str(vnd_model->pub.period)) == NULL) {
                                goto cleanup;
                        }

                        if (cJSON_AddNumberToObject(pub_obj, JSON_STR_TX_COUNT,
                                                BT_MESH_TRANSMIT_COUNT(vnd_model->pub.transmit)) == NULL) {
                                goto cleanup;
                        }

                        if (cJSON_AddNumberToObject(pub_obj, JSON_STR_TX_INT,
                                                BT_MESH_TRANSMIT_INT(vnd_model->pub.transmit)) == NULL) {
                                goto cleanup;
                        }
                }
        }

        if (cJSON_AddNumberToObject(disc_obj, JSON_STR_MSG_ID, message_id++) == NULL) {
                goto cleanup;
        }

encode:

        if (!cJSON_PrintPreallocated(disc_obj, buf, buf_len, 0)) {
                goto cleanup;
        }

        err = 0;

cleanup:
        cJSON_Delete(disc_obj);
        return err;
}

static bool parse_cfg_op(cJSON *op_obj, enum btmesh_op *op)
{
        char *cfg_type_str;

        codec_get_str(op_obj, "configuration", &cfg_type_str);

        if (cfg_type_str == NULL) {
                return false;
        }

        if (!strcmp(cfg_type_str, "networkBeaconSet")) {
                *op = BTMESH_OP_BEACON_SET;
        } else if (!strcmp(cfg_type_str, "timeToLiveSet")) {
                *op = BTMESH_OP_TTL_SET;
        } else if (!strcmp(cfg_type_str, "relayFeatureSet")) {
                *op = BTMESH_OP_RELAY_SET;
        } else if (!strcmp(cfg_type_str, "friendFeatureSet")) {
                *op = BTMESH_OP_FRIEND_SET;
        } else if (!strcmp(cfg_type_str, "proxyFeatureSet")) {
                *op = BTMESH_OP_PROXY_SET;
        } else if (!strcmp(cfg_type_str, "subnetAdd")) {
                *op = BTMESH_OP_NET_KEY_ADD;
        } else if (!strcmp(cfg_type_str, "subnetDelete")) {
                *op = BTMESH_OP_NET_KEY_DEL;
        } else if (!strcmp(cfg_type_str, "appKeyBind")) {
                *op = BTMESH_OP_MOD_APP_BIND;
        } else if (!strcmp(cfg_type_str, "appKeyBindVnd")) {
                *op = BTMESH_OP_MOD_APP_BIND_VND;
        } else if (!strcmp(cfg_type_str, "appKeyUnbind")) {
                *op = BTMESH_OP_MOD_APP_UNBIND;
        } else if (!strcmp(cfg_type_str, "appKeyUnbindVnd")) {
                *op = BTMESH_OP_MOD_APP_UNBIND_VND;
        } else if (!strcmp(cfg_type_str, "publishParametersSet")) {
                *op = BTMESH_OP_MOD_PUB_SET;
        } else if (!strcmp(cfg_type_str, "publishParametersSetVnd")) {
                *op = BTMESH_OP_MOD_PUB_SET_VND;
        } else if (!strcmp(cfg_type_str, "subscribeAddressAdd")) {
                *op = BTMESH_OP_MOD_SUB_ADD;
        } else if (!strcmp(cfg_type_str, "subscribeAddressAddVnd")) {
                *op = BTMESH_OP_MOD_SUB_ADD_VND;
        } else if (!strcmp(cfg_type_str, "subscribeAddressDelete")) {
                *op = BTMESH_OP_MOD_SUB_DEL;
        } else if (!strcmp(cfg_type_str, "subscribeAddressDeleteVnd")) {
                *op = BTMESH_OP_MOD_SUB_DEL_VND;
        } else if (!strcmp(cfg_type_str, "subscribeAddressOverwrite")) {
                *op = BTMESH_OP_MOD_SUB_OVRW;
        } else if (!strcmp(cfg_type_str, "subscribeAddressOverwriteVnd")) {
                *op = BTMESH_OP_MOD_SUB_OVRW_VND;
	} else if (!strcmp(cfg_type_str, "heartbeatSubscribeSet")) {
		*op = BTMESH_OP_HB_SUB_SET;
	} else if (!strcmp(cfg_type_str, "heartbeatPublishSet")) {
		*op = BTMESH_OP_HB_PUB_SET;
        } else {
                LOG_ERR("UNRECOGNIZED CFG OP: %s", log_strdup(cfg_type_str));
                return false;
        }
        
	LOG_DBG("CFG OP: %d", *op);
        return true;
}

static int parse_cfg_args(cJSON *op_obj, enum btmesh_op op, union btmesh_op_args *args)
{
        switch (op) {
	case BTMESH_OP_BEACON_SET:
		args->beacon_set.net_idx = PRIMARY_SUBNET;
		if (!codec_get_uint16(op_obj, JSON_STR_ADDR, &(args->beacon_set.addr)) ||
		    !codec_get_bool(op_obj, JSON_STR_STATE, (bool*)&(args->beacon_set.val))) {
			return -EINVAL;
		}
		break;

	case BTMESH_OP_TTL_SET:
		args->ttl_set.net_idx = PRIMARY_SUBNET;
		if (!codec_get_uint16(op_obj, JSON_STR_ADDR, &(args->ttl_set.addr)) ||
		    !codec_get_uint8(op_obj, JSON_STR_TTL, &(args->ttl_set.val))) {
			return -EINVAL;
		}
		break;

	case BTMESH_OP_RELAY_SET:
	{
		uint8_t count;
		uint16_t interval;

		args->relay_set.net_idx = PRIMARY_SUBNET;
		if (!codec_get_uint16(op_obj, JSON_STR_ADDR, &(args->relay_set.addr)) ||
		    !codec_get_bool(op_obj, JSON_STR_STATE, (bool*)&(args->relay_set.new_relay)) ||
		    !codec_get_uint8(op_obj, JSON_STR_TX_COUNT, &count) ||
		    !codec_get_uint16(op_obj, JSON_STR_TX_INT, &interval)) {
			return -EINVAL;
		}
		args->relay_set.new_transmit = BT_MESH_TRANSMIT(count, interval);
		break;
	}

	case BTMESH_OP_FRIEND_SET:
		args->friend_set.net_idx = PRIMARY_SUBNET;
		if (!codec_get_uint16(op_obj, JSON_STR_ADDR, &(args->friend_set.addr)) ||
		    !codec_get_bool(op_obj, JSON_STR_STATE, (bool*)&(args->friend_set.val))) {
			return -EINVAL;
		}
		break;

	case BTMESH_OP_PROXY_SET:
		args->proxy_set.net_idx = PRIMARY_SUBNET;
		if (!codec_get_uint16(op_obj, JSON_STR_ADDR, &(args->proxy_set.addr)) ||
		    !codec_get_bool(op_obj, JSON_STR_STATE, (bool*)&(args->proxy_set.val))) {
			return -EINVAL;
		}
		break;

	case BTMESH_OP_NET_KEY_ADD:
	{
		uint16_t key_net_idx;
		struct bt_mesh_cdb_subnet *subnet;

		args->net_key_add.net_idx = PRIMARY_SUBNET;
		if (!codec_get_uint16(op_obj, JSON_STR_ADDR, &(args->net_key_add.addr)) ||
		    !codec_get_uint16(op_obj, JSON_STR_NET_IDX, &key_net_idx)) {
			return -EINVAL;
		}
		subnet = bt_mesh_cdb_subnet_get(key_net_idx);
		if (subnet == NULL) {
			return -ENOEXEC;
		}
		args->net_key_add.key_net_idx = key_net_idx;
		memcpy(args->net_key_add.net_key, subnet->keys[0].net_key, KEY_LEN);
		break;
	}

	case BTMESH_OP_NET_KEY_DEL:
		args->net_key_del.net_idx = PRIMARY_SUBNET;
		if (!codec_get_uint16(op_obj, JSON_STR_ADDR, &(args->net_key_del.addr)) ||
		    !codec_get_uint16(op_obj, JSON_STR_NET_IDX, &(args->net_key_del.key_net_idx))) {
			return -EINVAL;
		}
		break;

	case BTMESH_OP_MOD_APP_BIND:
	case BTMESH_OP_MOD_APP_BIND_VND:
	{
		int i;
		int err;
		uint16_t elem_addr;
		uint16_t model_id;
		uint16_t cid;
		uint16_t app_idx;
		struct bt_mesh_cdb_app_key *app_key;

		args->app_key_get.net_idx = PRIMARY_SUBNET;
		if (!codec_get_uint16(op_obj, JSON_STR_ADDR, &(args->app_key_get.addr)) ||
		    !codec_get_uint16(op_obj, JSON_STR_ELEM_ADDR, &elem_addr) ||
		    !codec_get_uint16(op_obj, JSON_STR_MOD_ID, &model_id) ||
		    !codec_get_uint16(op_obj, JSON_STR_APP_IDX, &app_idx)) {
			return -EINVAL;
		}

		if (op == BTMESH_OP_MOD_APP_BIND_VND) {
			if (!codec_get_uint16(op_obj, JSON_STR_CID, &cid)) {
				return -EINVAL;
			}
		}

		app_key = bt_mesh_cdb_app_key_get(app_idx);

		if (app_key == NULL) {
			return -ENOEXEC;
		}

		args->app_key_get.key_net_idx = app_key->net_idx;
		args->app_key_get.key_cnt = sizeof(args->app_key_get.keys);

		err = btmesh_perform_op(BTMESH_OP_APP_KEY_GET, args);

		if (err) {
			return -ENOEXEC;
		}

		for (i = 0; i < args->app_key_get.key_cnt; i++) {
			if (args->app_key_get.keys[i] == app_idx) {
				break;
			}
		}

		if (i >= args->app_key_get.key_cnt) {
			args->app_key_add.key_app_idx = app_idx;
			memcpy(args->app_key_add.app_key, app_key->keys[0].app_key, KEY_LEN);
			err = btmesh_perform_op(BTMESH_OP_APP_KEY_ADD, args);

			if (err) {
				return -ENOEXEC;
			}
		}

		args->mod_app_bind.elem_addr = elem_addr;
		args->mod_app_bind.mod_app_idx = app_idx;
		args->mod_app_bind.mod_id = model_id;

		if (op == BTMESH_OP_MOD_APP_BIND_VND) {
			args->mod_app_bind_vnd.cid = cid;
		}

		break;
	}

	case BTMESH_OP_MOD_APP_UNBIND:
	case BTMESH_OP_MOD_APP_UNBIND_VND:
		args->mod_app_unbind.net_idx = PRIMARY_SUBNET;
		if (!codec_get_uint16(op_obj, JSON_STR_ADDR, &(args->mod_app_unbind.addr)) ||
		    !codec_get_uint16(op_obj, JSON_STR_ELEM_ADDR,
			    &(args->mod_app_unbind.elem_addr)) ||
		    !codec_get_uint16(op_obj, JSON_STR_MOD_ID, &(args->mod_app_unbind.mod_id)) ||
		    !codec_get_uint16(op_obj, JSON_STR_APP_IDX,
			    &(args->mod_app_unbind.mod_app_idx))) {
			return -EINVAL;
		}
		if (op == BTMESH_OP_MOD_APP_UNBIND) {
			if (!codec_get_uint16(op_obj, JSON_STR_CID,
						&(args->mod_app_unbind_vnd.cid))) {
				return -EINVAL;
			}
		}
		break;

	case BTMESH_OP_MOD_PUB_SET:
	case BTMESH_OP_MOD_PUB_SET_VND:
	{
		char *units;
		uint8_t period;
		uint8_t count;
		uint16_t interval;
		args->mod_pub_set.net_idx = PRIMARY_SUBNET;
		if (!codec_get_uint16(op_obj, JSON_STR_ADDR, &(args->mod_pub_set.addr)) ||
		    !codec_get_uint16(op_obj, JSON_STR_ELEM_ADDR, &(args->mod_pub_set.elem_addr)) ||
		    !codec_get_uint16(op_obj, JSON_STR_MOD_ID, &(args->mod_pub_set.mod_id)) ||
		    !codec_get_uint16(op_obj, JSON_STR_PUB_ADDR, &(args->mod_pub_set.pub.addr)) ||
		    !codec_get_uint16(op_obj, JSON_STR_APP_IDX, &(args->mod_pub_set.pub.app_idx)) ||
		    !codec_get_bool(op_obj, JSON_STR_FRIEND_CRED_FLAG,
			    &(args->mod_pub_set.pub.cred_flag)) ||
		    !codec_get_uint8(op_obj, JSON_STR_TTL, &(args->mod_pub_set.pub.ttl)) ||
		    !codec_get_uint8(op_obj, JSON_STR_PERIOD, &period) ||
		    !codec_get_str(op_obj, JSON_STR_PERIOD_UNITS, &units) ||
		    !codec_get_uint8(op_obj, JSON_STR_TX_COUNT, &count) ||
		    !codec_get_uint16(op_obj, JSON_STR_TX_INT, &interval)) {
			return -EINVAL;
		}

		if (op == BTMESH_OP_MOD_PUB_SET_VND) {
			if (!codec_get_uint16(op_obj, JSON_STR_CID, &(args->mod_pub_set_vnd.cid))) {
				return -EINVAL;
			}
		}

		if (!strcmp(units, "100ms")) {
			args->mod_pub_set.pub.period = BT_MESH_PUB_PERIOD_100MS(period);
			break;
		} else if (!strcmp(units, "1s")) {
			args->mod_pub_set.pub.period = BT_MESH_PUB_PERIOD_SEC(period);
			break;
		} else if (!strcmp(units, "10s")) {
			args->mod_pub_set.pub.period = BT_MESH_PUB_PERIOD_10SEC(period);
			break;
		} else if (!strcmp(units, "10m")) {
			args->mod_pub_set.pub.period = BT_MESH_PUB_PERIOD_10MIN(period);
			break;
		} else {
			return -EINVAL;
		}

		args->mod_pub_set.pub.transmit = BT_MESH_TRANSMIT(count, interval);

		break;
	}

	case BTMESH_OP_MOD_SUB_ADD:
	case BTMESH_OP_MOD_SUB_ADD_VND:
		args->mod_sub_add.net_idx = PRIMARY_SUBNET;
		if (!codec_get_uint16(op_obj, JSON_STR_ADDR, &(args->mod_sub_add.addr)) ||
		    !codec_get_uint16(op_obj, JSON_STR_ELEM_ADDR, &(args->mod_sub_add.elem_addr)) ||
		    !codec_get_uint16(op_obj, JSON_STR_MOD_ID, &(args->mod_sub_add.mod_id)) ||
		    !codec_get_uint16(op_obj, JSON_STR_SUB_ADDR, &(args->mod_sub_add.sub_addr))) {
		       return -ENOEXEC;
		}	       
		if (op == BTMESH_OP_MOD_SUB_ADD_VND) {
			if (!codec_get_uint16(op_obj, JSON_STR_CID, &(args->mod_sub_add_vnd.cid))) {
				return -ENOEXEC;
			}
		}
		break;

	case BTMESH_OP_MOD_SUB_DEL:
	case BTMESH_OP_MOD_SUB_DEL_VND:
		args->mod_sub_del.net_idx = PRIMARY_SUBNET;
		if (!codec_get_uint16(op_obj, JSON_STR_ADDR, &(args->mod_sub_del.addr)) ||
		    !codec_get_uint16(op_obj, JSON_STR_ELEM_ADDR, &(args->mod_sub_del.elem_addr)) ||
		    !codec_get_uint16(op_obj, JSON_STR_MOD_ID, &(args->mod_sub_del.mod_id)) ||
		    !codec_get_uint16(op_obj, JSON_STR_SUB_ADDR, &(args->mod_sub_del.sub_addr))) {
			return -EINVAL;
		}
		if (op == BTMESH_OP_MOD_SUB_DEL_VND) {
			if (!codec_get_uint16(op_obj, JSON_STR_CID, &(args->mod_sub_del_vnd.cid))) {
				return -EINVAL;
			}
		}
		break;

	case BTMESH_OP_MOD_SUB_OVRW:
	case BTMESH_OP_MOD_SUB_OVRW_VND:
		args->mod_sub_ovrw.net_idx = PRIMARY_SUBNET;
		if (!codec_get_uint16(op_obj, JSON_STR_ADDR, &(args->mod_sub_ovrw.addr)) ||
		    !codec_get_uint16(op_obj, JSON_STR_ELEM_ADDR, &(args->mod_sub_ovrw.elem_addr)) ||
		    !codec_get_uint16(op_obj, JSON_STR_MOD_ID, &(args->mod_sub_ovrw.mod_id)) ||
		    !codec_get_uint16(op_obj, JSON_STR_SUB_ADDR, &(args->mod_sub_ovrw.sub_addr))) {
			return -EINVAL;
		}
		if (op == BTMESH_OP_MOD_SUB_OVRW_VND) {
			if (!codec_get_uint16(op_obj, JSON_STR_CID, &(args->mod_sub_ovrw_vnd.cid))) {
				return -EINVAL;
			}
		}
		break;

	case BTMESH_OP_HB_SUB_SET:
		args->hb_sub_set.net_idx = PRIMARY_SUBNET;
		if (!codec_get_uint16(op_obj, JSON_STR_ADDR, &(args->hb_sub_set.addr)) ||
		    !codec_get_uint16(op_obj, JSON_STR_SRC_ADDR, &(args->hb_sub_set.sub.src)) ||
		    !codec_get_uint16(op_obj, JSON_STR_DST_ADDR, &(args->hb_sub_set.sub.dst)) ||
		    !codec_get_uint8(op_obj, JSON_STR_PERIOD, &(args->hb_sub_set.sub.period)) ||
		    !codec_get_uint8(op_obj, JSON_STR_COUNT, &(args->hb_sub_set.sub.count)) ||
		    !codec_get_uint8(op_obj, JSON_STR_MIN_HOPS, &(args->hb_sub_set.sub.min)) ||
		    !codec_get_uint8(op_obj, JSON_STR_MAX_HOPS, &(args->hb_sub_set.sub.max))) {
			return -EINVAL;
		}
		break;

	case BTMESH_OP_HB_PUB_SET:
	{
		bool relay;
		bool proxy;
		bool friend;
		bool lpn;

		args->hb_pub_set.net_idx = PRIMARY_SUBNET;
		if (!codec_get_uint16(op_obj, JSON_STR_ADDR, &(args->hb_pub_set.addr)) ||
		    !codec_get_uint16(op_obj, JSON_STR_NET_IDX, &(args->hb_pub_set.pub.net_idx)) ||
		    !codec_get_uint16(op_obj, JSON_STR_DST_ADDR, &(args->hb_pub_set.pub.dst)) ||
		    !codec_get_uint8(op_obj, JSON_STR_COUNT, &(args->hb_pub_set.pub.count)) ||
		    !codec_get_uint8(op_obj, JSON_STR_PERIOD, &(args->hb_pub_set.pub.period)) ||
		    !codec_get_uint8(op_obj, JSON_STR_TTL, &(args->hb_pub_set.pub.ttl)) ||
		    !codec_get_bool(op_obj, JSON_STR_RELAY, &relay) ||
		    !codec_get_bool(op_obj, JSON_STR_PROXY, &proxy) ||
		    !codec_get_bool(op_obj, JSON_STR_FRIEND, &friend) ||
		    !codec_get_bool(op_obj, JSON_STR_LPN, &lpn)) {
			return -EINVAL;
		}

		args->hb_pub_set.pub.feat =
			(relay ? BT_MESH_FEAT_RELAY : 0) |
			(proxy ? BT_MESH_FEAT_PROXY : 0) |
			(friend ? BT_MESH_FEAT_FRIEND : 0) |
			(lpn ? BT_MESH_FEAT_LOW_POWER : 0);
		break;
	}
	default:
		LOG_ERR("Unsupported configuration operation");
		return false;
        }

        return 0;
}

int codec_parse_node_cfg(cJSON *op_obj, uint16_t *addr)
{
        int err;
        enum btmesh_op op;
        union btmesh_op_args args;

	if (!codec_get_uint16(op_obj, JSON_STR_ADDR, addr)) {
		return -EINVAL;
	}

        if (!parse_cfg_op(op_obj, &op)) {
                return -EINVAL;
        }

	err = parse_cfg_args(op_obj, op, &args);
	if (err) {
		return err;
	}

        err = btmesh_perform_op(op, &args);
        if (err) {
		return -ENOEXEC;
        }

        if (op == BTMESH_OP_MOD_APP_UNBIND) {
                err = btmesh_clean_node_key(args.mod_app_unbind.addr, args.mod_app_unbind.mod_app_idx);

                if (err) {
			return -ENOEXEC;
                }
        }

        return 0;
}

int codec_parse_subscribe_addrs(cJSON *op_obj, uint16_t **addr_list, int *addr_count)
{
        int i;
        cJSON *addr_list_obj;
        cJSON *addr_obj;
        uint16_t addr;

        addr_list_obj = cJSON_GetObjectItem(op_obj, JSON_STR_ADDR_LIST);

        if (addr_list_obj == NULL) {
                return -ENOMEM;
        }

        *addr_count = cJSON_GetArraySize(addr_list_obj);
        *addr_list = k_malloc(sizeof(uint16_t) * *addr_count);

        if (*addr_list == NULL) {
                return -ENOMEM;
        }

        for (i = 0; i < *addr_count; i++) {
                addr_obj = cJSON_GetArrayItem(addr_list_obj, i);
		
		if (!codec_get_uint16(addr_obj, JSON_STR_ADDR, &addr)) {
			return -EINVAL;
		}
                
		(*addr_list)[i] = addr;
        }

        return 0;        
}

int codec_encode_subscribe_list(char *buf, size_t buf_len)
{
        int err;
        int i;
        const uint16_t *subscribe_list;
        cJSON *subscribe_list_obj;
        cJSON *event_obj;
        cJSON *addr_list_obj;
        cJSON *addr_obj;

        subscribe_list = btmesh_get_subscribe_list(BTMESH_SUB_TYPE_GATEWAY);

        if (subscribe_list == NULL) {
                return -EINVAL;
        }

        if (!codec_init_event(&subscribe_list_obj, &event_obj, "subscribe_list")) {
                return -ENOMEM;
        }

        addr_list_obj = cJSON_AddArrayToObject(event_obj, JSON_STR_ADDR_LIST);
        err = -ENOMEM;

        if (addr_list_obj == NULL) {
                goto cleanup;
        }

        for (i = 0; i < CONFIG_BT_MESH_GATEWAY_SUB_LIST_LEN; i++) {
                if (subscribe_list[i] == BT_MESH_ADDR_UNASSIGNED) {
                        continue;
                }

                addr_obj = cJSON_CreateObject();

                if (addr_obj == NULL) {
                        goto cleanup;
                }

                cJSON_AddItemToArray(addr_list_obj, addr_obj);

                if (cJSON_AddNumberToObject(addr_obj, JSON_STR_ADDR, subscribe_list[i]) == NULL) {
                        goto cleanup;
                }
        }

        if (!cJSON_PrintPreallocated(subscribe_list_obj, buf, buf_len, 0)) {
                goto cleanup;
        }

        err = 0;

cleanup:
        cJSON_Delete(subscribe_list_obj);
        return err;
}

int codec_parse_model_msg(cJSON *op_obj, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
        int i;
        uint8_t byte;
        uint32_t opcode;
        size_t payload_len;
        cJSON *payload_obj;
        cJSON *byte_obj;

	if (!codec_get_uint16(op_obj, JSON_STR_NET_IDX, &(ctx->net_idx)) ||
	    !codec_get_uint16(op_obj, JSON_STR_APP_IDX, &(ctx->app_idx)) ||
	    !codec_get_uint16(op_obj, JSON_STR_ADDR, &(ctx->addr)) ||
	    !codec_get_uint32(op_obj, JSON_STR_OPCODE, &opcode)) {
		return -EINVAL;
	}
        
	if (opcode & 0xFF0000) {
                net_buf_simple_add_u8(buf, (uint8_t)(opcode >> 16));
                net_buf_simple_add_le16(buf, (uint16_t)opcode);
        } else if (opcode & 0xFF00) {
                net_buf_simple_add_be16(buf, (uint16_t)opcode);
        } else if (opcode & 0xFF) {
                net_buf_simple_add_u8(buf, (uint8_t)opcode);
        } else {
                LOG_DBG("Invalid opcode: 0x%08x", opcode);
                return -EINVAL;
        }
                
        payload_obj = cJSON_GetObjectItem(op_obj, JSON_STR_PAYLOAD);

        if (payload_obj == NULL) {
                return -EINVAL;
        }

        if (!cJSON_IsArray(payload_obj)) {
                return -EINVAL;
        }

        payload_len = cJSON_GetArraySize(payload_obj);

        for (i = 0; i < payload_len; i++) {
                byte_obj = cJSON_GetArrayItem(payload_obj, i);

                if (byte_obj == NULL) {
                        return -ENOMEM;
                }

		if (!codec_get_uint8(byte_obj, JSON_STR_BYTE, &byte)) {
			return -EINVAL;
		}
                
                net_buf_simple_add_u8(buf, byte);
        }

        return 0;
}

int codec_encode_model_msg(char *buf, size_t buf_len, uint32_t opcode, struct bt_mesh_msg_ctx *ctx,
                uint8_t *payload, size_t payload_len)
{
        int err;
        int i;
        cJSON *model_status_obj;
        cJSON *event_obj;
        cJSON *payload_obj;
        cJSON *byte_obj;

        if (!codec_init_event(&model_status_obj, &event_obj, "recieve_model_message")) {
                return -ENOMEM;
        }

        err = -ENOMEM;


        if (cJSON_AddNumberToObject(event_obj, JSON_STR_NET_IDX, ctx->net_idx) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddNumberToObject(event_obj, JSON_STR_APP_IDX, ctx->app_idx) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddNumberToObject(event_obj, JSON_STR_SRC_ADDR, ctx->addr) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddNumberToObject(event_obj, JSON_STR_DST_ADDR, ctx->recv_dst) == NULL) {
                goto cleanup;
        }

        if (cJSON_AddNumberToObject(event_obj, JSON_STR_OPCODE, opcode) == NULL) {
                goto cleanup;
        }

        payload_obj = cJSON_AddArrayToObject(event_obj, JSON_STR_PAYLOAD);

        if (payload_obj == NULL) {
                goto cleanup;
        }

        for (i = 0; i < payload_len; i++) {
                byte_obj = cJSON_CreateObject();

                if (byte_obj == NULL) {
                        goto cleanup;
                }

                cJSON_AddItemToArray(payload_obj, byte_obj);

                if (cJSON_AddNumberToObject(byte_obj, JSON_STR_BYTE, payload[i]) == NULL) {
                        goto cleanup;
                }
        }

        if (!cJSON_PrintPreallocated(model_status_obj, buf, buf_len, 0)) {
                goto cleanup;
        }

        err = 0;

cleanup:
        cJSON_Delete(model_status_obj);
        return err;
}

int codec_parse_hlth_fault(cJSON *op_obj, uint16_t *addr, uint16_t *app_idx, uint16_t *cid)
{
        if (!codec_get_uint16(op_obj, JSON_STR_ADDR, addr) ||
            !codec_get_uint16(op_obj, JSON_STR_APP_IDX, app_idx) ||
            !codec_get_uint16(op_obj, JSON_STR_CID, cid)) {
                return -EINVAL; 
        }

	return 0;
}

int codec_parse_hlth_fault_test(cJSON *op_obj, uint16_t *addr, uint16_t *app_idx, uint16_t *cid,
		uint8_t *test_id)
{
	if (!codec_get_uint8(op_obj, JSON_STR_TEST_ID, test_id)) {
		return -EINVAL;
	}

	return codec_parse_hlth_fault(op_obj, addr, app_idx, cid);
}

static int codec_encode_hlth_faults(char *buf, size_t buf_len, bool current, uint16_t addr,
		uint16_t app_idx, uint16_t cid, uint8_t test_id, uint8_t *faults,
		size_t fault_count)
{
	int err;
	int i;
	cJSON *hlth_faults_obj;
	cJSON *event_obj;
	cJSON *faults_obj;
	cJSON *fault_obj;

	if (current) {
		if (!codec_init_event(&hlth_faults_obj, &event_obj, "health_faults_current")) {
			return -ENOMEM;
		}
	} else {
		if (!codec_init_event(&hlth_faults_obj, &event_obj, "health_faults_registered")) {
			return -ENOMEM;
		}
	}

	err = -ENOMEM;

	if (cJSON_AddNumberToObject(event_obj, JSON_STR_ADDR, addr) == NULL) {
		goto cleanup;
	}

	if (!current) {
		if (cJSON_AddNumberToObject(event_obj, JSON_STR_APP_IDX, app_idx) == NULL) {
			goto cleanup;
		}
	}

	if (cJSON_AddNumberToObject(event_obj, JSON_STR_CID, cid) == NULL) {
		goto cleanup;
	}

	if (cJSON_AddNumberToObject(event_obj, JSON_STR_TEST_ID, test_id) == NULL) {
		goto cleanup;
	}

	faults_obj = cJSON_AddArrayToObject(event_obj, "faults");

	if (faults_obj == NULL) {
		goto cleanup;
	}

	for (i = 0; i < fault_count; i++) {
		fault_obj = cJSON_CreateObject();

		if (fault_obj == NULL) {
			goto cleanup;
		}

		cJSON_AddItemToArray(faults_obj, fault_obj);

		if (cJSON_AddNumberToObject(fault_obj, "fault", faults[i]) == NULL) {
			goto cleanup;
		}
	}

	if (!cJSON_PrintPreallocated(hlth_faults_obj, buf, buf_len, 0)) {
		goto cleanup;
	}

	err = 0;

cleanup:
	cJSON_Delete(hlth_faults_obj);
	return err;
}

int codec_encode_hlth_faults_cur(char *buf, size_t buf_len, uint16_t addr, uint16_t cid,
		uint8_t test_id, uint8_t *faults, size_t fault_count)
{
	return codec_encode_hlth_faults(buf, buf_len, true, addr, 0, cid, test_id, faults,
			fault_count);
}

int codec_encode_hlth_faults_reg(char *buf, size_t buf_len, uint16_t addr, uint16_t app_idx,
		uint16_t cid, uint8_t test_id, uint8_t *faults, size_t fault_count)
{
	return codec_encode_hlth_faults(buf, buf_len, false, addr, app_idx, cid, test_id, faults,
			fault_count);
}

int codec_parse_hlth_period(cJSON *op_obj, uint16_t *addr, uint16_t *app_idx)
{
	if (!codec_get_uint16(op_obj, JSON_STR_ADDR, addr) ||
	    !codec_get_uint16(op_obj, JSON_STR_APP_IDX, app_idx)) {
		return -EINVAL;
	}

	return 0;
}

int codec_parse_hlth_period_set(cJSON *op_obj, uint16_t *addr, uint16_t *app_idx, uint8_t *div)
{
	if (!codec_get_uint8(op_obj, JSON_STR_DIV, div)) {
		return -EINVAL;
	}

	return codec_parse_hlth_period(op_obj, addr, app_idx);
}

int codec_encode_hlth_period(char *buf, size_t buf_len, uint16_t addr, uint8_t div)
{
	int err;
	cJSON *hlth_period_obj;
	cJSON *event_obj;

	if (!codec_init_event(&hlth_period_obj, &event_obj, "health_period")) {
		return -ENOMEM;
	}

	err = -ENOMEM;

	if (cJSON_AddNumberToObject(event_obj, JSON_STR_ADDR, addr) == NULL) {
		goto cleanup;
	}

	if (cJSON_AddNumberToObject(event_obj, JSON_STR_DIV, div) == NULL) {
		goto cleanup;
	}

	if (!cJSON_PrintPreallocated(hlth_period_obj, buf, buf_len, 0)) {
		goto cleanup;
	}

	err = 0;

cleanup:
	cJSON_Delete(hlth_period_obj);
	return err;
}

int codec_parse_hlth_attn(cJSON *op_obj, uint16_t *addr, uint16_t *app_idx)
{
	if (!codec_get_uint16(op_obj, JSON_STR_ADDR, addr) ||
	    !codec_get_uint16(op_obj, JSON_STR_APP_IDX, app_idx)) {
		return -EINVAL;
	}

	return 0;
}

int codec_parse_hlth_attn_set(cJSON *op_obj, uint16_t *addr, uint16_t *app_idx, uint8_t *attn)
{
	if (!codec_get_uint8(op_obj, JSON_STR_ATTN, attn)) {
		return -EINVAL;
	}

	return codec_parse_hlth_attn(op_obj, addr, app_idx);
}

int codec_encode_hlth_attn(char *buf, size_t buf_len, uint16_t addr, uint8_t attn)
{
	int err;
	cJSON *hlth_attn_obj;
	cJSON *event_obj;

	if (!codec_init_event(&hlth_attn_obj, &event_obj, "health_attention")) {
		return -ENOMEM;
	}

	err = -ENOMEM;

	if (cJSON_AddNumberToObject(event_obj, JSON_STR_ADDR, addr) == NULL) {
		goto cleanup;
	}

	if (cJSON_AddNumberToObject(event_obj, JSON_STR_ATTN, attn) == NULL) {
		goto cleanup;
	}

	if (!cJSON_PrintPreallocated(hlth_attn_obj, buf, buf_len, 0)) {
		goto cleanup;
	}

	err = 0;

cleanup:
	cJSON_Delete(hlth_attn_obj);
	return err;
}

int codec_parse_hlth_timeout(cJSON *op_obj, int32_t *timeout)
{
	if (!codec_get_int32(op_obj, JSON_STR_TIMEOUT, timeout)) {
		return -EINVAL;
	}

	return 0;
}

int codec_encode_hlth_timeout(char *buf, size_t buf_len, int32_t timeout)
{
	int err;
	cJSON *hlth_timeout_obj;
	cJSON *event_obj;

	if (!codec_init_event(&hlth_timeout_obj, &event_obj, "health_client_timeout")) {
		return -ENOMEM;
	}

	err = -ENOMEM;

	if (cJSON_AddNumberToObject(event_obj, JSON_STR_TIMEOUT, timeout) == NULL) {
		goto cleanup;
	}

	if (!cJSON_PrintPreallocated(hlth_timeout_obj, buf, buf_len, 0)) {
		goto cleanup;
	}

	err= 0;

cleanup:
	cJSON_Delete(hlth_timeout_obj);
	return err;
}
