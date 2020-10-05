#ifndef LTE_H_
#define LTE_H_


#ifdef __cplusplus
extern "C" {
#endif


#include "error.h"
#include "gateway.h"


void lte_shutdown_modem(void);

int lte_init(struct k_work_q *_work_q, void(*_error_handler)(enum error_type type, int err));


#ifdef _cplusplus
}
#endif


#endif /* LTE_H_ */
