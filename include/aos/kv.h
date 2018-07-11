/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#ifndef AOS_KV_H
#define AOS_KV_H

#include <aos/types.h>
#include <hal/soc/flash.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Add a new KV pair.
 *
 * @param[in]  key    the key of the KV pair.
 * @param[in]  value  the value of the KV pair.
 * @param[in]  len    the length of the value.
 * @param[in]  sync   save the KV pair to flash right now (should always be 1).
 *
 * @return  0 on success, negative error on failure.
 */
int aos_kv_set(const char *key, const void *value, int len, int sync);

/**
 * Get the KV pair's value stored in buffer by its key.
 *
 * @note: the buffer_len should be larger than the real length of the value,
 *        otherwise buffer would be NULL.
 *
 * @param[in]      key         the key of the KV pair to get.
 * @param[out]     buffer      the memory to store the value.
 * @param[in-out]  buffer_len  in: the length of the input buffer.
 *                             out: the real length of the value.
 *
 * @return  0 on success, negative error on failure.
 */
int aos_kv_get(const char *key, void *buffer, int *buffer_len);

/**
 * Delete the KV pair by its key.
 *
 * @param[in]  key  the key of the KV pair to delete.
 *
 * @return  0 on success, negative error on failure.
 */
int aos_kv_del(const char *key);


typedef struct aos_kv2 {
    hal_partition_t partition;
    int encrypt;
} aos_kv2_t;

/**
 * New KV class.
 *
 * @param[in-out] kv            kv context
 * @param[in]     partition     the flash partition id.
 * @param[in]     encrypt       1: encrypt the data, 0: don't encrypt.
 *
 * @return  0 on success, negative error on failure.
 */
int aos_kv2_open(aos_kv2_t *kv, hal_partition_t partition, int encrypt);

/**
 * Set a new KV pair.
 *
 * @param[in]  kv     kv context
 * @param[in]  key    the key of the KV pair.
 * @param[in]  value  the value of the KV pair.
 * @param[in]  len    the length of the value.
 *
 * @return  0 on success, negative error on failure.
 */
int aos_kv2_set(aos_kv2_t *kv, const char *key, const void *value, int len);

/**
 * Get the KV pair's value stored in buffer by its key.
 *
 * @note: the buffer_len should be larger than the real length of the value,
 *        otherwise buffer would be NULL.
 *
 * @param[in]      kv     kv context
 * @param[in]      key    the key of the KV pair to get.
 * @param[out]     value  the memory to store the value.
 * @param[in]      len    the length of the input buffer.
 *
 * @return  negative error on failure. >=0: the real length of the value.
 */
int aos_kv2_get(aos_kv2_t *kv, const char *key, void *value, int len);

/**
 * Delete the KV pair by its key.
 *
 * @param[in]  kv   kv context
 * @param[in]  key  the key of the KV pair to delete.
 *
 * @return  0 on success, negative error on failure.
 */
int aos_kv2_del(aos_kv2_t *kv, const char *key);

int aos_kv2_close(aos_kv2_t *kv);

#ifdef __cplusplus
}
#endif

#endif /* AOS_KV_H */
