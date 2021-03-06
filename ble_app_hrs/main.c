/**
 * Copyright (c) 2014 - 2017, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @example examples/ble_peripheral/ble_app_hrs/main.c
 *
 * @brief Heart Rate Service Sample Application main file.
 *
 * This file contains the source code for a sample application using the Heart Rate service
 * (and also Battery and Device Information services). This application uses the
 * @ref srvlib_conn_params module.
 */

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_sdm.h"
#include "app_error.h"
#include "ble.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_bas.h"
#include "ble_hrs.h"
#include "ble_dis.h"
#include "ble_conn_params.h"
#include "sensorsim.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "app_timer.h"
#include "bsp_btn_ble.h"
#include "app_scheduler.h"
#include "peer_manager.h"
#include "fds.h"
#include "nrf_ble_gatt.h"
#include "ble_conn_state.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"


#define DEVICE_NAME                         "Nordic_HRM"                            /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME                   "NordicSemiconductor"                   /**< Manufacturer. Will be passed to Device Information Service. */


#define APP_BLE_CONN_CFG_TAG            1                                       /**< A tag identifying the SoftDevice BLE configuration. */
#define LINK_TOTAL                      NRF_SDH_BLE_PERIPHERAL_LINK_COUNT + \
        NRF_SDH_BLE_CENTRAL_LINK_COUNT

#define ADVERTISING_LED                 BSP_BOARD_LED_0                         /**< Is on when device is advertising. */
#define CONNECTED_LED                   BSP_BOARD_LED_1                         /**< Is on when device has connected. */
#define BONDING_LED                     BSP_BOARD_LED_2                         /**< Is on when device is advertising for 2nd connection. */
#define CONNECTED_2_LED                 BSP_BOARD_LED_3                         /**< Is on when device has connected with 2nd host. */

#define REMOVE_BOND_BUTTON              BSP_BUTTON_0                            /**< Button that will trigger the notification event with the LED Button Service */
#define BONDING_BUTTON                  BSP_BUTTON_1                            /**< Button that will trigger the notification event with the LED Button Service */

#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50)                     /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */

#define APP_ADV_FAST_INTERVAL               0x0028                                     /**< Fast advertising interval (in units of 0.625 ms. This value corresponds to 25 ms.). */
#define APP_ADV_SLOW_INTERVAL               0x0C80                                     /**< Slow advertising interval (in units of 0.625 ms. This value corrsponds to 2 seconds). */
#define APP_ADV_FAST_TIMEOUT                30                                         /**< The duration of the fast advertising period (in seconds). */
#define APP_ADV_SLOW_TIMEOUT                180                                        /**< The duration of the slow advertising period (in seconds). */

#define APP_BLE_CONN_CFG_TAG                1                                       /**< A tag identifying the SoftDevice BLE configuration. */
#define APP_BLE_OBSERVER_PRIO               3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */

#define BATTERY_LEVEL_MEAS_INTERVAL         APP_TIMER_TICKS(2000)                   /**< Battery level measurement interval (ticks). */
#define MIN_BATTERY_LEVEL                   81                                      /**< Minimum simulated battery level. */
#define MAX_BATTERY_LEVEL                   100                                     /**< Maximum simulated 7battery level. */
#define BATTERY_LEVEL_INCREMENT             1                                       /**< Increment between each simulated battery level measurement. */

#define HEART_RATE_MEAS_INTERVAL            APP_TIMER_TICKS(1000)                   /**< Heart rate measurement interval (ticks). */
#define MIN_HEART_RATE                      140                                     /**< Minimum heart rate as returned by the simulated measurement function. */
#define MAX_HEART_RATE                      300                                     /**< Maximum heart rate as returned by the simulated measurement function. */
#define HEART_RATE_INCREMENT                10                                      /**< Value by which the heart rate is incremented/decremented for each call to the simulated measurement function. */

#define RR_INTERVAL_INTERVAL                APP_TIMER_TICKS(300)                    /**< RR interval interval (ticks). */
#define MIN_RR_INTERVAL                     100                                     /**< Minimum RR interval as returned by the simulated measurement function. */
#define MAX_RR_INTERVAL                     500                                     /**< Maximum RR interval as returned by the simulated measurement function. */
#define RR_INTERVAL_INCREMENT               1                                       /**< Value by which the RR interval is incremented/decremented for each call to the simulated measurement function. */

#define SENSOR_CONTACT_DETECTED_INTERVAL    APP_TIMER_TICKS(5000)                   /**< Sensor Contact Detected toggle interval (ticks). */

#define MIN_CONN_INTERVAL                   MSEC_TO_UNITS(400, UNIT_1_25_MS)        /**< Minimum acceptable connection interval (0.4 seconds). */
#define MAX_CONN_INTERVAL                   MSEC_TO_UNITS(650, UNIT_1_25_MS)        /**< Maximum acceptable connection interval (0.65 second). */
#define SLAVE_LATENCY                       0                                       /**< Slave latency. */
#define CONN_SUP_TIMEOUT                    MSEC_TO_UNITS(4000, UNIT_10_MS)         /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY      APP_TIMER_TICKS(5000)                   /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY       APP_TIMER_TICKS(30000)                  /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT        3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                      1                                       /**< Perform bonding. */
#define SEC_PARAM_MITM                      0                                       /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                      0                                       /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS                  0                                       /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES           BLE_GAP_IO_CAPS_NONE                    /**< No I/O capabilities. */
#define SEC_PARAM_OOB                       0                                       /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE              7                                       /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE              16                                      /**< Maximum encryption key size. */

