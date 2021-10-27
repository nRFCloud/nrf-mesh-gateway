#include <zephyr.h>
#include <logging/log.h>
#include <cJSON.h>
#include <cJSON_os.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>
#include <string.h>

/* Include mesh stack header for direct access to mesh access layer messaging. */
#include "mesh/net.h"
#include "mesh/access.h"
#include "net/nrf_cloud.h"
#include "net/cloud.h"

#include "btmesh.h"
#include "codec.h"
#include "util.h"
#include "nrf_cloud_transport.h"
#include "gateway.h"


#define PROV_TIMEOUT_SEC 60
#define GATEWAY_PROC_THREAD_SLEEP_TIME_MS 500
#define GATEWAY_PROC_THREAD_STACK_SIZE 5120
#define GATEWAY_PROC_THREAD_PRIORITY 5
#define GATEWAY_BUF_LEN 4096
#define ERR_STR "ERROR: "


LOG_MODULE_REGISTER(app_gateway, CONFIG_NRF_CLOUD_MESH_GATEWAY_LOG_LEVEL);

enum gateway_err {
	ERR_BEACON_LIST_ENCODE,
	ERR_PROV_RESP_ENCODE,
	ERR_PROV_PARSE,
	ERR_PROV_RESOURCE,
	ERR_PROV_OP,
	ERR_PROV_TIMEOUT,
	ERR_PROC_DATA_MEM,
	ERR_NODE_LIST_ENCODE,
	ERR_SUBNET_ALLOC,
	ERR_SUBNET_ADD_PARSE,
	ERR_SUBNET_CDB_ADD,
	ERR_SUBNET_LIST_ENCODE,
	ERR_SUBNET_GEN_PARSE,
	ERR_SUBNET_DEL_PARSE,
	ERR_SUBNET_DEL,
	ERR_APP_KEY_ALLOC,
	ERR_APP_KEY_ADD_PARSE,
	ERR_APP_KEY_CDB_ADD,
	ERR_APP_KEY_LIST_ENCODE,
	ERR_APP_KEY_GEN_PARSE,
	ERR_APP_KEY_DEL_PARSE,
	ERR_APP_KEY_DEL,
	ERR_NODE_DISC_PARSE,
	ERR_NODE_DISC_OP,
	ERR_NODE_DISC_ENCODE,
	ERR_NODE_CFG_PARSE,
	ERR_SUB_PARSE,
	ERR_SUB_CHANGE,
	ERR_SUB_ENCODE,
	ERR_MOD_MSG_ENCODE,
	ERR_MOD_MSG_PARSE,
	ERR_MOD_MSG_SEND,
	ERR_HLTH_FAULT_GET_PARSE,
	ERR_HLTH_FAULT_GET_OP,
	ERR_HLTH_FAULT_CUR_ENCODE,
	ERR_HLTH_FAULT_GET_ENCODE,
	ERR_HLTH_FAULT_CLEAR_PARSE,
	ERR_HLTH_FAULT_CLEAR_OP,
	ERR_HLTH_FAULT_CLEAR_ENCODE,
	ERR_HLTH_FAULT_TEST_PARSE,
	ERR_HLTH_FAULT_TEST_OP,
	ERR_HLTH_FAULT_TEST_ENCODE,
	ERR_HLTH_PERIOD_GET_PARSE,
	ERR_HLTH_PERIOD_GET_OP,
	ERR_HLTH_PERIOD_GET_ENCODE,
	ERR_HLTH_PERIOD_SET_PARSE,
	ERR_HLTH_PERIOD_SET_OP,
	ERR_HLTH_PERIOD_SET_ENCODE,
	ERR_HLTH_ATTN_GET_PARSE,
	ERR_HLTH_ATTN_GET_OP,
	ERR_HLTH_ATTN_GET_ENCODE,
	ERR_HLTH_ATTN_SET_PARSE,
	ERR_HLTH_ATTN_SET_OP,
	ERR_HLTH_ATTN_SET_ENCODE,
	ERR_HLTH_TIMEOUT_GET_OP,
	ERR_HLTH_TIMEOUT_GET_ENCODE,
	ERR_HLTH_TIMEOUT_SET_PARSE,
	ERR_HLTH_TIMEOUT_SET_OP,
	ERR_HLTH_TIMEOUT_SET_ENCODE
};

enum gateway_proc {
        GATEWAY_PROC_BEACON_REQ,
        GATEWAY_PROC_PROV,
        GATEWAY_PROC_PROV_RESP,
        GATEWAY_PROC_SUBNET_ADD,
        GATEWAY_PROC_SUBNET_GEN,
        GATEWAY_PROC_SUBNET_DEL,
        GATEWAY_PROC_SUBNET_REQ,
        GATEWAY_PROC_APP_KEY_ADD,
        GATEWAY_PROC_APP_KEY_GEN,
        GATEWAY_PROC_APP_KEY_DEL,
        GATEWAY_PROC_APP_KEY_REQ,
        GATEWAY_PROC_NODE_RESET,
        GATEWAY_PROC_NODE_REQ,
        GATEWAY_PROC_NODE_DISC,
        GATEWAY_PROC_NODE_CFG,
        GATEWAY_PROC_SUBSCRIBE,
        GATEWAY_PROC_UNSUBSCRIBE,
        GATEWAY_PROC_SUBSCRIBE_REQ,
        GATEWAY_PROC_RECV_MODEL_MSG,
#if defined(CONFIG_BT_MESH_ACCESS_LAYER_MSG)
        GATEWAY_PROC_SEND_MODEL_MSG,
#endif // defined(CONFIG_BT_MESH_ACCESS_LAYER_MSG)
	GATEWAY_PROC_HLTH_FAULT_CUR,
	GATEWAY_PROC_HLTH_FAULT_GET,
	GATEWAY_PROC_HLTH_FAULT_CLEAR,
	GATEWAY_PROC_HLTH_FAULT_TEST,
	GATEWAY_PROC_HLTH_PERIOD_GET,
	GATEWAY_PROC_HLTH_PERIOD_SET,
	GATEWAY_PROC_HLTH_ATTN_GET,
	GATEWAY_PROC_HLTH_ATTN_SET,
	GATEWAY_PROC_HLTH_TIMEOUT_GET,
	GATEWAY_PROC_HLTH_TIMEOUT_SET
};

struct gateway_proc_data {
        void *fifo_reserved;
        enum gateway_proc proc;
        cJSON *root_obj;
        cJSON *op_obj;
        uint32_t opcode;
        struct bt_mesh_msg_ctx msg_ctx;
        uint8_t *payload;
        size_t payload_len;
	uint16_t addr;
	uint8_t test_id;
	uint16_t cid;
	uint8_t *faults;
	size_t fault_count;
};

