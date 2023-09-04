
#ifndef APPLICATIONS_HCI_INTERFACE_C_
#define APPLICATIONS_HCI_INTERFACE_C_

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_gpio.h"
#include "rtthread_driver_spi.h"

#include "drivers/hci_driver.h"
#include "drivers/hci_h4.h"

#include "logging/bt_log_impl.h"
#include "common/bt_buf.h"

struct hci_trans_spi_config {
    const char *device_name;    /* spi device name, i.e. "spi1" */
    const char *bus_name;       /* spi BUS name, i.e. "spi10" */
    int cs_pin_num;         /* RT-Thread drv_gpio number of SPI CS PIN */
    int irq_pin_num;        /* RT-Thread drv_gpio number of SPI IRQ PIN */
    uint32_t rate;
    int data_width;         /* Data width: 8bits, 16bits, 32bits */
    int LSB_MSB;            /* Data transmission order: LSB:0 MSB:1 */
    int Master_Slave;       /* Set the master-slave mode of SPI:  Master:0 Slave:1 */
    int CPOL;               /* Set clock polarity(CPOL):  0 1 */
    int CPHA;               /* Set clock phase(CPHA):  0 1 */
};

static struct hci_trans_spi_config hci_config;
struct rt_spi_device *ble_spi = RT_NULL;

int32_t IsDataAvailable(void);