#define SCHED_MAX_EVENT_DATA_SIZE           APP_TIMER_SCHED_EVENT_DATA_SIZE            /**< Maximum size of scheduler events. */
#ifdef SVCALL_AS_NORMAL_FUNCTION
#define SCHED_QUEUE_SIZE                    20                                         /**< Maximum number of events in the scheduler queue. More is needed in case of Serialization. */
#else
#define SCHED_QUEUE_SIZE                    10                                         /**< Maximum number of events in the scheduler queue. */
#endif


#define DEAD_BEEF                           0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define APP_FEATURE_NOT_SUPPORTED           BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2    /**< Reply when unsupported features are requested. */


BLE_HRS_DEF(m_hrs);                                                 /**< Heart rate service instance. */
BLE_BAS_DEF(m_bas);                                                 /**< Structure used to identify the battery service. */
NRF_BLE_GATT_DEF(m_gatt);                                           /**< GATT module instance. */
BLE_ADVERTISING_DEF(m_advertising);                                 /**< Advertising module instance. */
APP_TIMER_DEF(m_battery_timer_id);                                  /**< Battery timer. */
APP_TIMER_DEF(m_heart_rate_timer_id);                               /**< Heart rate measurement timer. */
APP_TIMER_DEF(m_rr_interval_timer_id);                              /**< RR interval timer. */
APP_TIMER_DEF(m_sensor_contact_timer_id);                           /**< Sensor contact detected timer. */

#define ADVERTISING_BOND_TIME_INTERVAL                               APP_TIMER_TICKS(30000)
APP_TIMER_DEF(m_advertising_bond_timer_id);                                /**< Advertising time for the bonding with 2nd host. */
static bool advertising_bond_timer_is_running = false;                     /**< Flag Avertising timer status for bonding with 2nd host. */
static pm_peer_id_t m_bonded_peer_id;                                      /**< Peer ID of the current bonded central. */
static bool m_bond_second_host_is_running = false;

static uint16_t m_conn_handle         = BLE_CONN_HANDLE_INVALID;    /**< Handle of the current connection. */
static bool m_rr_interval_enabled = true;                           /**< Flag for enabling and disabling the registration of new RR interval measurements (the purpose of disabling this is just to test sending HRM without RR interval data. */

static sensorsim_cfg_t m_battery_sim_cfg;                           /**< Battery Level sensor simulator configuration. */
static sensorsim_state_t m_battery_sim_state;                       /**< Battery Level sensor simulator state. */
static sensorsim_cfg_t m_heart_rate_sim_cfg;                        /**< Heart Rate sensor simulator configuration. */
static sensorsim_state_t m_heart_rate_sim_state;                    /**< Heart Rate sensor simulator state. */
static sensorsim_cfg_t m_rr_interval_sim_cfg;                       /**< RR Interval sensor simulator configuration. */
static sensorsim_state_t m_rr_interval_sim_state;                   /**< RR Interval sensor simulator state. */

static pm_peer_id_t m_peer_id;                                      /**< Device reference handle to the current bonded central. */
static uint32_t m_whitelist_peer_cnt;                               /**< Number of peers currently in the whitelist. */
static pm_peer_id_t m_whitelist_peers[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];        /**< List of peers currently in the whitelist. */


static ble_uuid_t m_adv_uuids[] =                                   /**< Universally unique service identifiers. */
{
        {BLE_UUID_HEART_RATE_SERVICE,           BLE_UUID_TYPE_BLE},
        {BLE_UUID_BATTERY_SERVICE,              BLE_UUID_TYPE_BLE},
        {BLE_UUID_DEVICE_INFORMATION_SERVICE,   BLE_UUID_TYPE_BLE}
};


/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
        app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Fetch the list of peer manager peer IDs.
 *
 * @param[inout] p_peers   The buffer where to store the list of peer IDs.
 * @param[inout] p_size    In: The size of the @p p_peers buffer.
 *                         Out: The number of peers copied in the buffer.
 */
static void peer_list_get(pm_peer_id_t * p_peers, uint32_t * p_size)
{
        pm_peer_id_t peer_id;
        uint32_t peers_to_copy;

        peers_to_copy = (*p_size < BLE_GAP_WHITELIST_ADDR_MAX_COUNT) ?
                        *p_size : BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

        peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);
        *p_size = 0;

        while ((peer_id != PM_PEER_ID_INVALID) && (peers_to_copy--))
        {
                p_peers[(*p_size)++] = peer_id;
                peer_id = pm_next_peer_id_get(peer_id);
        }
}


/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
        ret_code_t err_code;

        NRF_LOG_INFO("Erase bonds!");

        err_code = pm_peers_delete();
        APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting advertising.
 */
void advertising_start(bool b_whitelist)
{
        ret_code_t ret = NRF_SUCCESS;
        if (b_whitelist)
        {
                memset(m_whitelist_peers, PM_PEER_ID_INVALID, sizeof(m_whitelist_peers));
                m_whitelist_peer_cnt = (sizeof(m_whitelist_peers) / sizeof(pm_peer_id_t));

                peer_list_get(m_whitelist_peers, &m_whitelist_peer_cnt);

                ret = pm_whitelist_set(m_whitelist_peers, m_whitelist_peer_cnt);
                APP_ERROR_CHECK(ret);

                NRF_LOG_INFO("advertising_start, m_whitelist_peer_cnt = %d", m_whitelist_peer_cnt);
                if (m_whitelist_peer_cnt > 0)
                {
                        // Setup the device identies list.
                        // Some SoftDevices do not support this feature.
                        ret = pm_device_identities_list_set(m_whitelist_peers, m_whitelist_peer_cnt);
                        if (ret != NRF_ERROR_NOT_SUPPORTED)
                        {
                                APP_ERROR_CHECK(ret);
                        }
                        NRF_LOG_INFO("Advertising with whitelist");
                        bsp_board_led_on(ADVERTISING_LED);

                }
                else
                {
                        NRF_LOG_INFO("Peer record is empty. Without Whiltelist");
                        bsp_board_led_on(BONDING_LED);
                }

                ret = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
                APP_ERROR_CHECK(ret);
        }
        else
        {
                //if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
                {
                        NRF_LOG_INFO("Restart the advertising without whitelist");
                        ret = ble_advertising_restart_without_whitelist(&m_advertising);
                        if (ret != NRF_ERROR_INVALID_STATE)
                        {
                                APP_ERROR_CHECK(ret);
                        }
                }
        }
}

