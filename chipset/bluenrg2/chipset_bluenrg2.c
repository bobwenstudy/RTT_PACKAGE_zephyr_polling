#include <errno.h>

#include "chipset_bluenrg2.h"
#include <logging/bt_log_impl.h>

#define STATE_POLLING_NONE      0
#define STATE_POLLING_BOOTING   1
#define STATE_POLLING_PREPARING 2

int state;
int step;

#define CONFIG_DATA_LL_WITHOUT_HOST         (0x2C) /**< Switch on/off Link Layer only mode. Set to 1 to disable Host. */
#define CONFIG_DATA_LL_WITHOUT_HOST_LEN     (1)

static int bluenrg2_config_without_host()
{
    uint8_t cmd_buffer[CONFIG_DATA_LL_WITHOUT_HOST_LEN + 2];
    struct net_buf *buf;

    cmd_buffer[0] = CONFIG_DATA_LL_WITHOUT_HOST;                //offset
    cmd_buffer[1] = CONFIG_DATA_LL_WITHOUT_HOST_LEN;            //config len
    cmd_buffer[2] = 1;                                          //Set to 1 to disable Host

    uint16_t ogf = 0x3f, ocf = 0x00c;
    uint16_t opcode = (uint16_t)((ocf & 0x03ff)|(ogf << 10));

    buf = bt_hci_cmd_create(opcode, sizeof(cmd_buffer));
    if (!buf)
    {
        return -ENOBUFS;
    }
    net_buf_add_mem(buf, cmd_buffer, sizeof(cmd_buffer));

    return bt_hci_cmd_send(opcode, buf);
}

#define BLE_MAC_ADDR                                                                               \
    {                                                                                              \
        {                                                                                          \
            0xf5, 0x00, 0x00, 0xE1, 0x80, 0x02                                                   \
        }                                                                                          \
    }
#define CONFIG_DATA_PUBADDR_OFFSET          (0x00) /**< Bluetooth public address */
#define CONFIG_DATA_PUBADDR_LEN             (6)

static int bluenrg2_config_set_public_addr()
{
    //uint8_t cmd_buffer[258];
    uint8_t cmd_buffer[CONFIG_DATA_PUBADDR_LEN + 2];
    struct net_buf *buf;
    bt_addr_t addr = BLE_MAC_ADDR;

    cmd_buffer[0] = CONFIG_DATA_PUBADDR_OFFSET;                 //offset
    cmd_buffer[1] = CONFIG_DATA_PUBADDR_LEN;                    //config len
    memcpy(cmd_buffer + 2, addr.val, CONFIG_DATA_PUBADDR_LEN);   // addr

    uint16_t ogf = 0x3f, ocf = 0x00c;
    uint16_t opcode = (uint16_t)((ocf & 0x03ff)|(ogf << 10));

    buf = bt_hci_cmd_create(opcode, sizeof(cmd_buffer));
    if (!buf)
    {
        return -ENOBUFS;
    }
    net_buf_add_mem(buf, cmd_buffer, sizeof(cmd_buffer));

    return bt_hci_cmd_send(opcode, buf);
}

/**
 * @brief This command sets the TX power level of the device. By controlling the
 *        EN_HIGH_POWER and the PA_LEVEL, the combination of the 2 determines
 *        the output power level (dBm).  When the system starts up or reboots,
 *        the default TX power level will be used, which is the maximum value of
 *        8 dBm. Once this command is given, the output power will be changed
 *        instantly, regardless if there is Bluetooth communication going on or
 *        not. For example, for debugging purpose, the device can be set to
 *        advertise all the time. And use this command to observe the signal
 *        strength changing. The system will keep the last received TX power
 *        level from the command, i.e. the 2nd command overwrites the previous
 *        TX power level. The new TX power level remains until another Set TX
 *        Power command, or the system reboots.
 * @param En_High_Power Enable High Power mode.  High power mode should be
 *        enabled only to reach the maximum output power.
 *        Values:
 *        - 0x00: Normal Power
 *        - 0x01: High Power
 * @param PA_Level Power amplifier output level. The allowed PA levels depends
 *        on the device (see user manual to know wich output power is expected
 *        at a given PA level). The obtained output power can also depend on PCB
 *        layout and associated components.
 *        Values:
 *        - 0: -14 dBm (High Power)
 *        - 1: -11 dBm (High Power)
 *        - 2: -8 dBm (High Power)
 *        - 3: -5 dBm (High Power)
 *        - 4: -2 dBm (High Power)
 *        - 5: 2 dBm (High Power)
 *        - 6: 4 dBm (High Power)
 *        - 7: 8 dBm (High Power)
 * @retval Value indicating success or error code.
 */
static int bluenrg2_set_tx_power_level(uint8_t En_High_Power, uint8_t PA_Level)
{
    //uint8_t cmd_buffer[258];
    uint8_t cmd_buffer[2];
    struct net_buf *buf;

    cmd_buffer[0] = En_High_Power;      //En_High_Power
    cmd_buffer[1] = PA_Level;           //config PA_Level

    uint16_t ogf = 0x3f, ocf = 0x00f;
    uint16_t opcode = (uint16_t)((ocf & 0x03ff)|(ogf << 10));

    buf = bt_hci_cmd_create(opcode, sizeof(cmd_buffer));
    if (!buf)
    {
        return -ENOBUFS;
    }
    net_buf_add_mem(buf, cmd_buffer, sizeof(cmd_buffer));

    return bt_hci_cmd_send(opcode, buf);
}


