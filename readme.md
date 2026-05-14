
# Linux Gateway - Modbus RTU/TCP to CAN to MQTT

## 项目简介

这是一个运行在 Linux 上的工业物联网网关原型。  
采集 Modbus RTU/TCP 设备数据，通过 CAN 总线转发，最终使用 MQTT 协议上报到华为云 IoTDA。  
支持多线程、断线重连、日志分级等工业级特性。

## 架构图

```text
[Modbus RTU设备/TCP 设备python脚本模拟）] -> [libmodbus] -> [CAN 总线] -> [MQTT] -> [华为云]
```

## 功能列表

- Modbus RTU/TCP 主站，支持断线重连与超时重试
- CAN 总线数据收发（SocketCAN，接收自己发送的数据，自发自接），解决内核默认不接收自己发送帧的问题
- MQTT 上云（华为云 IoTDA），支持密钥认证
- 多线程（采集、转发、上报分离），互斥锁保护共享数据
- 分级日志系统（DEBUG / INFO / ERROR）
- Makefile 支持 debug / release 编译

## 如何编译与运行

### 1. 安装依赖

```bash
sudo apt install libmodbus-dev libmosquitto-dev can-utils
```
### 1.1 安装 Modbus TCP 从站依赖
```bash
pip install pymodbus
```

### 2. 创建虚拟 CAN 接口

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

### 3. 编译

```bash
# debug 版本（带调试信息）
make debug

# release 版本（优化）
make release
```
> **注意：从站需要在另一个终端中运行，保持它持续运行，不要关闭。**
### 5. 启动 Modbus TCP 从站
```bash
cd modbus_tcp_slave
chmod +x start.sh   # 首次执行需添加执行权限
./start.sh
```
### 6. 启动 
```bash
sudo ./gateway_debug
```

## 效果展示

//截图或视频链接

## 踩过的坑

- **CAN 自收发收不到**：需设置 `CAN_RAW_RECV_OWN_MSGS` 内核选项
- **mbpoll 无法通信**：手动计算 CRC 验证，发现工具兼容性问题
- **多线程共享数据错乱**：用互斥锁保护，避免竞争条件

## 目录结构

```
├── src/           # 源代码
├── include/       # 头文件
├── modbus_tcp_slave/ #tcp虚拟从站
├── Makefile
├── README.md
└──test            #mqtt连接华为云和can测试

## 下一步计划

- 移植到树莓派（ARM 交叉编译）
- 增加 CANopen 协议支持
- 日志落盘与远程配置
```