/**@brief Function for handling File Data Storage events.
 *
 * @param[in] p_evt  Peer Manager event.
 * @param[in] cmd
 */
static void fds_evt_handler(fds_evt_t const * const p_evt)
{
        if (p_evt->id == FDS_EVT_GC)
        {
                NRF_LOG_DEBUG("GC completed\n");
        }
}

static void stop_advertising_bond_timer(void)
{
        uint32_t err_code = NRF_SUCCESS;
        if (advertising_bond_timer_is_running)
        {
                err_code = app_timer_stop(m_advertising_bond_timer_id);
                APP_ERROR_CHECK(err_code);
                advertising_bond_timer_is_running = false;
                m_bond_second_host_is_running = false;
        }
}

static void advertising_bond_timeout_handler(void * p_context)
{
        UNUSED_PARAMETER(p_context);

        uint32_t periph_link_cnt = ble_conn_state_n_peripherals(); // Number of peripheral links.
        if (periph_link_cnt == 1)//NRF_SDH_BLE_PERIPHERAL_LINK_COUNT)
        {
                NRF_LOG_INFO("Stop advertising for bonding!!!");
                (void) sd_ble_gap_adv_stop();
                m_bond_second_host_is_running = false;
        }
}

static void on_advertising_for_bond_request(void)
{
        uint32_t err_code = NRF_SUCCESS;

        uint32_t periph_link_cnt = ble_conn_state_n_peripherals(); // Number of peripheral links.

        if (m_bond_second_host_is_running)
                return;

        // if the device is already connected to 1 host, it would start the 2nd advertising.
        if (periph_link_cnt == 1)//NRF_SDH_BLE_PERIPHERAL_LINK_COUNT)
        {
                // Create timers.
                err_code = app_timer_create(&m_advertising_bond_timer_id,
                                            APP_TIMER_MODE_SINGLE_SHOT,
                                            advertising_bond_timeout_handler);
                APP_ERROR_CHECK(err_code);

                // Start application timers.
                err_code = app_timer_start(m_advertising_bond_timer_id, ADVERTISING_BOND_TIME_INTERVAL, NULL);
                APP_ERROR_CHECK(err_code);

                advertising_bond_timer_is_running = true;

                m_bond_second_host_is_running = true;

                NRF_LOG_INFO("Press button BONDING_BUTTON");
                NRF_LOG_INFO("Start Advertising bonding for 2nd host!!");
                advertising_start(false);
        }
}

/**@brief Function for clearing other bond records in main loop
 *
 * @details This function is called when a pairing/bonding has just been successful.
 */
static void on_bonded (pm_peer_id_t const * p_handle, uint16_t event_size)
{
        uint32_t err_code;
        uint32_t n_peer, i;
        pm_peer_id_t peer_id, peer_id_prev = PM_PEER_ID_INVALID;

        n_peer = pm_peer_count();

        NRF_LOG_INFO("on_bonded: # peer %d, delete all except peer id = %d", n_peer, m_bonded_peer_id);

        for (i = 0; i < n_peer; i++)
        {
                peer_id      = pm_next_peer_id_get (peer_id_prev);
                peer_id_prev = peer_id;

                if (peer_id != PM_PEER_ID_INVALID && peer_id != m_bonded_peer_id)
                {
                        err_code = pm_peer_delete(peer_id);
                        APP_ERROR_CHECK(err_code);
                }
        }

        // Stop the advertising bonding timer
        stop_advertising_bond_timer();

        uint32_t periph_link_cnt = ble_conn_state_n_peripherals();   // Number of peripheral links.
        if (periph_link_cnt  == NRF_SDH_BLE_PERIPHERAL_LINK_COUNT)
        {
                bsp_board_led_off(BONDING_LED);
                NRF_LOG_INFO("Disconnect the original connection handle %d with Host A", m_conn_handle);
                err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
                APP_ERROR_CHECK(err_code);
        }
}