int32_t HCI_TL_SPI_Init(void* pConf)
{
    /******PIN******/
    rt_pin_mode(hci_config.irq_pin_num, PIN_MODE_INPUT); // IRQ input
    rt_pin_mode(hci_config.cs_pin_num, PIN_MODE_OUTPUT); // CS output

    /*******SPI******/
    RT_ASSERT(hci_config.device_name);
    RT_ASSERT(hci_config.bus_name);

    ble_spi = (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    if(RT_NULL == ble_spi)
    {
        printk("Failed to malloc the spi device.\n");
        return -RT_ENOMEM;
    }

    if (RT_EOK != rt_spi_bus_attach_device(ble_spi, hci_config.device_name, hci_config.bus_name, RT_NULL))
    {
        printk("Failed to attach the spi device.\n");
        return -RT_ERROR;
    }

    /*******SPI Device Config*******/
    struct rt_spi_configuration cfg;
    cfg.data_width = hci_config.data_width;
    cfg.mode = (hci_config.Master_Slave << 3) |
            ((hci_config.CPOL << 1) | (hci_config.CPHA << 0)) |
            (hci_config.LSB_MSB << 2);
    cfg.max_hz =  hci_config.rate;                           /* 1M */

    if (RT_EOK != rt_spi_configure(ble_spi, &cfg)) {
        printk("Failed to config the spi device.\n");
        return -RT_ERROR;
    }

    return RT_EOK;
}

/**
 * @brief  Reads from BlueNRG SPI buffer and store data into local buffer.
 *
 * @param  buffer : Buffer where data from SPI are stored
 * @param  size   : Buffer size
 * @retval int32_t: Number of read bytes
 */
int32_t HCI_TL_SPI_Receive(uint8_t* buffer, uint16_t size)
{
  uint16_t byte_count;
  uint8_t len = 0;
  uint8_t char_00 = 0x00;
  volatile uint8_t read_char;

  uint8_t header_master[HEADER_SIZE] = {0x0b, 0x00, 0x00, 0x00, 0x00};
  uint8_t header_slave[HEADER_SIZE];

  /* CS reset */
  rt_pin_write(HCI_TL_SPI_CS_PIN, PIN_LOW);

  /* Read the header */
  rt_spi_transfer(ble_spi, &header_master, &header_slave, HEADER_SIZE); //RTAPI

  /* device is ready */
  byte_count = (header_slave[4] << 8)| header_slave[3];

  if(byte_count > 0)
  {
    /* avoid to read more data than the size of the buffer */
    if (byte_count > size)
    {
      byte_count = size;
    }

    for(len = 0; len < byte_count; len++)
    {
      rt_spi_transfer(ble_spi, &char_00, (uint8_t*)&read_char, 1);
      buffer[len] = read_char;
    }
  }

  /**
   * To be aligned to the SPI protocol.
   * Can bring to a delay inside the frame, due to the BlueNRG-2 that needs
   * to check if the header is received or not.
   */
  uint32_t tickstart = rt_tick_get();
  while ((rt_tick_get() - tickstart) < TIMEOUT_IRQ_HIGH) {
    if (rt_pin_read(HCI_TL_SPI_IRQ_PIN) == PIN_LOW) {
      break;
    }
  }

  /* Release CS line */
  rt_pin_write(HCI_TL_SPI_CS_PIN, PIN_HIGH);

  return len;
}

/**
 * @brief  Writes data from local buffer to SPI.
 *
 * @param  buffer : data buffer to be written
 * @param  size   : size of first data buffer to be written
 * @retval int32_t: Number of read bytes
 */
int32_t HCI_TL_SPI_Send(uint8_t* buffer, uint16_t size)
{
  int32_t result;
  uint16_t rx_bytes;

  uint8_t header_master[HEADER_SIZE] = {0x0a, 0x00, 0x00, 0x00, 0x00};
  uint8_t header_slave[HEADER_SIZE];

  static uint8_t read_char_buf[MAX_BUFFER_SIZE];
  uint32_t tickstart = rt_tick_get();


  do
  {
    uint32_t tickstart_data_available = rt_tick_get();

    result = 0;

    /* CS reset */
    rt_pin_write(HCI_TL_SPI_CS_PIN, PIN_LOW);

    /*
     * Wait until BlueNRG-2 is ready.
     * When ready it will raise the IRQ pin.
     */
    while(!IsDataAvailable())
    {
      if((rt_tick_get() - tickstart_data_available) > TIMEOUT_DURATION)
      {
        printk("%d timeout\r\n", rt_tick_get() - tickstart_data_available);
        result = -3;
        break;
      }
    }
    if(result == -3)
    {
      /* The break causes the exiting from the "while", so the CS line must be released */
      rt_pin_write(HCI_TL_SPI_CS_PIN, PIN_HIGH);
      break;
    }

    /* Read header */
    rt_spi_transfer(ble_spi, &header_master, &header_slave, HEADER_SIZE);

    printk("Send header size: %d buffer: %02x", HEADER_SIZE, header_slave[0]);
    for (int i = 1; i < HEADER_SIZE; ++i) {
        printk("%02x", header_slave[i]);
    }
    printk("\r\n");

    rx_bytes = (((uint16_t)header_slave[2])<<8) | ((uint16_t)header_slave[1]);

    if(rx_bytes >= size)
    {
      /* Buffer is big enough */
      rt_spi_transfer(ble_spi, buffer, &read_char_buf, size);
    }
    else
    {
      /* Buffer is too small */
      result = -2;
    }

    /* Release CS line */
    rt_pin_write(HCI_TL_SPI_CS_PIN, PIN_HIGH);

    if((rt_tick_get() - tickstart) > TIMEOUT_DURATION)
    {
      result = -3;
      break;
    }
  } while(result < 0);

  /**
   * To be aligned to the SPI protocol.
   * Can bring to a delay inside the frame, due to the BlueNRG-2 that needs
   * to check if the header is received or not.
   */
  tickstart = rt_tick_get();
  while ((rt_tick_get() - tickstart) < TIMEOUT_IRQ_HIGH) {
    if (rt_pin_read(HCI_TL_SPI_IRQ_PIN) == PIN_LOW) {
      break;
    }
  }

  return result;
}


/**
 * @brief  Reports if the BlueNRG has data for the host micro.
 *
 * @param  None
 * @retval int32_t: 1 if data are present, 0 otherwise
 */
int32_t IsDataAvailable(void)
{
  return (rt_pin_read(HCI_TL_SPI_IRQ_PIN) == PIN_HIGH);
}

static int hci_driver_open(void)
{
    HCI_TL_SPI_Init(NULL); // RT SPI init

    printk("hci_driver_open, SPI_config_finish\n");

    return 0;
}

int switch_net_buf_type(uint8_t type)
{
    switch (type)
    {
    case BT_BUF_ACL_OUT:
        return HCI_ACLDATA_PKT;
    case BT_BUF_CMD:
        return HCI_COMMAND_PKT;
    default:
        printk("Unknown buffer type");
    }
    return 0;
}
static int hci_driver_send(struct net_buf *buf)
{
    uint8_t type = bt_buf_get_type(buf);

//    net_buf_push_u8(buf, type);
//    uint8_t* data = buf->data;

    uint8_t len = buf->len;

    if(len >= MAX_BUFFER_SIZE)
    {
        return -1;
    }

    uint8_t data[MAX_BUFFER_SIZE];
    data[0] = switch_net_buf_type(type);
    memcpy(data + 1, buf->data, len); //data[0] is the type  copy to data[1]

    printk("hci_driver_send, type: %d, len: %d, data: %02x:%02x:%02x:%02x:%02x:%02x\n", type, len, data[0], data[1], data[2], data[3], data[4], data[5]);

    if (HCI_TL_SPI_Send(data, len + 1) < 0) { // type +1
        return -1;
    }
    net_buf_unref(buf);
    return 0;
}

static int hci_driver_recv(uint8_t *buf, uint16_t len)
{
    return HCI_TL_SPI_Receive(buf, len);
}

void hci_driver_init_loop(void)
{
    uint8_t data[MAX_BUFFER_SIZE];
    uint8_t len = MAX_BUFFER_SIZE;
    int ret = HCI_TL_SPI_Receive(data, len); //ret: bytes num Recv
    if(ret > 0 && (data[0] != 0)) // type cant be 0
    {
        printk("hci_driver_init_loop, ret: %d, data: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", ret, data[0], data[1], data[2], data[3], data[4], data[5],data[6],data[7]);

        struct net_buf *buf;
        switch(data[0])
        {
            case HCI_EVENT_PKT:
                buf = bt_buf_get_controller_tx_evt();
                break;
            case HCI_ACLDATA_PKT:
                buf = bt_buf_get_controller_tx_acl();
                break;
            default:
                return;
        }

        if (buf)
        {
            net_buf_add_mem(buf, data + 1, ret - 1);
            bt_recv(buf);
        }
    }
}

static const struct bt_hci_driver drv = {
        .open = hci_driver_open,
        .send = hci_driver_send,
};

static char device_name[NAME_SIZE];
static char bus_name[NAME_SIZE];

int hci_driver_init(bt_BlueNRG_SPI_interface_t *hci_cfg, int device_idx, int spi_index)
{

    rt_sprintf(bus_name, "spi%d", spi_index);
    hci_config.bus_name = bus_name;
    rt_sprintf(device_name, "spi%d%d", spi_index, device_idx);
    hci_config.device_name = device_name;

    hci_config.rate = hci_cfg->rate;
    hci_config.data_width = hci_cfg->data_width;
    hci_config.LSB_MSB = hci_cfg->LSB_MSB;
    hci_config.Master_Slave = hci_cfg->Master_Slave;
    hci_config.CPOL = hci_cfg->CPOL;
    hci_config.CPHA = hci_cfg->CPHA;

    hci_config.cs_pin_num = hci_cfg->cs_pin_num;
    hci_config.irq_pin_num = hci_cfg->irq_pin_num;

    printk("SPI_init_process device_name: %s, spi_name: %s, rate: %d, databits: %d, LSB_MSB: %d, Master_Slave: %d, CPOL: %d, CPHA: %d\n",
            hci_config.device_name, hci_config.bus_name, hci_config.rate, hci_config.data_width,
            hci_config.LSB_MSB, hci_config.Master_Slave, hci_config.CPOL, hci_config.CPHA);

    printk("SPI_init_process cs_pin_num: %d, irq_pin_num: %d\n",
            hci_config.cs_pin_num, hci_config.irq_pin_num);

    if (bt_hci_driver_register(&drv) != 0)
    {
        return -1;
    }
    return 0;
}


#endif /* APPLICATIONS_HCI_INTERFACE_C_ */