K_FIFO_DEFINE(gateway_proc_fifo);

static struct k_work_q *work_q;
static struct k_work_delayable prov_timeout_work;
static struct {
        int err;
        uint8_t uuid[UUID_LEN];
        uint16_t net_idx;
        uint16_t addr;
        uint8_t num_elem;
} prov;

K_SEM_DEFINE(prov_sem, 1, 1);

static char buf[GATEWAY_BUF_LEN];

static void log_err(enum gateway_err loc, int err)
{
	LOG_ERR("Error at %d: %d", -loc, err);
}

static bool strings_equal(const char *s1, const  char *s2)
{
        return !strcmp(s1, s2);
}

static int g2c_send(char *buf)
{
	struct nrf_cloud_tx_data msg;

	msg.data.ptr = (const void *)buf;
	msg.data.len = strlen(buf);
	msg.topic_type = NRF_CLOUD_TOPIC_MESSAGE;
	msg.qos = MQTT_QOS_1_AT_LEAST_ONCE;

	return nrf_cloud_send(&msg);
}

static void beacon_req(void)
{
        int err;

        err = codec_encode_beacon_list(buf, sizeof(buf));

        if (err) {
		log_err(ERR_BEACON_LIST_ENCODE, err);
                return;
        }

        g2c_send(buf);
}

static void prov_result(int err, uint8_t uuid[UUID_LEN], uint16_t net_idx, uint16_t addr,
                uint8_t num_elem)
{
        int codec_err;

        codec_err = codec_encode_prov_result(buf, sizeof(buf), err, net_idx, uuid, addr, num_elem);

        if (codec_err) {
		log_err(ERR_PROV_RESP_ENCODE, codec_err);
                goto cleanup;
        }

        g2c_send(buf);

cleanup:
        k_sem_give(&prov_sem);
}


static void prov_dev(cJSON *op_obj)
{
        int err;
        union btmesh_op_args args;

	err = codec_parse_prov(op_obj, args.prov_adv.uuid, &args.prov_adv.net_idx,
			&args.prov_adv.addr, &args.prov_adv.attn);

	if (err) {
		log_err(ERR_PROV_PARSE, err);
		return;
	}

        if (k_sem_take(&prov_sem, K_SECONDS(PROV_TIMEOUT_SEC))) {
		log_err(ERR_PROV_RESOURCE, 0);
                prov_result(-EDEADLK, args.prov_adv.uuid, args.prov_adv.net_idx, args.prov_adv.addr,
                                args.prov_adv.attn);
                return;
        }

        err = btmesh_perform_op(BTMESH_OP_PROV_ADV, &args);

        if (err) {
		log_err(ERR_PROV_OP, err);
                prov_result(err, args.prov_adv.uuid, args.prov_adv.net_idx, args.prov_adv.addr,
                                args.prov_adv.attn);
                return;
        }

        util_uuid_cpy(prov.uuid, args.prov_adv.uuid);
        k_work_reschedule_for_queue(work_q, &prov_timeout_work, K_SECONDS(PROV_TIMEOUT_SEC));
        LOG_DBG("DONE INITIATING LTE PROVISION");
}

static void prov_timeout(struct k_work *work)
{
        char uuid_str[UUID_STR_LEN];
        struct gateway_proc_data proc_data;
        struct gateway_proc_data *mem_ptr;

        util_uuid2str(prov.uuid,  uuid_str);
	log_err(ERR_PROV_TIMEOUT, 0);
        LOG_ERR("  -> %s", log_strdup(uuid_str));

        proc_data.proc = GATEWAY_PROC_PROV_RESP;
        prov.err = -ETIMEDOUT;
        prov.net_idx = 0;
        prov.addr = 0;
        prov.num_elem = 0;

        mem_ptr = k_malloc(sizeof(proc_data));

        if (mem_ptr == NULL) {
		log_err(ERR_PROC_DATA_MEM, 0);
                k_sem_give(&prov_sem);
                return;
        }

        memcpy(mem_ptr, &proc_data, sizeof(proc_data));
        k_fifo_put(&gateway_proc_fifo, mem_ptr);
}

static void node_req(void)
{
        int err;

        err = codec_encode_node_list(buf, sizeof(buf));

        if (err) {
		log_err(ERR_NODE_LIST_ENCODE, err);
                return;
        }

        g2c_send(buf);
}

static bool add_subnet(uint16_t net_idx, uint8_t net_key[KEY_LEN])
{
        uint8_t err;
        struct bt_mesh_cdb_subnet *subnet;

        subnet = bt_mesh_cdb_subnet_alloc(net_idx);

        if (subnet == NULL) {
		log_err(ERR_SUBNET_ALLOC, 0);
                LOG_ERR("  -> 0x%04x", net_idx);
                return false;
        }

        memcpy(subnet->keys[0].net_key, net_key, KEY_LEN);
        bt_mesh_cdb_subnet_store(subnet);
        
        err = bt_mesh_subnet_add(net_idx, net_key);

        if (err) {
                bt_mesh_cdb_subnet_del(subnet, true); 
                return false;
        }

        return true;
}

static void subnet_add(cJSON *op_obj)
{
        int err;
        uint16_t net_idx;
        uint8_t net_key[KEY_LEN];

	err = codec_parse_subnet_add(op_obj, &net_idx, net_key);

	if (err) {
		log_err(ERR_SUBNET_ADD_PARSE, err);
		return;
	}

        if (!add_subnet(net_idx, net_key)) {
		log_err(ERR_SUBNET_CDB_ADD, 0);
                return;
        }

        err = codec_encode_subnet_list(buf, sizeof(buf));

        if (err) {
		log_err(ERR_SUBNET_LIST_ENCODE, err);
                return;
        }

        g2c_send(buf);
}

static void subnet_gen(cJSON *op_obj)
{
        int err;
        uint16_t net_idx;
        uint8_t net_key[KEY_LEN];

	err = codec_parse_subnet(op_obj, &net_idx);

	if (err) {
		log_err(ERR_SUBNET_GEN_PARSE, err);
		return;
	}

        bt_rand(net_key, KEY_LEN);

        if (!add_subnet(net_idx, net_key)) {
		log_err(ERR_SUBNET_CDB_ADD, 0);
                return;
        }

        err = codec_encode_subnet_list(buf, sizeof(buf));

        if (err) {
		log_err(ERR_SUBNET_LIST_ENCODE, err);
                return;
        }

        g2c_send(buf);
}