/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
        ret_code_t err_code;

        //NRF_LOG_DEBUG("pm_evt_handler, evt_id = %x", p_evt->evt_id);

        switch (p_evt->evt_id)
        {
        case PM_EVT_BONDED_PEER_CONNECTED:
        {
                NRF_LOG_INFO("PM_EVT_BONDED_PEER_CONNECTED");
                NRF_LOG_INFO("Connected to a previously bonded device.");
        } break;

        case PM_EVT_CONN_SEC_SUCCEEDED:
        {
                NRF_LOG_INFO("PM_EVT_CONN_SEC_SUCCEEDED");
                NRF_LOG_INFO("Connection secured: role: %d, conn_handle: 0x%x, procedure: %d.",
                             ble_conn_state_role(p_evt->conn_handle),
                             p_evt->conn_handle,
                             p_evt->params.conn_sec_succeeded.procedure);

                m_peer_id = p_evt->peer_id;

                switch (p_evt->params.conn_sec_succeeded.procedure)
                {
                case PM_LINK_SECURED_PROCEDURE_ENCRYPTION:
                        NRF_LOG_INFO("PM_LINK_SECURED_PROCEDURE_ENCRYPTION succeed.\r\n");

                        break;

                case PM_LINK_SECURED_PROCEDURE_BONDING:
                        NRF_LOG_INFO("PM_LINK_SECURED_PROCEDURE_BONDING succeed: Bonding has been successful.\r\n");

                        // // New bond. Clear the old ones.
                        m_bonded_peer_id = p_evt->peer_id;
                        err_code = app_sched_event_put ((void *)&m_bonded_peer_id, sizeof(pm_peer_id_t), (app_sched_event_handler_t)on_bonded);
                        APP_ERROR_CHECK(err_code);

                        break;

                case PM_LINK_SECURED_PROCEDURE_PAIRING:
                        NRF_LOG_INFO("PM_LINK_SECURED_PROCEDURE_PAIRING succeed.\r\n");
                        break;

                default:
                        break;
                }


        } break;

        case PM_EVT_CONN_SEC_FAILED:
        {
                /* Often, when securing fails, it shouldn't be restarted, for security reasons.
                 * Other times, it can be restarted directly.
                 * Sometimes it can be restarted, but only after changing some Security Parameters.
                 * Sometimes, it cannot be restarted until the link is disconnected and reconnected.
                 * Sometimes it is impossible, to secure the link, or the peer device does not support it.
                 * How to handle this error is highly application dependent. */

                NRF_LOG_INFO("PM_EVT_CONN_SEC_FAILED");
        } break;

        case PM_EVT_CONN_SEC_CONFIG_REQ:
        {
                // Reject pairing request from an already bonded peer.
                pm_conn_sec_config_t conn_sec_config = {.allow_repairing = false};
                pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);

                NRF_LOG_INFO("PM_EVT_CONN_SEC_CONFIG_REQ");
        } break;

        case PM_EVT_STORAGE_FULL:
        {
                // Run garbage collection on the flash.
                err_code = fds_gc();
                if (err_code == FDS_ERR_BUSY || err_code == FDS_ERR_NO_SPACE_IN_QUEUES)
                {
                        // Retry.
                }
                else
                {
                        APP_ERROR_CHECK(err_code);
                }
        } break;
        case PM_EVT_PEER_DELETE_SUCCEEDED:
        {

                NRF_LOG_INFO("PM_EVT_PEERS_DELETE_SUCCEEDED");

        }
        break;
        case PM_EVT_PEERS_DELETE_SUCCEEDED:
        {

                NRF_LOG_INFO("PM_EVT_PEERS_DELETE_SUCCEEDED");
                //advertising_start(true);
                err_code = ble_advertising_restart_without_whitelist(&m_advertising);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                        APP_ERROR_CHECK(err_code);
                }

        } break;

        case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED:
        {
                // The local database has likely changed, send service changed indications.
                pm_local_database_has_changed();
        } break;

        case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
        {
                NRF_LOG_INFO("PM_EVT_PEER_DATA_UPDATE_SUCCEEDED");
                if (     p_evt->params.peer_data_update_succeeded.flash_changed
                         && (p_evt->params.peer_data_update_succeeded.data_id == PM_PEER_DATA_ID_BONDING))
                {
                        NRF_LOG_INFO("New Bond, add the peer to the whitelist if possible");
                        NRF_LOG_INFO("\tm_whitelist_peer_cnt %d, MAX_PEERS_WLIST %d",
                                     m_whitelist_peer_cnt + 1,
                                     BLE_GAP_WHITELIST_ADDR_MAX_COUNT);


                        if (m_whitelist_peer_cnt < BLE_GAP_WHITELIST_ADDR_MAX_COUNT)
                        {
                                memset(m_whitelist_peers, PM_PEER_ID_INVALID, sizeof(m_whitelist_peers));
                                m_whitelist_peer_cnt = (sizeof(m_whitelist_peers) / sizeof(pm_peer_id_t));

                                peer_list_get(m_whitelist_peers, &m_whitelist_peer_cnt);

                                err_code = pm_whitelist_set(m_whitelist_peers, m_whitelist_peer_cnt);
                                APP_ERROR_CHECK(err_code);

                                if (m_whitelist_peer_cnt > 0)
                                {
                                        // Setup the device identies list.
                                        // Some SoftDevices do not support this feature.
                                        err_code = pm_device_identities_list_set(m_whitelist_peers, m_whitelist_peer_cnt);
                                        if (err_code != NRF_ERROR_NOT_SUPPORTED)
                                        {
                                                APP_ERROR_CHECK(err_code);
                                        }
                                        NRF_LOG_INFO("Advertising with whitelist");
                                }

                        }



                }
        } break;

        case PM_EVT_PEER_DATA_UPDATE_FAILED:
        {
                // Assert.
                APP_ERROR_CHECK(p_evt->params.peer_data_update_failed.error);
        } break;

        case PM_EVT_PEER_DELETE_FAILED:
        {
                // Assert.
                APP_ERROR_CHECK(p_evt->params.peer_delete_failed.error);
        } break;

        case PM_EVT_PEERS_DELETE_FAILED:
        {
                // Assert.
                APP_ERROR_CHECK(p_evt->params.peers_delete_failed_evt.error);
        } break;

        case PM_EVT_ERROR_UNEXPECTED:
        {
                // Assert.
                APP_ERROR_CHECK(p_evt->params.error_unexpected.error);
        } break;

        case PM_EVT_CONN_SEC_START:

        case PM_EVT_LOCAL_DB_CACHE_APPLIED:
        case PM_EVT_SERVICE_CHANGED_IND_SENT:
        case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED:
        default:
                break;
        }
}



/**@brief Function for performing battery measurement and updating the Battery Level characteristic
 *        in Battery Service.
 */
