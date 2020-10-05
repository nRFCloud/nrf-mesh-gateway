#ifndef GATEWAY_H_
#define GATEWAY_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr.h>
#include <bluetooth/mesh.h>
#include <net/cloud.h>

#include "util.h"
#include "nrf_cloud_transport.h"

uint8_t gateway_handler(const struct cloud_msg *gw_data);

void gateway_node_added(uint16_t net_idx, uint8_t uuid[UUID_LEN], uint16_t addr, uint8_t num_elem);

void gateway_msg_callback(uint32_t opcode, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf);

void gateway_hlth_cb(uint16_t addr, uint8_t test_id, uint16_t cid, uint8_t *faults,
		size_t fault_count);

int gateway_init(struct k_work_q *_work_q);


#ifdef __cplusplus
}
#endif


#endif /* GATEWAY_H_ */
