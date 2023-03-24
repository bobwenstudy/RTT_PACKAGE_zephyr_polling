# zephyr_pollling

## 1、介绍

本项目是基于[Zephyr Project](https://www.zephyrproject.org/)进行二次开发的，去除了OS调度部分，只保留了Bluetooth的Host协议栈。项目地址：[bobwenstudy/zephyr_polling (github.com)](https://github.com/bobwenstudy/zephyr_polling)。文档地址：[Welcome to Zephyr_polling’s documentation!](https://zephyr-polling.readthedocs.io/en/latest/)。该协议栈提供完整的BLE  Host 层支持，借助灵活清晰的porting+chipset结构，可实现 RT-Thread 下支持 Host 层搭配串口连接外部 Controller 芯片使用。

### 1.1 目录结构

zephyr_polling
 ├── chipset
 │   ├── artpi_ap6212
 │   └── common
 ├── example
 │   ├── beacon
 │   ├── broadcaster
 │   ├── central
 │   ├── observer
 │   ├── peripheral
 │   ...
 ├── platform
 │   └── rtthread
 │   ...
 ├── porting
 │   ├── rtthread_uart
 │   └── rtthread_artpi
 │   ...
 └── src
     ...

| 名称 | 说明 |
| ---- | ---- |
| chipset | 各家厂商在使用之前需要进行一些配置，有些是因为芯片是rom化版本，需要加载patch，有些要配置RF参数，有些要配置蓝牙地址等。 |
| example | 各种蓝牙例程，基本是照搬zephyr的来，当然会加入一些新的case。 |
| platform | 移植时重点关注的部分，蓝牙协议栈运行需要用到一些平台资源，不同平台有不同实现方式，主要包括log、timer、storage_kv和HCI接口的实现。 |
| porting | 程序的主入口，这些会将platform/chipset和协议栈接口进行绑定，并启动example，最后对协议栈进行调度。 |
| src | zephyr的蓝牙协议栈部分，具体实现蓝牙协议栈的具体细节。 |

### 1.2 许可证

zephyr_polling软件包遵循 Apache-2.0 许可，详见 LICENSE 文件。。

### 1.3 依赖

- RT-Thread 3.0+

## 2、获取zephyr_polling软件包

使用 zephyr_polling package 需要在 RT-Thread 的包管理器中选择它，具体路径如下：

```
RT-Thread online packages
     IoT - internet of things  --->
         [*] zephyr_polling: Bluetooth BLE Stack.  --->
         	  Version (latest)  --->
              Select ChipSet (pts_dongle)  --->
              Select Example (beacon)  --->
              Select Platform (rtthread_uart: generate uart interface)
              HCI Transport Set  --->
```



**Version** ： 软件包版本选择；

**Select ChipSet (pts_dongle)**： 配置chipset；

**Select Example (beacon)** ： 配置开启的example；

**Select Platform (rtthread_uart: generate uart interface)** ： 配置平台信息；



然后让 RT-Thread 的包管理器自动更新，或者使用 `pkgs --update` 命令更新包到 BSP 中。



## 3、使用zephyr_polling软件包

本项目时通用Host平台，理论上通过部署不同的platform+chipset可以支持所有外挂Controller 。

关于协议栈的详细说明可以参考：[Welcome to Zephyr_polling’s documentation!](https://zephyr-polling.readthedocs.io/en/latest/)

最新版本特性支持 RT-Thread 搭配 UART 外接蓝牙Controller卡片使用，参考以下文档：

- [QEMU + 蓝牙Controller卡片使用 NimBLE](https://club.rt-thread.org/ask/article/47e1aad061e7a53c.html)
- [如何在 ART-Pi 的 bsp 工程中使用 NimBLE 蓝牙协议栈](https://club.rt-thread.org/ask/article/2a90783d5ac51641.html)
- [如何在 ART-Pi 的 Studio 工程中使用 NimBLE 蓝牙协议栈](https://club.rt-thread.org/ask/article/ed1e170fb2a30f0a.html)

外部蓝牙 Controller 选择及固件可参考 [蓝牙控制器固件](https://github.com/RT-Thread-packages/nimble/tree/master/docs/firmwares)



## 4、注意事项

要使用蓝牙功能，必须外界蓝牙Controller。

## 5、联系方式 & 感谢

* 维护：[bobwenstudy](https://github.com/bobwenstudy)
* 主页：[bobwenstudy/RTT_PACKAGE_zephyr_polling](https://github.com/bobwenstudy/RTT_PACKAGE_zephyr_polling)