static void battery_level_update(void)
{
        ret_code_t err_code;
        uint8_t battery_level;

        battery_level = (uint8_t)sensorsim_measure(&m_battery_sim_state, &m_battery_sim_cfg);

        err_code = ble_bas_battery_level_update(&m_bas, battery_level);
        if ((err_code != NRF_SUCCESS) &&
            (err_code != NRF_ERROR_INVALID_STATE) &&
            (err_code != NRF_ERROR_RESOURCES) &&
            (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
            )
        {
                APP_ERROR_HANDLER(err_code);
        }
}


/**@brief Function for handling the Battery measurement timer timeout.
 *
 * @details This function will be called each time the battery level measurement timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
static void battery_level_meas_timeout_handler(void * p_context)
{
        UNUSED_PARAMETER(p_context);
        battery_level_update();
}


/**@brief Function for handling the Heart rate measurement timer timeout.
 *
 * @details This function will be called each time the heart rate measurement timer expires.
 *          It will exclude RR Interval data from every third measurement.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
static void heart_rate_meas_timeout_handler(void * p_context)
{
        static uint32_t cnt = 0;
        ret_code_t err_code;
        uint16_t heart_rate;

        UNUSED_PARAMETER(p_context);

        heart_rate = (uint16_t)sensorsim_measure(&m_heart_rate_sim_state, &m_heart_rate_sim_cfg);

        cnt++;
        err_code = ble_hrs_heart_rate_measurement_send(&m_hrs, heart_rate);
        if ((err_code != NRF_SUCCESS) &&
            (err_code != NRF_ERROR_INVALID_STATE) &&
            (err_code != NRF_ERROR_RESOURCES) &&
            (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
            )
        {
                APP_ERROR_HANDLER(err_code);
        }

        // Disable RR Interval recording every third heart rate measurement.
        // NOTE: An application will normally not do this. It is done here just for testing generation
        // of messages without RR Interval measurements.
        m_rr_interval_enabled = ((cnt % 3) != 0);
}


/**@brief Function for handling the RR interval timer timeout.
 *
 * @details This function will be called each time the RR interval timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
static void rr_interval_timeout_handler(void * p_context)
{
        UNUSED_PARAMETER(p_context);

        if (m_rr_interval_enabled)
        {
                uint16_t rr_interval;

                rr_interval = (uint16_t)sensorsim_measure(&m_rr_interval_sim_state,
                                                          &m_rr_interval_sim_cfg);
                ble_hrs_rr_interval_add(&m_hrs, rr_interval);
                rr_interval = (uint16_t)sensorsim_measure(&m_rr_interval_sim_state,
                                                          &m_rr_interval_sim_cfg);
                ble_hrs_rr_interval_add(&m_hrs, rr_interval);
                rr_interval = (uint16_t)sensorsim_measure(&m_rr_interval_sim_state,
                                                          &m_rr_interval_sim_cfg);
                ble_hrs_rr_interval_add(&m_hrs, rr_interval);
                rr_interval = (uint16_t)sensorsim_measure(&m_rr_interval_sim_state,
                                                          &m_rr_interval_sim_cfg);
                ble_hrs_rr_interval_add(&m_hrs, rr_interval);
                rr_interval = (uint16_t)sensorsim_measure(&m_rr_interval_sim_state,
                                                          &m_rr_interval_sim_cfg);
                ble_hrs_rr_interval_add(&m_hrs, rr_interval);
                rr_interval = (uint16_t)sensorsim_measure(&m_rr_interval_sim_state,
                                                          &m_rr_interval_sim_cfg);
                ble_hrs_rr_interval_add(&m_hrs, rr_interval);
        }
}


/**@brief Function for handling the Sensor Contact Detected timer timeout.
 *
 * @details This function will be called each time the Sensor Contact Detected timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
static void sensor_contact_detected_timeout_handler(void * p_context)
{
        static bool sensor_contact_detected = false;

        UNUSED_PARAMETER(p_context);

        sensor_contact_detected = !sensor_contact_detected;
        ble_hrs_sensor_contact_detected_update(&m_hrs, sensor_contact_detected);
}


/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
        ret_code_t err_code;

        // Initialize timer module.
        err_code = app_timer_init();
        APP_ERROR_CHECK(err_code);

        // Create timers.
        err_code = app_timer_create(&m_battery_timer_id,
                                    APP_TIMER_MODE_REPEATED,
                                    battery_level_meas_timeout_handler);
        APP_ERROR_CHECK(err_code);

        err_code = app_timer_create(&m_heart_rate_timer_id,
                                    APP_TIMER_MODE_REPEATED,
                                    heart_rate_meas_timeout_handler);
        APP_ERROR_CHECK(err_code);

        err_code = app_timer_create(&m_rr_interval_timer_id,
                                    APP_TIMER_MODE_REPEATED,
                                    rr_interval_timeout_handler);
        APP_ERROR_CHECK(err_code);

        err_code = app_timer_create(&m_sensor_contact_timer_id,
                                    APP_TIMER_MODE_REPEATED,
                                    sensor_contact_detected_timeout_handler);
        APP_ERROR_CHECK(err_code);
}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
        ret_code_t err_code;
        ble_gap_conn_params_t gap_conn_params;
        ble_gap_conn_sec_mode_t sec_mode;

        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

        err_code = sd_ble_gap_device_name_set(&sec_mode,
                                              (const uint8_t *)DEVICE_NAME,
                                              strlen(DEVICE_NAME));
        APP_ERROR_CHECK(err_code);

        err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_HEART_RATE_SENSOR_HEART_RATE_BELT);
        APP_ERROR_CHECK(err_code);

        memset(&gap_conn_params, 0, sizeof(gap_conn_params));

        gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
        gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
        gap_conn_params.slave_latency     = SLAVE_LATENCY;
        gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

        err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
        APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling events from the GATT library. */
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
        NRF_LOG_INFO("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                     p_gatt->att_mtu_desired_central,
                     p_gatt->att_mtu_desired_periph);
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
        ret_code_t err_code;

        err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
        APP_ERROR_CHECK(err_code);

        err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
        APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing services that will be used by the application.
 *
 * @details Initialize the Heart Rate, Battery and Device Information services.
 */
static void services_init(void)
{
        ret_code_t err_code;
        ble_hrs_init_t hrs_init;
        ble_bas_init_t bas_init;
        ble_dis_init_t dis_init;
        uint8_t body_sensor_location;

        // Initialize Heart Rate Service.
        body_sensor_location = BLE_HRS_BODY_SENSOR_LOCATION_FINGER;

        memset(&hrs_init, 0, sizeof(hrs_init));

        hrs_init.evt_handler                 = NULL;
        hrs_init.is_sensor_contact_supported = true;
        hrs_init.p_body_sensor_location      = &body_sensor_location;

        // Here the sec level for the Heart Rate Service can be changed/increased.
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&hrs_init.hrs_hrm_attr_md.cccd_write_perm);
        BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hrs_init.hrs_hrm_attr_md.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hrs_init.hrs_hrm_attr_md.write_perm);

        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&hrs_init.hrs_bsl_attr_md.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hrs_init.hrs_bsl_attr_md.write_perm);

        err_code = ble_hrs_init(&m_hrs, &hrs_init);
        APP_ERROR_CHECK(err_code);

        // Initialize Battery Service.
        memset(&bas_init, 0, sizeof(bas_init));

        // Here the sec level for the Battery Service can be changed/increased.
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_char_attr_md.cccd_write_perm);
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_char_attr_md.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&bas_init.battery_level_char_attr_md.write_perm);

        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_report_read_perm);

        bas_init.evt_handler          = NULL;
        bas_init.support_notification = true;
        bas_init.p_report_ref         = NULL;
        bas_init.initial_batt_level   = 100;

        err_code = ble_bas_init(&m_bas, &bas_init);
        APP_ERROR_CHECK(err_code);

        // Initialize Device Information Service.
        memset(&dis_init, 0, sizeof(dis_init));

        ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, (char *)MANUFACTURER_NAME);

        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&dis_init.dis_attr_md.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&dis_init.dis_attr_md.write_perm);

        err_code = ble_dis_init(&dis_init);
        APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the sensor simulators.
 */
