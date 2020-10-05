#ifndef GW_CLOUD_H_
#define GW_CLOUD_H_


#ifdef __cplusplus
extern "C" {
#endif


#include "error.h"


#define NRF_CLOUD_CLIENT_ID_LEN 10

extern char gateway_id[NRF_CLOUD_CLIENT_ID_LEN+1];

void gw_cloud_error_callback(void);

const char *gw_cloud_get_id(void);

int gw_cloud_init(struct k_work_q *_work_q,
        void(*_error_handler)(enum error_type type, int err));


#ifdef __cplusplus
}
#endif


#endif /* GW_CLOUD_H_ */
