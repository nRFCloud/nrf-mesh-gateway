#ifndef BTMESH_H_
#define BTMESH_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <bluetooth/mesh.h>

#include "util.h"

#define PRIMARY_SUBNET 0x0000

struct btmesh_feature {
        bool state;
        bool support;
};

struct btmesh_relay_feature {
        bool state;
        bool support;
        uint8_t count;
        uint16_t interval;
};

struct btmesh_sig_model {
        uint16_t model_id;
        size_t appkey_count;
        uint16_t *appkey_idxs;
        size_t sub_addr_count;
        uint16_t *sub_addrs;
        struct bt_mesh_cfg_mod_pub pub;
};

struct btmesh_vnd_model {
        uint16_t company_id;
        uint16_t model_id;
        size_t appkey_count;
        uint16_t *appkey_idxs;
        size_t sub_addr_count;
        uint16_t *sub_addrs;
        struct bt_mesh_cfg_mod_pub pub;
};

struct btmesh_elem {
        uint16_t addr;
        uint16_t loc;
        size_t sig_model_count;
        size_t vnd_model_count;
        struct btmesh_sig_model *sig_models;
        struct btmesh_vnd_model *vnd_models;
};

struct btmesh_node {
        uint8_t uuid[UUID_LEN];
        uint16_t net_idx;
        uint16_t addr;
        uint16_t cid;
        uint16_t pid;
        uint16_t vid;
        uint16_t crpl;
        bool net_beacon_state;
        uint8_t ttl;
	struct bt_mesh_cfg_hb_sub hb_sub;
	struct bt_mesh_cfg_hb_pub hb_pub;
        struct btmesh_relay_feature relay;
        struct btmesh_feature proxy;
        struct btmesh_feature friend;
        bool lpn;
        size_t subnet_count;
        uint16_t *subnet_idxs;
        size_t elem_count;
        struct btmesh_elem *elems;
};

enum btmesh_op
{
        BTMESH_OP_PROV_ADV,
        BTMESH_OP_NODE_RESET,
        BTMESH_OP_COMP_GET,
        BTMESH_OP_BEACON_GET,
        BTMESH_OP_BEACON_SET,
        BTMESH_OP_TTL_GET,
        BTMESH_OP_TTL_SET,
        BTMESH_OP_FRIEND_GET,
        BTMESH_OP_FRIEND_SET,
        BTMESH_OP_PROXY_GET,
        BTMESH_OP_PROXY_SET,
        BTMESH_OP_RELAY_GET,
        BTMESH_OP_RELAY_SET,
        BTMESH_OP_NET_KEY_ADD,
        BTMESH_OP_NET_KEY_GET,
        BTMESH_OP_NET_KEY_DEL,
        BTMESH_OP_APP_KEY_ADD,
        BTMESH_OP_APP_KEY_GET,
        BTMESH_OP_APP_KEY_DEL,
        BTMESH_OP_MOD_APP_BIND,
        BTMESH_OP_MOD_APP_BIND_VND,
        BTMESH_OP_MOD_APP_UNBIND,
        BTMESH_OP_MOD_APP_UNBIND_VND,
        BTMESH_OP_MOD_APP_GET,
        BTMESH_OP_MOD_APP_GET_VND,
        BTMESH_OP_MOD_PUB_GET,
        BTMESH_OP_MOD_PUB_GET_VND,
        BTMESH_OP_MOD_PUB_SET,
        BTMESH_OP_MOD_PUB_SET_VND,
        BTMESH_OP_MOD_SUB_ADD,
        BTMESH_OP_MOD_SUB_ADD_VND,
        BTMESH_OP_MOD_SUB_DEL,
        BTMESH_OP_MOD_SUB_DEL_VND,
        BTMESH_OP_MOD_SUB_OVRW,
        BTMESH_OP_MOD_SUB_OVRW_VND,
        BTMESH_OP_MOD_SUB_GET,
        BTMESH_OP_MOD_SUB_GET_VND,
        BTMESH_OP_HLTH_FAULT_GET,
        BTMESH_OP_HLTH_FAULT_CLR,
        BTMESH_OP_HLTH_FAULT_TEST,
        BTMESH_OP_HLTH_PERIOD_GET,
        BTMESH_OP_HLTH_PERIOD_SET,
        BTMESH_OP_HLTH_ATTN_GET,
        BTMESH_OP_HLTH_ATTN_SET,
        BTMESH_OP_HLTH_TIMEOUT_GET,
        BTMESH_OP_HLTH_TIMEOUT_SET,
	BTMESH_OP_HB_SUB_GET,
	BTMESH_OP_HB_SUB_SET,
	BTMESH_OP_HB_PUB_GET,
	BTMESH_OP_HB_PUB_SET
};