static int  bluenrg2_gatt_init(void)
{
    uint16_t ogf = 0x3f, ocf = 0x101;
    uint16_t opcode = (uint16_t)((ocf & 0x03ff)|(ogf << 10));

    return bt_hci_cmd_send(opcode, NULL);
}

/**
 * @brief Initialize the GAP layer. Register the GAP service with the GATT. All
 *        the standard GAP characteristics will also be added: - Device Name -
 *        Appearance - Peripheral Preferred Connection Parameters (peripheral
 *        role only)  WARNING: A section of the Flash memory (pointed by
 *        stored_device_id_data_p) is used by this procedure. When this section
 *        is empty, data are written inside. This normally happens once during
 *        the lifetime of the device, when the command is executed for the first
 *        time (or every time it is called after that section has been erased).
 *        Do not power off the device while this function is writing into Flash
 *        memory. If the functions returns FLASH_WRITE_FAILED, it means that the
 *        flash area pointed by stored_device_id_data_p is corrupted (probably
 *        due to a power loss during the first call to aci_gap_init()). To fix
 *        the problem, that flash area has to be erased, so that the
 *        aci_gap_init() can reinitialize it correctly.
 * @param Role Bitmap of allowed roles.
 *        Flags:
 *        - 0x01: Peripheral
 *        - 0x02: Broadcaster
 *        - 0x04: Central
 *        - 0x08: Observer
 * @param privacy_enabled Specify if privacy is enabled or not and which one .
 *        Values:
 *        - 0x00: Privacy disabled
 *        - 0x01: Privacy host enabled
 *        - 0x02: Privacy controller enabled
 * @param device_name_char_len Length of the device name characteristic
 *        Values:
 *        - 0 ... 248
 * @param[out] Service_Handle Handle of the GAP service
 * @param[out] Dev_Name_Char_Handle Device Name Characteristic handle
 * @param[out] Appearance_Char_Handle Appearance Characteristic handle
 * @retval Value indicating success or error code.
 */
#define GAP_PERIPHERAL_ROLE                     (0x01)
#define GAP_BROADCASTER_ROLE                    (0x02)
#define GAP_CENTRAL_ROLE                        (0x04)
#define GAP_OBSERVER_ROLE                       (0x08)
#define privacy_enabled                         (0x00)
#define device_name_char_len                    (0x08)

static int bluenrg2_gap_init()
{
    uint8_t cmd_buffer[3];
    struct net_buf *buf;

    cmd_buffer[0] = GAP_PERIPHERAL_ROLE;    //role
    cmd_buffer[1] = privacy_enabled;        //privacy
    cmd_buffer[2] = device_name_char_len;   //device_name_char_len

    uint16_t ogf = 0x3f, ocf = 0x08a;
    uint16_t opcode = (uint16_t)((ocf & 0x03ff)|(ogf << 10));

    buf = bt_hci_cmd_create(opcode, sizeof(cmd_buffer));
    if (!buf)
    {
        return -ENOBUFS;
    }
    net_buf_add_mem(buf, cmd_buffer, sizeof(cmd_buffer));

    return bt_hci_cmd_send(opcode, buf);
}

void init_work(void){
    state = STATE_POLLING_BOOTING;
    step = 0;
}

void boot_start(void) {
    state = STATE_POLLING_BOOTING;
    // nothing to do
    bt_hci_set_boot_ready();          //finish boot
}

void prepare_start(void) {
    state = STATE_POLLING_PREPARING;
    step = 1;
    // step 1 close host
    bluenrg2_config_without_host(); // It can be written only if aci_hal_write_config_data() is the first command after reset.
}

void event_process(uint8_t event, struct net_buf *buf)
{
    if(state == STATE_POLLING_PREPARING) // boot do nothing
    {
        if (event == BT_HCI_EVT_CMD_COMPLETE)  //only complete
        {
            printk("prepare_event_process, step: %d\n", step);
            switch (step)
            {
            case 1: //close host just now
                bluenrg2_config_set_public_addr();  //step2  set_public_addr
                step = 2;
                break;
            case 2:
                bluenrg2_set_tx_power_level(1, 4);  //step3  set_public_addr
                step = 3;
                break;
            case 3:
                bluenrg2_gatt_init();               //step4  gatt_ini
                step = 4;
                break;
            case 4:
                bluenrg2_gap_init();                //step5  gap_ini
                step = 5;
                break;
            case 5:
                bt_hci_set_prepare_ready();          //finish prepare
                step = 0;
                break;
            }
        }
    }
}


static const struct bt_hci_chipset_driver chipset_drv = {
        init_work, boot_start, prepare_start, event_process,
};

// public drv API
const struct bt_hci_chipset_driver *bt_hci_chipset_impl_local_instance(void)
{
    return &chipset_drv;
}

// For test, you can set your customor setting here.
static const bt_usb_interface_t usb_interface = {0, 0};

const bt_usb_interface_t *bt_chipset_get_usb_interface(void)
{
    return &usb_interface;
}

// For test, you can set your customor setting here.
static const bt_uart_interface_t uart_interface = {115200, 8, 1, 0, true};
const bt_uart_interface_t *bt_chipset_get_uart_interface(void)
{
    return &uart_interface;
}

// For test, you can set your customor setting here.
static const bt_BlueNRG_SPI_interface_t spi_interface = {1, 0, 1 * 1000 * 1000, 8, 1, 0, 0, 1};
//for Nucleo-L476RG BLUENRG2:  {PA1(1), PA0(0), 1MHz, 8bits, MSB, Master, CPOL = 0, CPHA = 1};

const bt_BlueNRG_SPI_interface_t *bt_BlueNRG_get_SPI_interface(void)
{
    return &spi_interface;
}