static void subnet_del(cJSON *op_obj)
{
        int err;
        uint16_t net_idx;
        struct bt_mesh_cdb_subnet *subnet;

	err = codec_parse_subnet(op_obj, &net_idx);

	if (err) {
		log_err(ERR_SUBNET_DEL_PARSE, err);
		return;
	}

        subnet = bt_mesh_cdb_subnet_get(net_idx);

        if (subnet == NULL) {
		log_err(ERR_SUBNET_DEL, 0);
                return;
        }

        bt_mesh_cdb_subnet_del(subnet, true);
        bt_mesh_subnet_del(net_idx);
        err = codec_encode_subnet_list(buf, sizeof(buf));

        if (err) {
		log_err(ERR_SUBNET_LIST_ENCODE, err);
                return;
        }

        g2c_send(buf);
}

static void subnet_req(void)
{
        int err;

        err = codec_encode_subnet_list(buf, sizeof(buf));

        if (err) {
		log_err(ERR_SUBNET_LIST_ENCODE, err);
                return;
        }

        g2c_send(buf);
}

static bool add_app_key(uint16_t net_idx, uint16_t app_idx, uint8_t app_key[KEY_LEN])
{
        uint8_t err;
        struct bt_mesh_cdb_app_key *app_key_ptr;

        app_key_ptr = bt_mesh_cdb_app_key_alloc(net_idx, app_idx);

        if (app_key_ptr == NULL) {
		log_err(ERR_APP_KEY_ALLOC, 0);
                LOG_ERR("  -> 0x%04x", app_idx);
                return false;
        }

        memcpy(app_key_ptr->keys[0].app_key, app_key, KEY_LEN);
        bt_mesh_cdb_app_key_store(app_key_ptr);
        err = bt_mesh_app_key_add(app_idx, net_idx, app_key);
        
        if (err) {
                bt_mesh_cdb_app_key_del(app_key_ptr, true); 
                /* CDB sets the net_idx to BT_MESH_ADDR_UNASSIGNED which is not the same
                 * as the default state. Set net_idx to default state manually so we can
                 * detect that the app_key storage location is free to use for new keys. */
                app_key_ptr->net_idx = BT_MESH_KEY_UNUSED;
                return false;
        }

        return true;
}

static void app_key_add(cJSON *op_obj)
{
        int err;
        uint16_t net_idx;
        uint16_t app_idx;
        uint8_t app_key[KEY_LEN];

	err = codec_parse_app_key_add(op_obj, &net_idx, &app_idx, app_key);

	if (err) {
		log_err(ERR_APP_KEY_ADD_PARSE, err);
		return;
	}

        if (!add_app_key(net_idx, app_idx, app_key)) {
		log_err(ERR_APP_KEY_CDB_ADD, 0);
                return;
        }

        err = codec_encode_app_key_list(buf, sizeof(buf));

        if (err) {
		log_err(ERR_APP_KEY_LIST_ENCODE, err);
                return;
        }

        g2c_send(buf);
}

static void app_key_gen(cJSON *op_obj)
{
        int err;
        uint16_t net_idx;
        uint16_t app_idx;
        uint8_t app_key[KEY_LEN];

	err = codec_parse_app_key(op_obj, &net_idx, &app_idx);

	if (err) {
		log_err(ERR_APP_KEY_GEN_PARSE, err);
		return;
	}

        bt_rand(app_key, KEY_LEN);

        if (!add_app_key(net_idx, app_idx, app_key)) {
		log_err(ERR_APP_KEY_CDB_ADD, 0);
                return;
        }

        err = codec_encode_app_key_list(buf, sizeof(buf));

        if (err) {
		log_err(ERR_APP_KEY_LIST_ENCODE, 0);
                return;
        }

        g2c_send(buf);
}

static void app_key_del(cJSON *op_obj)
{
        int err;
        uint16_t net_idx;
        uint16_t app_idx;
        struct bt_mesh_cdb_app_key *app_key;

	err = codec_parse_app_key(op_obj, &net_idx, &app_idx);

	if (err) {
		log_err(ERR_APP_KEY_DEL_PARSE, err);
		return;
	}

        app_key = bt_mesh_cdb_app_key_get(app_idx);

        if (app_key == NULL) {
		log_err(ERR_APP_KEY_DEL, 0);
                return;
        }

        net_idx = app_key->net_idx;
        bt_mesh_cdb_app_key_del(app_key, true);
        app_key->net_idx = BT_MESH_KEY_UNUSED;
        bt_mesh_app_key_del(app_idx, net_idx);
        err = codec_encode_app_key_list(buf, sizeof(buf));

        if (err) {
		log_err(ERR_APP_KEY_LIST_ENCODE, 0);
                return;
        }

        g2c_send(buf);
}

static void app_key_req(void)
{
        int err;

        err = codec_encode_app_key_list(buf, sizeof(buf));

        if (err) {
		log_err(ERR_APP_KEY_LIST_ENCODE, 0);
                return;
        }

        g2c_send(buf);
}

static void node_disc(cJSON *op_obj)
{
        int err;
        uint8_t status;
        struct btmesh_node node;

	err = codec_parse_node_disc(op_obj, &node.addr);

	if (err) {
		log_err(ERR_NODE_DISC_PARSE, err);
		return;
	}

        err = btmesh_discover_node(&node, &status);

        if (err) {
		log_err(ERR_NODE_DISC_OP, err);
                return;
        }

        err = codec_encode_node_disc(buf, sizeof(buf), &node, err, status);

        if (err) {
		log_err(ERR_NODE_DISC_ENCODE, err);
                return;
        }

        g2c_send(buf);
}

static void node_cfg(cJSON *op_obj)
{
        int err;
        uint8_t status;
        struct btmesh_node node;

        err = codec_parse_node_cfg(op_obj, &node.addr);

        if (err) {
		log_err(ERR_NODE_CFG_PARSE, 0);
        }

        err = btmesh_discover_node(&node, &status);

        if (err) {
		log_err(ERR_NODE_DISC_OP, err);
                return;
        }

        err = codec_encode_node_disc(buf, sizeof(buf), &node, err, status);

        if (err) {
		log_err(ERR_NODE_DISC_ENCODE, err);
                return;
        }

        g2c_send(buf);
}

