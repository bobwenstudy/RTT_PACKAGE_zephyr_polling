
#ifndef APPLICATIONS_HCI_INTERFACE_H_
#define APPLICATIONS_HCI_INTERFACE_H_

#include "chipset_interface.h"

/* SPI Trans Defines -------------------------------------------------------------------*/
#define HEADER_SIZE       5U
#define MAX_BUFFER_SIZE   255U
#define TIMEOUT_DURATION  100U
#define TIMEOUT_IRQ_HIGH  1000U

/* HCI Packet types */
#define HCI_NONE_PKT        0x00
#define HCI_COMMAND_PKT     0x01
#define HCI_ACLDATA_PKT     0x02
#define HCI_SCODATA_PKT     0x03
#define HCI_EVENT_PKT       0x04
#define HCI_ISO_PKT         0x05
#define HCI_VENDOR_PKT      0xff

/* GET PIN  -----------------------------------------------*/
#define HCI_TL_SPI_IRQ_PIN      GET_PIN(A, 0)  /* IRQ PIN OF BLUENRG*/
#define HCI_TL_SPI_CS_PIN       GET_PIN(A, 1)  /* SPI CS PIN */

#define NAME_SIZE       10U

/* Exported Functions --------------------------------------------------------*/
int32_t HCI_TL_SPI_Init    (void* pConf);
int32_t HCI_TL_SPI_Receive (uint8_t* buffer, uint16_t size);
int32_t HCI_TL_SPI_Send    (uint8_t* buffer, uint16_t size);
int hci_driver_init(bt_BlueNRG_SPI_interface_t *, int, int);

#endif /* PACKAGES_ZEPHYR_POLLING_LATEST_PLATFORM_RTTHREAD_RTTHREAD_DRIVER_SPI_H_ */