static void sensor_simulator_init(void)
{
        m_battery_sim_cfg.min          = MIN_BATTERY_LEVEL;
        m_battery_sim_cfg.max          = MAX_BATTERY_LEVEL;
        m_battery_sim_cfg.incr         = BATTERY_LEVEL_INCREMENT;
        m_battery_sim_cfg.start_at_max = true;

        sensorsim_init(&m_battery_sim_state, &m_battery_sim_cfg);

        m_heart_rate_sim_cfg.min          = MIN_HEART_RATE;
        m_heart_rate_sim_cfg.max          = MAX_HEART_RATE;
        m_heart_rate_sim_cfg.incr         = HEART_RATE_INCREMENT;
        m_heart_rate_sim_cfg.start_at_max = false;

        sensorsim_init(&m_heart_rate_sim_state, &m_heart_rate_sim_cfg);

        m_rr_interval_sim_cfg.min          = MIN_RR_INTERVAL;
        m_rr_interval_sim_cfg.max          = MAX_RR_INTERVAL;
        m_rr_interval_sim_cfg.incr         = RR_INTERVAL_INCREMENT;
        m_rr_interval_sim_cfg.start_at_max = false;

        sensorsim_init(&m_rr_interval_sim_state, &m_rr_interval_sim_cfg);
}


/**@brief Function for starting application timers.
 */