static void change_subscribe_list(cJSON *op_obj, bool subscribe)
{
        int i;
        int err;
        int addr_count;
        uint16_t *addr_list;

        addr_list = NULL;
        err = codec_parse_subscribe_addrs(op_obj, &addr_list, &addr_count);

        if (err) {
		log_err(ERR_SUB_PARSE, err);
                return;
        }

        for (i = 0; i < addr_count; i++) {
                if (subscribe) {
                        err = btmesh_subscribe(BTMESH_SUB_TYPE_GATEWAY, addr_list[i]);
                } else {
                        btmesh_unsubscribe(BTMESH_SUB_TYPE_GATEWAY, addr_list[i]);
                }

                if (err) {
			log_err(ERR_SUB_CHANGE, err);
                        goto cleanup;
                }
        }

        err = codec_encode_subscribe_list(buf, sizeof(buf));

        if (err) {
		log_err(ERR_SUB_ENCODE, err);
                goto cleanup;
        }

        g2c_send(buf);

cleanup:
        k_free(addr_list);
}

static void subscribe(cJSON *op_obj)
{
        change_subscribe_list(op_obj, true);
}

static void unsubscribe(cJSON *op_obj)
{
        change_subscribe_list(op_obj, false);
}

static void subscribe_req(cJSON *op_obj)
{
        int err;

        err = codec_encode_subscribe_list(buf, sizeof(buf));

        if (err) {
		log_err(ERR_SUB_ENCODE, err);
                return;
        }

        g2c_send(buf);
}

static void recv_model_msg(uint32_t opcode, struct bt_mesh_msg_ctx ctx, uint8_t *payload,
                size_t payload_len)
{
        int err;

        err = codec_encode_model_msg(buf, sizeof(buf), opcode, &ctx, payload, payload_len);

        if (err) {
		log_err(ERR_MOD_MSG_ENCODE, err);
                return;
        }

        g2c_send(buf);
}

#if defined(CONFIG_BT_MESH_ACCESS_LAYER_MSG)
static void send_model_msg(cJSON *op_obj)
{
        int err;
        struct bt_mesh_msg_ctx ctx;

        NET_BUF_SIMPLE_DEFINE(buf, 32);

        err = codec_parse_model_msg(op_obj, &ctx, &buf);

        if (err) {
		log_err(ERR_MOD_MSG_PARSE, err);
                return;
        }

        ctx.send_rel = false;
        ctx.send_ttl = BT_MESH_TTL_DEFAULT;
        err = bt_mesh_msg_send(&ctx, &buf, bt_mesh_primary_addr(), NULL, NULL);

        if (err) {
		log_err(ERR_MOD_MSG_SEND, err);
        }
}
#endif // defined(CONFIG_BT_MESH_ACCESS_LAYER_MSG)

static void hlth_fault_cur(uint16_t addr, uint8_t test_id, uint16_t cid, uint8_t *faults,
		size_t fault_count)
{
	int err;

	err = codec_encode_hlth_faults_cur(buf, sizeof(buf), addr, cid, test_id, faults,
			fault_count);

	if (err) {
		log_err(ERR_HLTH_FAULT_CUR_ENCODE, err);
		return;
	}

	g2c_send(buf);
}

static void hlth_fault_get(cJSON *op_obj)
{
	int err;
	union btmesh_op_args op_args;

	err = codec_parse_hlth_fault(op_obj, &op_args.hlth_fault_get.addr,
			&op_args.hlth_fault_get.app_idx, &op_args.hlth_fault_get.cid);

	if (err) {
		log_err(ERR_HLTH_FAULT_GET_PARSE, err);
		return;
	}

	err = btmesh_perform_op(BTMESH_OP_HLTH_FAULT_GET, &op_args);

	if (err) {
		log_err(ERR_HLTH_FAULT_GET_OP, err);
		return;
	}

	err = codec_encode_hlth_faults_reg(buf, sizeof(buf), op_args.hlth_fault_get.addr,
			op_args.hlth_fault_get.app_idx, op_args.hlth_fault_get.cid,
			op_args.hlth_fault_get.test_id, op_args.hlth_fault_get.faults,
			op_args.hlth_fault_get.fault_count);

	if (err) {
		log_err(ERR_HLTH_FAULT_GET_ENCODE, err);
		return;
	}

	g2c_send(buf);
}

static void hlth_fault_clear(cJSON *op_obj)
{
	int err;
	union btmesh_op_args op_args;

	err = codec_parse_hlth_fault(op_obj, &op_args.hlth_fault_clear.addr,
			&op_args.hlth_fault_clear.app_idx, &op_args.hlth_fault_clear.cid);

	if (err) {
		log_err(ERR_HLTH_FAULT_CLEAR_PARSE, err);
		return;
	}

	err = btmesh_perform_op(BTMESH_OP_HLTH_FAULT_CLR, &op_args);

	if (err) {
		log_err(ERR_HLTH_FAULT_CLEAR_OP, err);
		return;
	}

	err = codec_encode_hlth_faults_reg(buf, sizeof(buf), op_args.hlth_fault_clear.addr,
			op_args.hlth_fault_clear.app_idx, op_args.hlth_fault_clear.cid,
			op_args.hlth_fault_clear.test_id, op_args.hlth_fault_clear.faults,
			op_args.hlth_fault_clear.fault_count);

	if (err) {
		log_err(ERR_HLTH_FAULT_CLEAR_ENCODE, err);
		return;
	}

	g2c_send(buf);
}

static void hlth_fault_test(cJSON *op_obj)
{
	int err;
	union btmesh_op_args op_args;

	err = codec_parse_hlth_fault_test(op_obj, &op_args.hlth_fault_test.addr,
			&op_args.hlth_fault_test.app_idx, &op_args.hlth_fault_test.cid,
			&op_args.hlth_fault_test.test_id);
	
	if (err) {
		log_err(ERR_HLTH_FAULT_TEST_PARSE, err);
		return;
	}

	err = btmesh_perform_op(BTMESH_OP_HLTH_FAULT_TEST, &op_args);

	if (err) {
		log_err(ERR_HLTH_FAULT_TEST_OP, err);
		return;
	}

	err = codec_encode_hlth_faults_reg(buf, sizeof(buf), op_args.hlth_fault_clear.addr,
			op_args.hlth_fault_clear.app_idx, op_args.hlth_fault_clear.cid,
			op_args.hlth_fault_clear.test_id, op_args.hlth_fault_clear.faults,
			op_args.hlth_fault_clear.fault_count);

	if (err) {
		log_err(ERR_HLTH_FAULT_TEST_ENCODE, err);
		return;
	}

	g2c_send(buf);
}

