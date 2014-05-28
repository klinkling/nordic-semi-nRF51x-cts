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

/* The following three functions must be called by the application:
 * - ble_cts_init()
 * - ble_cts_on_ble_evt()
 * - ble_cts_date_time_update()
 */

#include "ble_cts.h"
#include <string.h>
#include "ble_date_time.h"		// a file provided by nordic with the SDK (use a file system search to find it). It can't be shared due to license restrictions.
#include "nordic_common.h"
#include "ble_srv_common.h"
#include "app_util.h"

#define		ENC_DATE_TIME_LENGTH	7		// length of the encoded dateTime don't touch this! It really is 7.

/* update_current_time_value()
 * used to update the current time value in the softdevice
 * after this a notification can be sent or the authorisation for
 * a read can be given to the softdevice
 * NOTE: This is the only connection point between ble_cts.c and your time source
 */
static uint32_t update_current_time_value_in_sd(ble_cts_t * p_cts){
	uint16_t len;
	uint8_t encodedDateTime[ENC_DATE_TIME_LENGTH];

	// TODO: implement a function to get the time
	cts_getTime(&dateTime);

    len = (uint16_t) ble_date_time_encode(&dateTime, encodedDateTime);

	// write the new value into the softdevice
    return sd_ble_gatts_value_set(p_cts->current_time_handles.value_handle,  0, &len, encodedDateTime);
}


/**@brief     Function for handling a Read event on the Current Time characteristic.
 *
 * @param[in] p_cts             CTS Service Structure.
 * @param[in] p_ble_read_evt    Pointer to the read event received from BLE stack.
 *
 * @return    NRF_SUCCESS on successful processing of current time read. Otherwise an error code.
 */
static uint32_t on_current_time_read(ble_cts_t * p_cts, ble_gatts_evt_read_t * p_ble_read_evt){
	ble_gatts_rw_authorize_reply_params_t read_authorize_reply;

	// update the value in the softdevice
	update_current_time_value_in_sd(p_cts);

	// --- send an positive authorisation ---
	// clear the whole thing so that everything not set here will be 0
	memset(&read_authorize_reply, 0, sizeof(read_authorize_reply));
	// set the type of the authorisation reply to read
	read_authorize_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
	// send a positive GATTS response to the peer and give authorisation for the operation to the softdevice
	read_authorize_reply.params.read.gatt_status = BLE_GATT_STATUS_SUCCESS;
	// set the length
	read_authorize_reply.params.read.len = ENC_DATE_TIME_LENGTH;
	// set a NULL value as it will be updated by update_current_time_value_in_sd() and not here
	read_authorize_reply.params.read.p_data = NULL;
	// send authorisation
	sd_ble_gatts_rw_authorize_reply(p_cts->conn_handle, &read_authorize_reply);

	// always return success
	return NRF_SUCCESS;
}


/* --- BLE event types --- */

/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_cts       Current Time Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(ble_cts_t * p_cts, ble_evt_t * p_ble_evt)
{
	// save the connection handle into the struct
    p_cts->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}


/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_cts       Current Time Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_cts_t * p_cts, ble_evt_t * p_ble_evt)
{
	// make the connection handle invalid
    UNUSED_PARAMETER(p_ble_evt);
    p_cts->conn_handle = BLE_CONN_HANDLE_INVALID;
}