static void application_timers_start(void)
{
        ret_code_t err_code;

        // Start application timers.
        err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
        APP_ERROR_CHECK(err_code);

        err_code = app_timer_start(m_heart_rate_timer_id, HEART_RATE_MEAS_INTERVAL, NULL);
        APP_ERROR_CHECK(err_code);

        err_code = app_timer_start(m_rr_interval_timer_id, RR_INTERVAL_INTERVAL, NULL);
        APP_ERROR_CHECK(err_code);

        err_code = app_timer_start(m_sensor_contact_timer_id, SENSOR_CONTACT_DETECTED_INTERVAL, NULL);
        APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
        ret_code_t err_code;

        if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
        {
                err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
                APP_ERROR_CHECK(err_code);
        }
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
        APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
        ret_code_t err_code;
        ble_conn_params_init_t cp_init;

        memset(&cp_init, 0, sizeof(cp_init));

        cp_init.p_conn_params                  = NULL;
        cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
        cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
        cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
        cp_init.start_on_notify_cccd_handle    = m_hrs.hrm_handles.cccd_handle;
        cp_init.disconnect_on_fail             = false;
        cp_init.evt_handler                    = on_conn_params_evt;
        cp_init.error_handler                  = conn_params_error_handler;

        err_code = ble_conn_params_init(&cp_init);
        APP_ERROR_CHECK(err_code);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
        ret_code_t err_code;

        err_code = bsp_indication_set(BSP_INDICATE_IDLE);
        APP_ERROR_CHECK(err_code);

        // Prepare wakeup buttons.
        err_code = bsp_btn_ble_sleep_mode_prepare();
        APP_ERROR_CHECK(err_code);

        // Go to system-off mode (this function will not return; wakeup will cause a reset).
        err_code = sd_power_system_off();
        APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
        ret_code_t ret;

        switch (ble_adv_evt)
        {
        case BLE_ADV_EVT_FAST:
                NRF_LOG_INFO("Fast advertising.");
                ret = bsp_indication_set(BSP_INDICATE_ADVERTISING);
                APP_ERROR_CHECK(ret);
                break;

        case BLE_ADV_EVT_IDLE:
//                sleep_mode_enter();
                break;

        case BLE_ADV_EVT_FAST_WHITELIST:
                NRF_LOG_INFO("Fast advertising with Whitelist");
                ret = bsp_indication_set(BSP_INDICATE_ADVERTISING_WHITELIST);
                APP_ERROR_CHECK(ret);
                break;


        case BLE_ADV_EVT_WHITELIST_REQUEST:
        {
                ble_gap_addr_t whitelist_addrs[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
                ble_gap_irk_t whitelist_irks[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
                uint32_t addr_cnt = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;
                uint32_t irk_cnt  = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

                ret = pm_whitelist_get(whitelist_addrs, &addr_cnt, whitelist_irks, &irk_cnt);
                APP_ERROR_CHECK(ret);
                NRF_LOG_INFO("pm_whitelist_get returns %d addr in whitelist and %d irk whitelist",
                             addr_cnt,
                             irk_cnt);

                // Apply the whitelist.
                ret = ble_advertising_whitelist_reply(&m_advertising,
                                                      whitelist_addrs,
                                                      addr_cnt,
                                                      whitelist_irks,
                                                      irk_cnt);
                APP_ERROR_CHECK(ret);
        }
        break;

        default:
                break;
        }
}




/**@brief Function for handling the Connected event.
 *
 * @param[in] p_gap_evt GAP event received from the BLE stack.
 */
static void on_connected(const ble_gap_evt_t * const p_gap_evt)
{
        ret_code_t err_code;
        uint32_t periph_link_cnt = ble_conn_state_n_peripherals(); // Number of peripheral links.

        NRF_LOG_INFO("Connection with link 0x%x established.", p_gap_evt->conn_handle);

        // Update LEDs
        // first device connection
        if (periph_link_cnt == 1)
        {
                m_conn_handle  = p_gap_evt->conn_handle;
                bsp_board_led_off(ADVERTISING_LED);
                bsp_board_led_off(BONDING_LED);
                bsp_board_led_on(CONNECTED_LED);
        }
        else if (periph_link_cnt == NRF_SDH_BLE_PERIPHERAL_LINK_COUNT)
        {
                bsp_board_led_off(BONDING_LED);
                bsp_board_led_on(CONNECTED_2_LED);
        }

}

/**@brief Function for handling the Disconnected event.
 *
 * @param[in] p_gap_evt GAP event received from the BLE stack.
 */
static void on_disconnected(ble_gap_evt_t const * const p_gap_evt)
{
        ret_code_t err_code;
        uint32_t periph_link_cnt = ble_conn_state_n_peripherals(); // Number of peripheral links.

        NRF_LOG_INFO("Connection 0x%x has been disconnected. Reason: 0x%X",
                     p_gap_evt->conn_handle,
                     p_gap_evt->params.disconnected.reason);

        if (m_conn_handle == p_gap_evt->conn_handle)
        {
                bsp_board_led_off(CONNECTED_LED);
                m_conn_handle = BLE_CONN_HANDLE_INVALID;
        }

        if (periph_link_cnt == 0)
        {
                advertising_start(true);
                bsp_board_led_off(CONNECTED_LED);
                bsp_board_led_off(CONNECTED_2_LED);
        }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
        ret_code_t err_code;

        switch (p_ble_evt->header.evt_id)
        {
        case BLE_GAP_EVT_CONNECTED:
                NRF_LOG_INFO("Connected.");
                on_connected(&p_ble_evt->evt.gap_evt);
                break;

        case BLE_GAP_EVT_DISCONNECTED:
                NRF_LOG_INFO("Disconnected.");
                on_disconnected(&p_ble_evt->evt.gap_evt);
                break;

#ifndef S140
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
                NRF_LOG_DEBUG("PHY update request.");
                ble_gap_phys_t const phys =
                {
                        .rx_phys = BLE_GAP_PHY_AUTO,
                        .tx_phys = BLE_GAP_PHY_AUTO,
                };
                err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
                APP_ERROR_CHECK(err_code);
        } break;
#endif

#if !defined (S112)
        case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST:
        {
                ble_gap_data_length_params_t dl_params;

                // Clearing the struct will effectivly set members to @ref BLE_GAP_DATA_LENGTH_AUTO
                memset(&dl_params, 0, sizeof(ble_gap_data_length_params_t));
                err_code = sd_ble_gap_data_length_update(p_ble_evt->evt.gap_evt.conn_handle, &dl_params, NULL);
                APP_ERROR_CHECK(err_code);
        } break;
#endif //!defined (S112)

        case BLE_GATTC_EVT_TIMEOUT:
                // Disconnect on GATT Client timeout event.
                NRF_LOG_DEBUG("GATT Client Timeout.");
                err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                                 BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
                APP_ERROR_CHECK(err_code);
                break;

        case BLE_GATTS_EVT_TIMEOUT:
                // Disconnect on GATT Server timeout event.
                NRF_LOG_DEBUG("GATT Server Timeout.");
                err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                                 BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
                APP_ERROR_CHECK(err_code);
                break;

        case BLE_EVT_USER_MEM_REQUEST:
                //err_code = sd_ble_user_mem_reply(m_conn_handle, NULL);
                err_code = sd_ble_user_mem_reply(p_ble_evt->evt.gattc_evt.conn_handle, NULL);
                APP_ERROR_CHECK(err_code);
                break;

        case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
        {
                ble_gatts_evt_rw_authorize_request_t req;
                ble_gatts_rw_authorize_reply_params_t auth_reply;

                req = p_ble_evt->evt.gatts_evt.params.authorize_request;

                if (req.type != BLE_GATTS_AUTHORIZE_TYPE_INVALID)
                {
                        if ((req.request.write.op == BLE_GATTS_OP_PREP_WRITE_REQ)     ||
                            (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) ||
                            (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL))
                        {
                                if (req.type == BLE_GATTS_AUTHORIZE_TYPE_WRITE)
                                {
                                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
                                }
                                else
                                {
                                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
                                }
                                auth_reply.params.write.gatt_status = APP_FEATURE_NOT_SUPPORTED;
                                err_code = sd_ble_gatts_rw_authorize_reply(p_ble_evt->evt.gatts_evt.conn_handle,
                                                                           &auth_reply);
                                APP_ERROR_CHECK(err_code);
                        }
                }
        } break;


        default:
                // No implementation needed.
                break;
        }
}



/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
        ret_code_t err_code;

        err_code = nrf_sdh_enable_request();
        APP_ERROR_CHECK(err_code);

        // Configure the BLE stack using the default settings.
        // Fetch the start address of the application RAM.
        uint32_t ram_start = 0;
        err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
        APP_ERROR_CHECK(err_code);

        // Enable BLE stack.
        err_code = nrf_sdh_ble_enable(&ram_start);
        APP_ERROR_CHECK(err_code);

        // Register a handler for BLE events.
        NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for the Event Scheduler initialization.
 */
static void scheduler_init(void)
{
        APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}


/**@brief Function for handling events from the button handler module.
 *
 * @param[in] pin_no        The pin that the event applies to.
 * @param[in] button_action The button action (press/release).
 */
static void button_event_handler(uint8_t pin_no, uint8_t button_action)
{
        ret_code_t err_code;

        switch (pin_no)
        {
        case REMOVE_BOND_BUTTON:
                if (button_action == APP_BUTTON_PUSH)
                {
                        NRF_LOG_INFO("Press button to remove all the bonding record");
                        delete_bonds();
                }

                break;
        case BONDING_BUTTON:
                if (button_action == APP_BUTTON_PUSH)
                {
                        NRF_LOG_INFO("Press bond button to start the advertising without whitelist");
                        on_advertising_for_bond_request();
                }
                break;
        default:

                break;
        }
}



/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
        ble_gap_sec_params_t sec_param;
        ret_code_t err_code;

        err_code = pm_init();
        APP_ERROR_CHECK(err_code);

        memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

        // Security parameters to be used for all security procedures.
        sec_param.bond           = SEC_PARAM_BOND;
        sec_param.mitm           = SEC_PARAM_MITM;
        sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
        sec_param.oob            = SEC_PARAM_OOB;
        sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
        sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
        sec_param.kdist_own.enc  = 1;
        sec_param.kdist_own.id   = 1;
        sec_param.kdist_peer.enc = 1;
        sec_param.kdist_peer.id  = 1;

        err_code = pm_sec_params_set(&sec_param);
        APP_ERROR_CHECK(err_code);

        err_code = pm_register(pm_evt_handler);
        APP_ERROR_CHECK(err_code);

        err_code = fds_register(fds_evt_handler);
        APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
        ret_code_t err_code;
        ble_advertising_init_t init;
        uint8_t adv_flags;

        memset(&init, 0, sizeof(init));

        adv_flags                            = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

        init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
        init.advdata.include_appearance      = true;
        init.advdata.flags                   = adv_flags;
        init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
        init.advdata.uuids_complete.p_uuids  = m_adv_uuids;

        init.config.ble_adv_whitelist_enabled      = true;
        init.config.ble_adv_fast_enabled           = true;
        init.config.ble_adv_fast_interval          = APP_ADV_FAST_INTERVAL;
        init.config.ble_adv_fast_timeout           = APP_ADV_FAST_TIMEOUT;
        init.config.ble_adv_slow_enabled           = true;
        init.config.ble_adv_slow_interval          = APP_ADV_SLOW_INTERVAL;
        init.config.ble_adv_slow_timeout           = APP_ADV_SLOW_TIMEOUT;
        init.config.ble_adv_on_disconnect_disabled = true;

        init.evt_handler = on_adv_evt;

        err_code = ble_advertising_init(&m_advertising, &init);
        APP_ERROR_CHECK(err_code);

        ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}