static void hlth_period_get(cJSON *op_obj)
{
	int err;
	union btmesh_op_args op_args;

	err = codec_parse_hlth_period(op_obj, &op_args.hlth_period_get.addr,
			&op_args.hlth_period_get.app_idx);

	if (err) {
		log_err(ERR_HLTH_PERIOD_GET_PARSE, err);
		return;
	}

	err = btmesh_perform_op(BTMESH_OP_HLTH_PERIOD_GET, &op_args);

	if (err) {
		log_err(ERR_HLTH_PERIOD_GET_OP, err);
		return;
	}

	err = codec_encode_hlth_period(buf, sizeof(buf), op_args.hlth_period_get.addr,
			op_args.hlth_period_get.divisor);

	if (err) {
		log_err(ERR_HLTH_PERIOD_GET_ENCODE, err);
		return;
	}
	
	g2c_send(buf);
}

static void hlth_period_set(cJSON *op_obj)
{
	int err;
	union btmesh_op_args op_args;

	err = codec_parse_hlth_period_set(op_obj, &op_args.hlth_period_set.addr,
			&op_args.hlth_period_set.app_idx, &op_args.hlth_period_set.divisor);

	if (err) {
		log_err(ERR_HLTH_PERIOD_SET_PARSE, err);
		return;
	}

	err = btmesh_perform_op(BTMESH_OP_HLTH_PERIOD_SET, &op_args);

	if (err) {
		log_err(ERR_HLTH_PERIOD_SET_OP, err);
		return;
	}

	err = codec_encode_hlth_period(buf, sizeof(buf), op_args.hlth_period_set.addr,
			op_args.hlth_period_set.updated_divisor);

	if (err) {
		log_err(ERR_HLTH_PERIOD_SET_ENCODE, err);
		return;
	}

	g2c_send(buf);
}

static void hlth_attn_get(cJSON *op_obj)
{
	int err;
	union btmesh_op_args op_args;

	err = codec_parse_hlth_attn(op_obj, &op_args.hlth_attn_get.addr,
			&op_args.hlth_attn_get.app_idx);
	
	if (err) {
		log_err(ERR_HLTH_ATTN_GET_PARSE, err);
		return;
	}

	err = btmesh_perform_op(BTMESH_OP_HLTH_ATTN_GET, &op_args);

	if (err) {
		log_err(ERR_HLTH_ATTN_GET_OP, err);
		return;
	}

	err = codec_encode_hlth_attn(buf, sizeof(buf), op_args.hlth_attn_get.addr,
			op_args.hlth_attn_get.attn);
	
	if (err) {
		log_err(ERR_HLTH_ATTN_GET_ENCODE, err);
		return;
	}

	g2c_send(buf);
}

static void hlth_attn_set(cJSON *op_obj)
{
	int err;
	union btmesh_op_args op_args;

	err = codec_parse_hlth_attn_set(op_obj, &op_args.hlth_attn_set.addr,
			&op_args.hlth_attn_set.app_idx, &op_args.hlth_attn_set.attn);
	
	if (err) {
		log_err(ERR_HLTH_ATTN_SET_PARSE, err);
		return;
	}

	err = btmesh_perform_op(BTMESH_OP_HLTH_ATTN_SET, &op_args);

	if (err) {
		log_err(ERR_HLTH_ATTN_SET_OP, err);
		return;
	}

	err = codec_encode_hlth_attn(buf, sizeof(buf), op_args.hlth_attn_set.addr,
			op_args.hlth_attn_set.updated_attn);
	
	if (err) {
		log_err(ERR_HLTH_ATTN_SET_ENCODE, err);
		return;
	}

	g2c_send(buf);
}

static void hlth_timeout_get(void)
{
	int err;
	union btmesh_op_args op_args;

	err = btmesh_perform_op(BTMESH_OP_HLTH_TIMEOUT_GET, &op_args);

	if (err) {
		log_err(ERR_HLTH_TIMEOUT_GET_OP, err);
		return;
	}

	err = codec_encode_hlth_timeout(buf, sizeof(buf), op_args.hlth_timeout_get.timeout);
	
	if (err) {
		log_err(ERR_HLTH_TIMEOUT_GET_ENCODE, err);
		return;
	}

	g2c_send(buf);
}

static void hlth_timeout_set(cJSON *op_obj)
{
	int err;
	union btmesh_op_args op_args;

	err = codec_parse_hlth_timeout(op_obj, &op_args.hlth_timeout_set.timeout);
	
	if (err) {
		log_err(ERR_HLTH_TIMEOUT_SET_PARSE, err);
		return;
	}

	err = btmesh_perform_op(BTMESH_OP_HLTH_TIMEOUT_SET, &op_args);

	if (err) {
		log_err(ERR_HLTH_TIMEOUT_SET_OP, err);
		return;
	}

	err = codec_encode_hlth_timeout(buf, sizeof(buf), op_args.hlth_timeout_set.timeout);
	
	if (err) {
		log_err(ERR_HLTH_TIMEOUT_SET_ENCODE, err);
		return;
	}

	g2c_send(buf);
}

static void log_proc(enum gateway_proc proc)
{
	LOG_DBG("Gateway procedure: %d", proc);
}