union btmesh_op_args {
        struct {
                uint8_t uuid[16];
                uint16_t net_idx;
                uint16_t addr;
                uint8_t attn;
        } prov_adv;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                bool status;
        } node_reset;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint8_t page;
                uint8_t status;
                struct net_buf_simple *comp;
        } comp_get;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint8_t status;
        } beacon_get;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint8_t val;
                uint8_t status;
        } beacon_set;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint8_t ttl;
        } ttl_get;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint8_t val;
                uint8_t ttl;
        } ttl_set;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint8_t status;
        } friend_get;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint8_t val;
                uint8_t status;
        } friend_set;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint8_t status;
        } proxy_get;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint8_t val;
                uint8_t status;
        } proxy_set;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint8_t status;
                uint8_t transmit;
        } relay_get;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint8_t new_relay;
                uint8_t new_transmit;
                uint8_t status;
                uint8_t transmit;
        } relay_set;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t key_net_idx;
                uint8_t net_key[16];
                uint8_t status;
        } net_key_add;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t keys[CONFIG_BT_MESH_SUBNET_COUNT];
                size_t key_cnt;
        } net_key_get;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t key_net_idx;
                uint8_t status;
        } net_key_del;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t key_net_idx;
                uint16_t key_app_idx;
                uint8_t app_key[16];
                uint8_t status;
        } app_key_add;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t key_net_idx;
                uint8_t status;
                uint16_t keys[CONFIG_BT_MESH_APP_KEY_COUNT];
                size_t key_cnt;
        } app_key_get;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t key_net_idx;
                uint16_t key_app_idx;
                uint8_t status;
        } app_key_del;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t mod_app_idx;
                uint16_t mod_id;
                uint8_t status;
        } mod_app_bind;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t mod_app_idx;
                uint16_t mod_id;
                uint8_t status;
                uint16_t cid;
        } mod_app_bind_vnd;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t mod_app_idx;
                uint16_t mod_id;
                uint8_t status;
        } mod_app_unbind;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t mod_app_idx;
                uint16_t mod_id;
                uint8_t status;
                uint16_t cid;
        } mod_app_unbind_vnd;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t mod_id;
                uint8_t status;
                uint16_t apps[CONFIG_BT_MESH_APP_KEY_COUNT];
                size_t app_cnt;
        } mod_app_get;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t mod_id;
                uint8_t status;
                uint16_t apps[CONFIG_BT_MESH_APP_KEY_COUNT];
                size_t app_cnt;
                uint16_t cid;
        } mod_app_get_vnd;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t mod_id;
                struct bt_mesh_cfg_mod_pub pub;
                uint8_t status;
        } mod_pub_get;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t mod_id;
                struct bt_mesh_cfg_mod_pub pub;
                uint8_t status;
                uint16_t cid;
        } mod_pub_get_vnd;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t mod_id;
                struct bt_mesh_cfg_mod_pub pub;
                uint8_t status;
        } mod_pub_set;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t mod_id;
                struct bt_mesh_cfg_mod_pub pub;
                uint8_t status;
                uint16_t cid;
        } mod_pub_set_vnd;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t sub_addr;
                uint16_t mod_id;
                uint8_t status;
        } mod_sub_add;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t sub_addr;
                uint16_t mod_id;
                uint8_t status;
                uint16_t cid;
        } mod_sub_add_vnd;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t sub_addr;
                uint16_t mod_id;
                uint8_t status;
        } mod_sub_del;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t sub_addr;
                uint16_t mod_id;
                uint8_t status;
                uint16_t cid;
        } mod_sub_del_vnd;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t sub_addr;
                uint16_t mod_id;
                uint8_t status;
        } mod_sub_ovrw;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t sub_addr;
                uint16_t mod_id;
                uint8_t status;
                uint16_t cid;
        } mod_sub_ovrw_vnd;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t mod_id;
                uint8_t status;
                uint16_t subs[CONFIG_BT_MESH_MODEL_GROUP_COUNT];
                size_t sub_cnt;
        } mod_sub_get;
        struct {
                uint16_t net_idx;
                uint16_t addr;
                uint16_t elem_addr;
                uint16_t mod_id;
                uint16_t cid;
                uint8_t status;
                uint16_t subs[CONFIG_BT_MESH_MODEL_GROUP_COUNT];
                size_t sub_cnt;
        } mod_sub_get_vnd;
        struct {
                uint16_t addr;
                uint16_t app_idx;
                uint16_t cid;
                uint8_t test_id;
                uint8_t faults[10];
                size_t fault_count;
        } hlth_fault_get;
        struct {
                uint16_t addr;
                uint16_t app_idx;
                uint16_t cid;
                uint8_t test_id;
                uint8_t faults[10];
                size_t fault_count;
        } hlth_fault_clear;
        struct {
                uint16_t addr;
                uint16_t app_idx;
                uint16_t cid;
                uint8_t test_id;
                uint8_t faults[10];
                size_t fault_count;
        } hlth_fault_test;
        struct {
                uint16_t addr;
                uint16_t app_idx;
                uint8_t divisor;
        } hlth_period_get;
        struct {
                uint16_t addr;
                uint16_t app_idx;
                uint8_t divisor;
                uint8_t updated_divisor;
        } hlth_period_set;
        struct {
                uint16_t addr;
                uint16_t app_idx;
                uint8_t attn;
        } hlth_attn_get;
        struct {
                uint16_t addr;
                uint16_t app_idx;
                uint8_t attn;
                uint8_t updated_attn;
        } hlth_attn_set;
        struct {
                int32_t timeout;
        } hlth_timeout_get;
        struct {
                int32_t timeout;
        } hlth_timeout_set;
	struct {
		uint16_t net_idx;
		uint16_t addr;
		struct bt_mesh_cfg_hb_sub sub;
		uint8_t status;
	} hb_sub_get;
	struct {
		uint16_t net_idx;
		uint16_t addr;
		struct bt_mesh_cfg_hb_sub sub;
		uint8_t status;
	} hb_sub_set;
	struct {
		uint16_t net_idx;
		uint16_t addr;
		struct bt_mesh_cfg_hb_pub pub;
		uint8_t status;
	} hb_pub_get;
	struct {
		uint16_t net_idx;
		uint16_t addr;
		struct bt_mesh_cfg_hb_pub pub;
		uint8_t status;
	} hb_pub_set;
};

