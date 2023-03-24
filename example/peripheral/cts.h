/** @file
 *  @brief CTS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif

void cts_init(void);
void cts_notify(void);

extern struct bt_gatt_service_static cts_svc;

#ifdef __cplusplus
}
#endif