static void gateway_process(int unused1, int unused2, int unused3)
{
        ARG_UNUSED(unused1);
        ARG_UNUSED(unused2);
        ARG_UNUSED(unused3);

        struct gateway_proc_data *proc_data;

        for(;;) {
                k_sleep(K_MSEC(GATEWAY_PROC_THREAD_SLEEP_TIME_MS));
                proc_data = k_fifo_get(&gateway_proc_fifo, K_NO_WAIT);

                if (proc_data == NULL) {
                        continue;
                }

                switch (proc_data->proc) {
                        case GATEWAY_PROC_BEACON_REQ:
                                log_proc(GATEWAY_PROC_BEACON_REQ);
                                beacon_req();
                                break;

                        case GATEWAY_PROC_PROV:
                                log_proc(GATEWAY_PROC_PROV);
                                prov_dev(proc_data->op_obj);
                                break;

                        case GATEWAY_PROC_PROV_RESP:
                                log_proc(GATEWAY_PROC_PROV_RESP);
                                prov_result(prov.err, prov.uuid, prov.net_idx, prov.addr,
                                                prov.num_elem);
                                break;

                        case GATEWAY_PROC_SUBNET_ADD:
                                log_proc(GATEWAY_PROC_SUBNET_ADD);
                                subnet_add(proc_data->op_obj);
                                break;

                        case GATEWAY_PROC_SUBNET_GEN:
                                log_proc(GATEWAY_PROC_SUBNET_GEN);
                                subnet_gen(proc_data->op_obj);
                                break;

                        case GATEWAY_PROC_SUBNET_DEL:
                                log_proc(GATEWAY_PROC_SUBNET_DEL);
                                subnet_del(proc_data->op_obj);
                                break;

                        case GATEWAY_PROC_SUBNET_REQ:
                                log_proc(GATEWAY_PROC_SUBNET_REQ);
                                subnet_req();
                                break;

                        case GATEWAY_PROC_APP_KEY_ADD:
                                log_proc(GATEWAY_PROC_APP_KEY_ADD);
                                app_key_add(proc_data->op_obj);
                                break;

                        case GATEWAY_PROC_APP_KEY_GEN:
                                log_proc(GATEWAY_PROC_APP_KEY_GEN);
                                app_key_gen(proc_data->op_obj);
                                break;

                        case GATEWAY_PROC_APP_KEY_DEL:
                                log_proc(GATEWAY_PROC_APP_KEY_DEL);
                                app_key_del(proc_data->op_obj);
                                break;

                        case GATEWAY_PROC_APP_KEY_REQ:
                                log_proc(GATEWAY_PROC_APP_KEY_REQ);
                                app_key_req();
                                break;

                        case GATEWAY_PROC_NODE_RESET:
                                log_proc(GATEWAY_PROC_NODE_RESET);
                                break;

                        case GATEWAY_PROC_NODE_REQ:
                                log_proc(GATEWAY_PROC_NODE_REQ);
                                node_req();
                                break;

                        case GATEWAY_PROC_NODE_DISC:
                                log_proc(GATEWAY_PROC_NODE_DISC);
                                node_disc(proc_data->op_obj);
                                break;

                        case GATEWAY_PROC_NODE_CFG:
                                log_proc(GATEWAY_PROC_NODE_CFG);
                                node_cfg(proc_data->op_obj);
                                break;

                        case GATEWAY_PROC_SUBSCRIBE:
                                log_proc(GATEWAY_PROC_SUBSCRIBE);
                                subscribe(proc_data->op_obj);
                                break;

                        case GATEWAY_PROC_UNSUBSCRIBE:
                                log_proc(GATEWAY_PROC_UNSUBSCRIBE);
                                unsubscribe(proc_data->op_obj);
                                break;

                        case GATEWAY_PROC_SUBSCRIBE_REQ:
                                log_proc(GATEWAY_PROC_SUBSCRIBE_REQ);
                                subscribe_req(proc_data->op_obj);
                                break;

                        case GATEWAY_PROC_RECV_MODEL_MSG:
                                log_proc(GATEWAY_PROC_RECV_MODEL_MSG);
                                recv_model_msg(proc_data->opcode, proc_data->msg_ctx,
                                                proc_data->payload, proc_data->payload_len);
                                k_free(proc_data->payload);
                                break;

#if defined(CONFIG_BT_MESH_ACCESS_LAYER_MSG)
                        case GATEWAY_PROC_SEND_MODEL_MSG:
                                log_proc(GATEWAY_PROC_SEND_MODEL_MSG);
                                send_model_msg(proc_data->op_obj);
                                break;
#endif // defined(CONFIG_BT_MESH_ACCESS_LAYER_MSG)

			case GATEWAY_PROC_HLTH_FAULT_CUR:
				log_proc(GATEWAY_PROC_HLTH_FAULT_CUR);
				hlth_fault_cur(proc_data->addr, proc_data->test_id, proc_data->cid,
						proc_data->faults, proc_data->fault_count);
				k_free(proc_data->faults);
				break;

			case GATEWAY_PROC_HLTH_FAULT_GET:
				log_proc(GATEWAY_PROC_HLTH_FAULT_GET);
				hlth_fault_get(proc_data->op_obj);
				break;

			case GATEWAY_PROC_HLTH_FAULT_CLEAR:
				log_proc(GATEWAY_PROC_HLTH_FAULT_CLEAR);
				hlth_fault_clear(proc_data->op_obj);
				break;

			case GATEWAY_PROC_HLTH_FAULT_TEST:
				log_proc(GATEWAY_PROC_HLTH_FAULT_TEST);
				hlth_fault_test(proc_data->op_obj);
				break;

			case GATEWAY_PROC_HLTH_PERIOD_GET:
				log_proc(GATEWAY_PROC_HLTH_PERIOD_GET);
				hlth_period_get(proc_data->op_obj);
				break;

			case GATEWAY_PROC_HLTH_PERIOD_SET:
				log_proc(GATEWAY_PROC_HLTH_PERIOD_SET);
				hlth_period_set(proc_data->op_obj);
				break;

			case GATEWAY_PROC_HLTH_ATTN_GET:
				log_proc(GATEWAY_PROC_HLTH_ATTN_GET);
				hlth_attn_get(proc_data->op_obj);
				break;

			case GATEWAY_PROC_HLTH_ATTN_SET:
				log_proc(GATEWAY_PROC_HLTH_ATTN_SET);
				hlth_attn_set(proc_data->op_obj);
				break;

			case GATEWAY_PROC_HLTH_TIMEOUT_GET:
				log_proc(GATEWAY_PROC_HLTH_TIMEOUT_GET);
				hlth_timeout_get();
				break;

			case GATEWAY_PROC_HLTH_TIMEOUT_SET:
				log_proc(GATEWAY_PROC_HLTH_TIMEOUT_SET);
				hlth_timeout_set(proc_data->op_obj);
				break;

                        default:
                                LOG_ERR("Unkown gateway process type: %d", proc_data->proc);
                                break;
                }

                cJSON_Delete(proc_data->root_obj);
                k_free(proc_data);
        }
}

K_THREAD_DEFINE(gateway_proc_thread, GATEWAY_PROC_THREAD_STACK_SIZE,
                gateway_process, NULL, NULL, NULL, GATEWAY_PROC_THREAD_PRIORITY, 0, 0);

void gateway_node_added(uint16_t net_idx, uint8_t uuid[UUID_LEN], uint16_t addr, uint8_t num_elem)
{
        char uuid_str[UUID_STR_LEN];
        struct gateway_proc_data proc_data;
        struct gateway_proc_data *mem_ptr;

        /* Check to see if this is the callback from a gateway provision attempt */
        if (util_uuid_cmp(prov.uuid, uuid)) {
                return;
        }

        k_work_cancel_delayable(&prov_timeout_work);
        util_uuid2str(uuid, uuid_str);
        LOG_INF("Gateway request to provision complete:");
        LOG_INF("  UUID         : %s", log_strdup(uuid_str));
        LOG_INF("  Net Index     : 0x%04x", net_idx);
        LOG_INF("  Address      : 0x%04x", addr);
        LOG_INF("  Element Count: %d", num_elem);

        prov_result(0, uuid, net_idx, addr, num_elem); // TODO: Fix this
        return;

        proc_data.proc = GATEWAY_PROC_PROV_RESP;
        prov.net_idx = net_idx;
        prov.addr = addr;
        prov.num_elem = num_elem;

        mem_ptr = k_malloc(sizeof(proc_data));

        if (mem_ptr == NULL) {
                LOG_ERR("Failed to allocate memory for process data");
                k_sem_give(&prov_sem);
                return;
        }

        memcpy(mem_ptr, &proc_data, sizeof(proc_data));
        k_fifo_put(&gateway_proc_fifo, mem_ptr);
}

