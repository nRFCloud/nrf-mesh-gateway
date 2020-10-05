#ifndef CODEC_H_
#define CODEC_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <cJSON.h>
#include <cJSON_os.h>

#include "btmesh.h"
#include "util.h"


int codec_encode_beacon_list(char *buf, size_t buf_len);

int codec_encode_prov_result(char *buf, size_t buf_len, int prov_err, uint16_t net_idx,
        uint8_t uuid[UUID_LEN], uint16_t addr, uint8_t num_elem);

int codec_parse_prov(cJSON *op_obj, uint8_t uuid[UUID_LEN], uint16_t *net_idx, uint16_t *addr,
		uint8_t *attn);

int codec_encode_subnet_list(char *buf, size_t buf_len);

int codec_parse_subnet(cJSON *op_obj, uint16_t *net_idx);

int codec_parse_subnet_add(cJSON *op_obj, uint16_t *net_idx, uint8_t net_key[KEY_LEN]);

int codec_encode_app_key_list(char *buf, size_t buf_len);

int codec_parse_app_key(cJSON *op_obj, uint16_t *net_idx, uint16_t *app_idx);

int codec_parse_app_key_add(cJSON *op_obj, uint16_t *net_idx, uint16_t *app_idx,
		uint8_t app_key[KEY_LEN]);

int codec_parse_node_disc(cJSON *op_obj, uint16_t *addr);

int codec_encode_node_list(char *buf, size_t buf_len);

int codec_encode_node_disc(char *buf, size_t buf_len, struct btmesh_node *node, int disc_err,
        uint8_t status);

int codec_parse_node_cfg(cJSON *op_obj, uint16_t* addr);

int codec_parse_subscribe_addrs(cJSON *op_obj, uint16_t **addr_list, int *addr_count);

int codec_encode_subscribe_list(char *buf, size_t buf_len);

int codec_parse_model_msg(cJSON *op_obj, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf);

int codec_encode_model_msg(char *buf, size_t buf_len, uint32_t opcode, struct bt_mesh_msg_ctx *ctx,
                uint8_t *payload, size_t payload_len);

int codec_parse_hlth_fault(cJSON *op_obj, uint16_t *addr, uint16_t *app_idx, uint16_t *cid);

int codec_parse_hlth_fault_test(cJSON *op_obj, uint16_t *addr, uint16_t *app_idx, uint16_t *cid,
		uint8_t *test_id);

int codec_encode_hlth_faults_cur(char *buf, size_t buf_len, uint16_t addr, uint16_t cid,
		uint8_t test_id, uint8_t *faults, size_t fault_count);

int codec_encode_hlth_faults_reg(char *buf, size_t buf_len, uint16_t addr, uint16_t app_idx,
		uint16_t cid, uint8_t test_id, uint8_t *faults, size_t fault_count);

int codec_parse_hlth_period(cJSON *op_obj, uint16_t *addr, uint16_t *app_idx);

int codec_parse_hlth_period_set(cJSON *op_obj, uint16_t *addr, uint16_t *app_idx, uint8_t *div);

int codec_encode_hlth_period(char *buf, size_t buf_len, uint16_t addr, uint8_t div);

int codec_parse_hlth_attn(cJSON *op_obj, uint16_t *addr, uint16_t *app_idx);

int codec_parse_hlth_attn_set(cJSON *op_obj, uint16_t *addr, uint16_t *app_idx, uint8_t *attn);

int codec_encode_hlth_attn(char *buf, size_t buf_len, uint16_t addr, uint8_t attn);

int codec_parse_hlth_timeout(cJSON *op_obj, int32_t *timeout);

int codec_encode_hlth_timeout(char *buf, size_t buf_len, int32_t timeout);


#ifdef __cplusplus
}
#endif


#endif /* CODEC_H_ */