/**@brief Function for initializing the button handler module.
 */
static void buttons_init(void)
{
        ret_code_t err_code;

        //The array must be static because a pointer to it will be saved in the button handler module.
        static app_button_cfg_t buttons[] =
        {
                {REMOVE_BOND_BUTTON, false, BUTTON_PULL, button_event_handler},
                {BONDING_BUTTON, false, BUTTON_PULL, button_event_handler},
        };

        err_code = app_button_init(buttons, sizeof(buttons) / sizeof(buttons[0]),
                                   BUTTON_DETECTION_DELAY);
        APP_ERROR_CHECK(err_code);
}



/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
        ret_code_t err_code = NRF_LOG_INIT(NULL);
        APP_ERROR_CHECK(err_code);

        NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
        ret_code_t err_code = sd_app_evt_wait();
        APP_ERROR_CHECK(err_code);
}


/**@brief Function for application main entry.
 */
int main(void)
{
        ret_code_t err_code;

        // Initialize.
        log_init();
        timers_init();
        buttons_init();
        ble_stack_init();
        scheduler_init();
        gap_params_init();
        gatt_init();
        advertising_init();
        services_init();
        sensor_simulator_init();
        conn_params_init();
        peer_manager_init();

        // Start execution.
        NRF_LOG_INFO("Heart Rate Sensor example started.");
        application_timers_start();

        // Start the advertising
        advertising_start(true);

        // Enable the button
        err_code = app_button_enable();
        APP_ERROR_CHECK(err_code);

        advertising_bond_timer_is_running = false;

        // Enter main loop.
        for (;;)
        {
                app_sched_execute();
                if (NRF_LOG_PROCESS() == false)
                {
                        power_manage();
                }
        }
}
