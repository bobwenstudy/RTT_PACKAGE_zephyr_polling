#include <stddef.h>
#include <stdio.h>

#include "base/common.h"

#include <drivers/hci_driver.h>
#include "btp/btp.h"

#define LOG_MODULE_NAME tester
#include "logging/bt_log.h"

bool socket_connected = false;

void bt_ready(int err)
{
    if (err)
    {
        BT_INFO("Bluetooth init failed (err %d)\n", err);
        return;
    }
    BT_INFO("Bluetooth initialized\n");

    tester_init();
//
//    //Socket initialization for BTP communication
//    socket_connected = tester_socket_init();
//    if (socket_connected){
//        BT_INFO("Start to initialize BTP.\n");
//        tester_init_core();
//        int ret = tester_btp_pkt_send(BTP_SERVICE_ID_CORE, BTP_CORE_EV_IUT_READY, BTP_INDEX_NONE,NULL, 0);
//        if (ret < 0){
//            BT_INFO("IUT ready event failed\n");
//            return;
//        }
//    }
//
//    BT_INFO("Sent IUT ready event\n");
}



void app_polling_work(void)
{
    //Read socket and Process
//    if (socket_connected) {
//        testr_btp_pkt_process();
//    } else{
//        socket_connected = tester_socket_init();
//        if (socket_connected){
//            tester_init();
//            int ret = tester_btp_pkt_send(BTP_SERVICE_ID_CORE, BTP_CORE_EV_IUT_READY, BTP_INDEX_NONE,NULL, 0);
//            if(!ret){
//                BT_INFO("Sent IUT ready event\n");
//            }
//        }
//    }
    tester_polling_work();
}
