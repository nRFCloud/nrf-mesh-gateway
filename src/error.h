#ifndef ERROR_H_
#define ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif


enum error_type {
    ERROR_CLOUD,
    ERROR_MODEM_RECOVERABLE,
    ERROR_MODEM_IRRECOVERABLE,
    ERROR_LTE_LC,
    ERROR_SYSTEM_FAULT
};


#ifdef __cplusplus
}
#endif


#endif /* ERROR_H_ */