enum btmesh_sub_type {
        BTMESH_SUB_TYPE_CLI,
        BTMESH_SUB_TYPE_GATEWAY
};

const char* btmesh_parse_opcode(uint32_t opcode);

int btmesh_subscribe(enum btmesh_sub_type type, uint16_t addr);

void btmesh_unsubscribe(enum btmesh_sub_type type, uint16_t addr);

const uint16_t *btmesh_get_subscribe_list(enum btmesh_sub_type);

const char *btmesh_get_sig_model_str(uint16_t model_id);

const char *btmesh_get_blocked_beacon(size_t idx);

int btmesh_block_beacon(uint8_t uuid[UUID_LEN]);

int btmesh_unblock_beacon(uint8_t uuid[UUID_LEN]);

const char *btmesh_get_beacon_uuid(size_t idx);

const char *btmesh_get_beacon_oob(size_t idx);

const uint32_t *btmesh_get_beacon_uri_hash(size_t idx);

uint8_t btmesh_get_pub_period(uint8_t period);

char * btmesh_get_pub_period_unit_str(uint8_t period);

void btmesh_free_node(struct btmesh_node *node);

int btmesh_discover_node(struct btmesh_node *node, uint8_t *status);

int btmesh_perform_op(enum btmesh_op op, union btmesh_op_args* args);

int btmesh_clean_node_key(uint16_t addr, uint16_t app_idx);

int btmesh_init(void);


#ifdef __cplusplus
}
#endif


#endif /* BTMESH_H_ */
