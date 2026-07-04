from pymodbus.server.sync import StartTcpServer
from pymodbus.datastore import ModbusSequentialDataBlock, ModbusSlaveContext, ModbusServerContext

# 初始化 100 个保持寄存器，值全部设为 123
store = ModbusSlaveContext(
    hr=ModbusSequentialDataBlock(0, [123] * 100)
)
context = ModbusServerContext(slaves=store, single=True)

print("✅ Modbus TCP 从站启动成功 端口：5020 (寄存器值=123)")
StartTcpServer(context, address=('0.0.0.0', 5020))