/**@brief Function for handling the Write event.
 *
 * @param[in]   p_cts       current time service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_write(ble_cts_t * p_cts, ble_evt_t * p_ble_evt)
{

	// as the Current Time Service does not have any kind of cyclic notification the state doesn't need
	// to be saved here and no handler needs to be called. Note: the softdevice doesn't permit sending notifications
	// if the appropriate cccd bits are not set. So in case a notification is generated by a time update or one
	// of the other reasons specified by the SIG the softdevice will handle the decision of if a notification is sent.
	// Therefore the CTS doesn't need to save any notification on/off state.
}

	// a general purpose application event_handler could be added here, but I don't need it right now


/**@brief     Function for handling the @ref BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST event from the S110
 *            Stack.
 *
 * @param[in] p_cts     CTS Service Structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_rw_auth_req(ble_cts_t * p_cts, ble_evt_t * p_ble_evt)
{
    ble_gatts_evt_rw_authorize_request_t * p_authorize_request;

    // copy the authorize request into an own variable
    p_authorize_request = &(p_ble_evt->evt.gatts_evt.params.authorize_request);

    // if it is a read authentication request and for the the current time characteristic
    if ((p_authorize_request->type == BLE_GATTS_AUTHORIZE_TYPE_READ)
        &&
        (p_authorize_request->request.read.handle == p_cts->current_time_handles.value_handle)){
    		// call the function to handle this case
    		on_current_time_read(p_cts, &(p_authorize_request->request.read));
    }
}


// --- the main event handler -> calls the specific event handlers ---
void ble_cts_on_ble_evt(ble_cts_t * p_cts, ble_evt_t * p_ble_evt)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_cts, p_ble_evt);
            break;
            
        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_cts, p_ble_evt);
            break;
            
        case BLE_GATTS_EVT_WRITE:
            on_write(p_cts, p_ble_evt);
            break;

        case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
            on_rw_auth_req(p_cts, p_ble_evt);
            break;
            
        default:
            // No implementation needed.
            break;
    }
}

// called only by ble_cts_init()
/**@brief Function for adding the current time characteristic.
 *
 * @param[in]   p_cts        Current time service structure.
 * @param[in]   p_cts_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t current_time_char_add(ble_cts_t * p_cts, const ble_cts_init_t * p_cts_init)
{
    uint32_t            err_code;
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;
    uint8_t             init_len;

    // --- Add Current Time characteristic ---
	memset(&cccd_md, 0, sizeof(cccd_md));

	// According to CTS_SPEC_V10, the read operation on cccd should be possible without
	// authentication. Let's set read and write to without auth
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

	cccd_md.vloc = BLE_GATTS_VLOC_STACK;
    
	// specify metadata of characteristic
    memset(&char_md, 0, sizeof(char_md));
    
    char_md.char_props.read   = 1;			// read property is supported
    char_md.char_props.notify = 1;			// notify property is supported
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md;	// notify is supported so set the &cccd_md
    char_md.p_sccd_md         = NULL;
    
    BLE_UUID_BLE_ASSIGN(ble_uuid, BLE_UUID_CURRENT_TIME_CHAR);
    
    // specify metadata of attribute
    memset(&attr_md, 0, sizeof(attr_md));
    // set the security level as specified by the SIG Current Time Service
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);			// it is readalbe
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);	// but not writeable

    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    // turn on read authentication for this attribute so it will generate an event to the application when the attribute is read
    // this will allow to calculate the time and update it before the softdevice sends a response
    attr_md.rd_auth    = 1;
    attr_md.wr_auth    = 0;			// write is forbidden anyway but turn this off for consistency
    attr_md.vlen       = 0;
    
    // specify how the characteristics value attribute is
    memset(&attr_char_value, 0, sizeof(attr_char_value));
    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = ENC_DATE_TIME_LENGTH;
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = ENC_DATE_TIME_LENGTH;
    attr_char_value.p_value      = NULL;			// the initial value is not important, just pass NULL as read auth is enabled it will be updated before it is read
    
    // add the characteristic to the softdevice
    err_code = sd_ble_gatts_characteristic_add(p_cts->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_cts->current_time_handles);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    // report reference is not needed for the current time service    
    return NRF_SUCCESS;
}

/*
 * ble_cts_init()
 * -- called by application --
 * call this from the application to init the service in the softdevice
 * call it just once during the init sequence
 */

uint32_t ble_cts_init(ble_cts_t * p_cts, const ble_cts_init_t * p_cts_init)
{
    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure
    p_cts->conn_handle               = BLE_CONN_HANDLE_INVALID;
    
    // assign the correct service uuid for the current_time_service specified by the Bluetooth SIG
    BLE_UUID_BLE_ASSIGN(ble_uuid, BLE_UUID_CURRENT_TIME_SERVICE);

    // Add the service to the softdevice
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_cts->service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    // Add current time characteristic
    return current_time_char_add(p_cts, p_cts_init);
}


/*
 * ble_cts_date_time_change()
 * this is called by the application in case of a time change that must be notified to the peer.
 * The time is acquired using update_current_time_value_in_sd() from whatever source you wish
 * and reported to the peer device.
 */

uint32_t ble_cts_date_time_change(ble_cts_t * p_cts){
	ble_gatts_hvx_params_t params;
	uint16_t len = ENC_DATE_TIME_LENGTH;

	// updates the value in the sd to it is ready to be sent
	update_current_time_value_in_sd(p_cts);

	// prepare the content of the notification
	memset(&params, 0, sizeof(params));
	params.type = BLE_GATT_HVX_NOTIFICATION;
	params.handle = p_cts->current_time_handles.value_handle;
	params.p_data = NULL;	// updated by update_current_time_value_in_sd(), so just set NULL here
	params.p_len = &len;	// expects a pointer, to write back to it, could maybe also be NULL, not sure

	// tell the sd to send the notification
    return sd_ble_gatts_hvx(p_cts->conn_handle, &params);
}
