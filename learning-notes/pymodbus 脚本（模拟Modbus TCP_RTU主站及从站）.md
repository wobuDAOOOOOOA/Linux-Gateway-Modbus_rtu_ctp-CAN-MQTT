# pymodbus 脚本（模拟Modbus TCP/RTU主站及从站）

python

## MODBUS_TCP主站代码

```python
from pymodbus.client import ModbusTcpClient

# 1. 创建客户端（就像你创建 ctx）
client = ModbusTcpClient('192.168.1.100', port=502)

# 2. 连接
client.connect()

# 3. 读保持寄存器（地址0，读3个）
result = client.read_holding_registers(address=0, count=3, slave=1)
if not result.isError():
    print(result.registers)  # [温度, 湿度, 气压] 列表

# 4. 写单个寄存器（地址10，值100）
client.write_register(address=10, value=100, slave=1)

# 5. 关闭
client.close()
```

pymodbus 自带重连和异常处理，比libmodbus省心。slave 参数就是设备地址（从站ID）和libmodbus库配置modbus tcp主站几乎一样

也是创建上下文，然后连接就可以读写寄存器了

## MODBUS_RTU主站代码

```python
from pymodbus.client import ModbusSerialClient

client = ModbusSerialClient(
    port='/dev/ttyUSB0',    # 对应你的 USB转485
    baudrate=9600,
    parity='N',
    stopbits=1,
    bytesize=8,
    method='rtu'
)
client.connect()
result = client.read_holding_registers(0, 2, slave=2)
# ... 同样逻辑
```

## MODBUS_TCP从站模拟

```python
from pymodbus.server import StartSerialServer
from pymodbus.datastore import ModbusSlaveContext, ModbusSequentialDataBlock

# 初始化一块数据区（相当于你网关里存传感器数据的结构体）
store = ModbusSlaveContext(
    di=ModbusSequentialDataBlock(0, [0]*100),   # 离散输入
    co=ModbusSequentialDataBlock(0, [0]*100),   # 线圈
    hr=ModbusSequentialDataBlock(0, [23, 55, 101]), # 保持寄存器[地址0,1,2] 温度/湿度/气压初始值
    ir=ModbusSequentialDataBlock(0, [0]*100)    # 输入寄存器
)
context = store  # 在最新版 pymodbus 中简化

# 启动 RTU 从站（串口）
StartSerialServer(context, port='/dev/ttyUSB0', baudrate=9600)
```

1. 配置从站，离散输入寄存器（只能读，是由从站决定的）

2. 离散输入寄存器他的寄存器值变化情况是由从站本身规定好的。

3. 如果是真实的从站硬件，离散寄存器的改变所需的条件需要去手册找。

4. 主站读取这个寄存器就知道是否有警报之类的信号

5. 叫做离散（Discrete）是因为离散只有两个状态

```python
from pymodbus.server import StartTcpServer
from pymodbus.datastore import ModbusSlaveContext, ModbusSequentialDataBlock

# 数据区定义（和RTU版本一模一样）
store = ModbusSlaveContext(
    di=ModbusSequentialDataBlock(0, [0]*100),    # 离散输入
    co=ModbusSequentialDataBlock(0, [0]*100),    # 线圈
    hr=ModbusSequentialDataBlock(0, [23, 55, 101]), # 保持寄存器：温度/湿度/气压
    ir=ModbusSequentialDataBlock(0, [0]*100)     # 输入寄存器
)

# 启动 TCP 从站，监听 502 端口
StartTcpServer(
    context=store,
    address=('0.0.0.0', 502)  # 0.0.0.0 表示监听所有网卡
)
```

从站这里监听502端口，这个端口是不是可以理解为一个接口

IP地址 = 小区地址（比如北海市某小区3栋）

端口 = 这栋楼里具体的房间号（502室、1883室、22室）

每个IP可以有65535个端口（0到65535，0到1023是系统预留的）

这个"创建端口"的行为，准确说是"绑定"
你什么时候需要异步模式（现在不需要）
场景	同步够不够
读一个传感器，打印，退出	够
循环读10次，间隔1秒	够
同时读50个Modbus设备，每个100个寄存器	不够，同步得一个个等，50个轮完要很久
边读传感器，边响应HTTP请求，边写数据库	不够，同步只能一次做一件事
异步模式就是解决高并发、多连接场景的。你的测试脚本一次只测一个设备，同步完全够用。