void gateway_msg_callback(uint32_t opcode, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
        uint8_t *payload;
        struct gateway_proc_data proc_data;
        struct gateway_proc_data *proc_ptr;

        payload = k_malloc(sizeof(uint8_t) * buf->len);
        memcpy(payload, buf->data, buf->len);

        proc_data.proc = GATEWAY_PROC_RECV_MODEL_MSG;
        proc_data.root_obj = NULL;
        proc_data.opcode = opcode;
        proc_data.msg_ctx.net_idx= ctx->net_idx;
        proc_data.msg_ctx.app_idx = ctx->app_idx;
        proc_data.msg_ctx.addr = ctx->addr;
        proc_data.msg_ctx.recv_dst = ctx->recv_dst;
        proc_data.payload = payload;
        proc_data.payload_len = buf->len;

        proc_ptr = k_malloc(sizeof(proc_data));
        memcpy(proc_ptr, &proc_data, sizeof(proc_data));
        k_fifo_put(&gateway_proc_fifo, proc_ptr);
}

void gateway_hlth_cb(uint16_t addr, uint8_t test_id, uint16_t cid, uint8_t *faults,
		size_t fault_count)
{
	uint8_t *faults_store;
	struct gateway_proc_data proc_data;
	struct gateway_proc_data *proc_ptr;

	faults_store = k_malloc(sizeof(uint8_t) * fault_count);
	memcpy(faults_store, faults, fault_count);

	proc_data.proc = GATEWAY_PROC_HLTH_FAULT_CUR;
	proc_data.root_obj = NULL;
	proc_data.addr = addr;
	proc_data.test_id = test_id;
	proc_data.cid = cid;
	proc_data.faults = faults_store;
	proc_data.fault_count = fault_count;

	proc_ptr = k_malloc(sizeof(proc_data));
	memcpy(proc_ptr, &proc_data, sizeof(proc_data));
	k_fifo_put(&gateway_proc_fifo, proc_ptr);
}

enum gateway_handler_err {
	HANDLER_ERR_JSON_PARSE,
	HANDLER_ERR_TYPE_OBJ,
	HANDLER_ERR_TYPE_STR,
	HANDLER_ERR_OP,
	HANDLER_ERR_OP_OBJ,
	HANDLER_ERR_OP_TYPE_OBJ,
	HANDLER_ERR_OP_TYPE_STR,
	HANDLER_ERR_PROC_DATA,
	HANDLER_ERR_UNKOWN_OP_TYPE
};

static void log_handler_err(enum gateway_handler_err err)
{
	LOG_ERR("Gateway Handler Error: %d", -err);
}

static void log_handler_proc(enum gateway_proc proc)
{
	LOG_DBG("Gateway Handler Procedure: %d", proc);
}

