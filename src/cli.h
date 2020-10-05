#ifndef CLI_H_
#define CLI_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <bluetooth/mesh.h>


void cli_init(void);

void cli_msg_callback(uint32_t opcode, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf);

void cli_hlth_cb(uint16_t addr, uint8_t test_id, uint16_t cid, uint8_t *faults, size_t fault_count);


#ifdef __cplusplus
}
#endif


#endif /* CLI_H_ */
