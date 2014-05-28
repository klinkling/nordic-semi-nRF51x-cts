/* ble_cts for nordic Semiconductor nRF51822
 * based on nordic's ble_bas, Copyright (c) 2012 Nordic Semiconductor, All rights reserved.
 * changes and additional code, Copyright (c) 2014, klinkling, All rights reserved.
 *
 * The code contributed by klinkling is released under the Apache License:
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 * The code by Nordic Semiconductor:
 * Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 *
 * @defgroup ble_sdk_srv_cts Current Time Service
 * @{
 * @ingroup ble_sdk_srv
 * @brief Current Time Service module.
 *
 * @details This module implements the Current Time Service with the Current Time characteristic.
 *          During initialization it adds the Current Time Service and Current Time characteristic
 *          to the BLE stack database.
 *
 *          If specified, the module will support notification of the Current Time characteristic
 *          through the ble_cts_date_time_change() function.
 *
 *			This implementation uses the read auth. When the time is read by a peer device the update_current_time_value_in_sd()
 *			is called and the time is updated. This gives the user freedom to implement sources of time that are not "cheap"
 *			enough to have them update the softdevice value permanently.
 *
 *			call ble_cts_init() to add the Service to the softdevice
 *
 *			chance the function update_current_time_value_in_sd() so it get's the time from where you wish
 *			
 *
 * @note The application must propagate BLE stack events to the Battery Service module by calling
 *       ble_cts_on_ble_evt() from the from the @ref ble_stack_handler callback.
 *
 */

#ifndef BLE_CTS_H__
#define BLE_CTS_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble_date_time.h"
#include "ble.h"
#include "ble_srv_common.h"

/**@brief Current Time Service event type. */
typedef enum
{
    BLE_CTS_EVT_NOTIFICATION_ENABLED,                             /**< Current Time value notification enabled event. */
    BLE_CTS_EVT_NOTIFICATION_DISABLED                             /**< Current Time value notification disabled event. */
} ble_cts_evt_type_t;

/**@brief Current Time Service event. */
typedef struct
{
    ble_cts_evt_type_t evt_type;                                  /**< Type of event. */
} ble_cts_evt_t;

// Forward declaration of the ble_cts_t type.
typedef struct ble_cts_s ble_cts_t;

/**@brief Current Time Service event handler type. */
typedef void (*ble_cts_evt_handler_t) (ble_cts_t * p_cts, ble_cts_evt_t * p_evt);

/**@brief Current Time Service init structure. This contains all options and data needed for
 *        initialization of the service.*/
typedef struct
{
	// in case you need an application level event handler for this service add it here and in ble_cts_t, it's defined above (ble_cts_evt_handler_t())
} ble_cts_init_t;

/**@brief Current Time Service structure. This contains various status information for the service. */
typedef struct ble_cts_s
{
	// in case you need an application level event handler for this service add it here and in ble_cts_s
    uint16_t                      service_handle;                 /**< Handle of Current Time Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t      current_time_handles;           /**< Handles related to the Current Time characteristic. */
    uint16_t                      conn_handle;                    /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection). */
} ble_cts_t;


/**@brief Function for initializing the Current Time Service.
 *
 * @param[out]  p_cts       Current Time Service structure. This structure will have to be supplied by
 *                          the application. It will be initialized by this function, and will later
 *                          be used to identify this particular service instance.
 * @param[in]   p_cts_init  Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
uint32_t ble_cts_init(ble_cts_t * p_cts, const ble_cts_init_t * p_cts_init);

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Current Time Service.
 *
 * @note For the requirements in the CTS specification to be fulfilled,
 *       ble_cts_date_time_update() must be called if the time in the device is updated.
 *
 * @param[in]   p_cts      Current Time Service structure.
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 */
void ble_cts_on_ble_evt(ble_cts_t * p_cts, ble_evt_t * p_ble_evt);

/**@brief Function for updating the time.
 *
 * @details The application calls this function if a BLE event comes in from the softdevice.
 *
 * @note For the requirements in the CTS specification to be fulfilled,
 *       ble_cts_date_time_update() must be called if the time in the device is updated.
 *
 * @param[in]   p_cts          Current Time Service structure.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */

uint32_t ble_cts_date_time_change(ble_cts_t * p_cts);

#endif // BLE_CTS_H__

/** @} */