uint8_t gateway_handler(const struct cloud_msg *gw_data)
{
        int err;
        char *type_str;
        char *op_type_str;
        cJSON *root_obj;
        cJSON *type_obj;
        cJSON *op_obj;
        cJSON *op_type_obj;
        struct gateway_proc_data proc_data;
        struct gateway_proc_data *mem_ptr;

	LOG_DBG("Cloud message len:%d, topic:%s, data:%s",
		gw_data->len,
		log_strdup(gw_data->endpoint.str),
		log_strdup(gw_data->buf));

	if (strstr(gw_data->endpoint.str, "shadow") != NULL) {
		LOG_DBG("Ignoring shadow changes");
		return 0;
	}

        root_obj = cJSON_Parse(gw_data->buf);

        if (root_obj == NULL) {
		log_handler_err(HANDLER_ERR_JSON_PARSE);
                return -ENOENT;
        }

        type_obj = cJSON_GetObjectItem(root_obj, "type");

        if (type_obj == NULL) {
		log_handler_err(HANDLER_ERR_TYPE_OBJ);
                err = -ENOENT;
                goto handler_err;
        }

        type_str = cJSON_GetStringValue(type_obj);

        if (type_str == NULL) {
		log_handler_err(HANDLER_ERR_TYPE_STR);
                err = -ENOENT;
                goto handler_err;
        }

        if (!strings_equal(type_str, "operation")) {
		log_handler_err(HANDLER_ERR_OP);
                err = -EINVAL;
                goto handler_err;
        }

        op_obj = cJSON_GetObjectItem(root_obj, "operation");

        if (op_obj == NULL) {
		log_handler_err(HANDLER_ERR_OP_OBJ);
                err = -ENOENT;
                goto handler_err;
        }

        op_type_obj = cJSON_GetObjectItem(op_obj, "type");

        if (op_type_obj == NULL) {
		log_handler_err(HANDLER_ERR_OP_TYPE_OBJ);
                err = -ENOENT;
                goto handler_err;
        }

        op_type_str = cJSON_GetStringValue(op_type_obj);

        if (op_type_str == NULL) {
		log_handler_err(HANDLER_ERR_OP_TYPE_STR);
                err = -ENOENT;
                goto handler_err;
        }

        mem_ptr = k_malloc(sizeof(proc_data));

        if (mem_ptr == NULL) {
		log_handler_err(HANDLER_ERR_PROC_DATA);
                err = -ENOMEM;
                goto handler_err;
        }

        proc_data.root_obj = root_obj;
        proc_data.op_obj = op_obj;
        LOG_DBG("Received request:");

        if (strings_equal(op_type_str, "beacon_request")) {
                log_handler_proc(GATEWAY_PROC_BEACON_REQ);
                proc_data.proc = GATEWAY_PROC_BEACON_REQ;

        } else if (strings_equal(op_type_str, "provision")) {
                log_handler_proc(GATEWAY_PROC_PROV);
                proc_data.proc = GATEWAY_PROC_PROV;

        } else if (strings_equal(op_type_str, "subnet_add")) {
                log_handler_proc(GATEWAY_PROC_SUBNET_ADD);
                proc_data.proc = GATEWAY_PROC_SUBNET_ADD;

        } else if (strings_equal(op_type_str, "subnet_generate")) {
                log_handler_proc(GATEWAY_PROC_SUBNET_GEN);
                proc_data.proc = GATEWAY_PROC_SUBNET_GEN;

        } else if (strings_equal(op_type_str, "subnet_delete")) {
                log_handler_proc(GATEWAY_PROC_SUBNET_DEL);
                proc_data.proc = GATEWAY_PROC_SUBNET_DEL;

        } else if (strings_equal(op_type_str, "subnet_request")) {
                log_handler_proc(GATEWAY_PROC_SUBNET_REQ);
                proc_data.proc = GATEWAY_PROC_SUBNET_REQ;

        } else if (strings_equal(op_type_str, "app_key_add")) {
                log_handler_proc(GATEWAY_PROC_APP_KEY_ADD);
                proc_data.proc = GATEWAY_PROC_APP_KEY_ADD;

        } else if (strings_equal(op_type_str, "app_key_generate")) {
                log_handler_proc(GATEWAY_PROC_APP_KEY_GEN);
                proc_data.proc = GATEWAY_PROC_APP_KEY_GEN;

        } else if (strings_equal(op_type_str, "app_key_delete")) {
                log_handler_proc(GATEWAY_PROC_APP_KEY_DEL);
                proc_data.proc = GATEWAY_PROC_APP_KEY_DEL;

        } else if (strings_equal(op_type_str, "app_key_request")) {
                log_handler_proc(GATEWAY_PROC_APP_KEY_REQ);
                proc_data.proc = GATEWAY_PROC_APP_KEY_REQ;

        } else if (strings_equal(op_type_str, "node_reset")) {
                log_handler_proc(GATEWAY_PROC_NODE_RESET);
                proc_data.proc = GATEWAY_PROC_NODE_RESET;

        } else if (strings_equal(op_type_str, "node_request")) {
                log_handler_proc(GATEWAY_PROC_NODE_REQ);
                proc_data.proc = GATEWAY_PROC_NODE_REQ;

        } else if (strings_equal(op_type_str, "node_discover")) {
                log_handler_proc(GATEWAY_PROC_NODE_DISC);
                proc_data.proc = GATEWAY_PROC_NODE_DISC;

        } else if (strings_equal(op_type_str, "node_configure")) {
                log_handler_proc(GATEWAY_PROC_NODE_CFG);
                proc_data.proc = GATEWAY_PROC_NODE_CFG;

        } else if (strings_equal(op_type_str, "subscribe")) {
                log_handler_proc(GATEWAY_PROC_SUBSCRIBE);
                proc_data.proc = GATEWAY_PROC_SUBSCRIBE;
        
        } else if (strings_equal(op_type_str, "unsubscribe")) {
                log_handler_proc(GATEWAY_PROC_UNSUBSCRIBE);
                proc_data.proc = GATEWAY_PROC_UNSUBSCRIBE; 
        
        } else if (strings_equal(op_type_str, "subscribe_list_request")) {
                log_handler_proc(GATEWAY_PROC_SUBSCRIBE_REQ);
                proc_data.proc = GATEWAY_PROC_SUBSCRIBE_REQ;

#if defined(CONFIG_BT_MESH_ACCESS_LAYER_MSG)
        } else if (strings_equal(op_type_str, "send_model_message")) {
                log_handler_proc(GATEWAY_PROC_SEND_MODEL_MSG);
                proc_data.proc = GATEWAY_PROC_SEND_MODEL_MSG;
#endif // defined(CONFIG_BT_MESH_ACCESS_LAYER_MSG)

	} else if (strings_equal(op_type_str, "health_fault_get")) {
		log_handler_proc(GATEWAY_PROC_HLTH_FAULT_GET);
		proc_data.proc = GATEWAY_PROC_HLTH_FAULT_GET;
	
	} else if (strings_equal(op_type_str, "health_fault_clear")) {
		log_handler_proc(GATEWAY_PROC_HLTH_FAULT_CLEAR);
		proc_data.proc = GATEWAY_PROC_HLTH_FAULT_CLEAR;

	} else if (strings_equal(op_type_str, "health_fault_test")) {
		log_handler_proc(GATEWAY_PROC_HLTH_FAULT_TEST);
		proc_data.proc = GATEWAY_PROC_HLTH_FAULT_TEST;

	} else if (strings_equal(op_type_str, "health_period_get")) {
		log_handler_proc(GATEWAY_PROC_HLTH_PERIOD_GET);
		proc_data.proc = GATEWAY_PROC_HLTH_PERIOD_GET;

	} else if (strings_equal(op_type_str, "health_period_set")) {
		log_handler_proc(GATEWAY_PROC_HLTH_PERIOD_SET);
		proc_data.proc = GATEWAY_PROC_HLTH_PERIOD_SET;

	} else if (strings_equal(op_type_str, "health_attention_get")) {
		log_handler_proc(GATEWAY_PROC_HLTH_ATTN_GET);
		proc_data.proc = GATEWAY_PROC_HLTH_ATTN_GET;

	} else if (strings_equal(op_type_str, "health_attention_set")) {
		log_handler_proc(GATEWAY_PROC_HLTH_ATTN_SET);
		proc_data.proc = GATEWAY_PROC_HLTH_ATTN_SET;

	} else if (strings_equal(op_type_str, "health_client_timeout_get")) {
		log_handler_proc(GATEWAY_PROC_HLTH_TIMEOUT_GET);
		proc_data.proc = GATEWAY_PROC_HLTH_TIMEOUT_GET;

	} else if (strings_equal(op_type_str, "health_client_timeout_set")) {
		log_handler_proc(GATEWAY_PROC_HLTH_TIMEOUT_SET);
		proc_data.proc = GATEWAY_PROC_HLTH_TIMEOUT_SET;

	} else {
		log_handler_err(HANDLER_ERR_UNKOWN_OP_TYPE);
                k_free(mem_ptr);
                err = -EINVAL;
                goto handler_err;
        }

        memcpy(mem_ptr, &proc_data, sizeof(proc_data));
        k_fifo_put(&gateway_proc_fifo, mem_ptr);
        return 0;

handler_err:
        cJSON_Delete(root_obj);
        return err;
}


int gateway_init(struct k_work_q *_work_q)
{
        if (_work_q == NULL) {
                LOG_ERR("NULL Workqueue");
                return -EINVAL;
        }

        work_q = _work_q;
        k_work_init_delayable(&prov_timeout_work, prov_timeout);

        cJSON_Init();

        k_thread_name_set(gateway_proc_thread, "gateway_proc_thread");

        return 0;
}
