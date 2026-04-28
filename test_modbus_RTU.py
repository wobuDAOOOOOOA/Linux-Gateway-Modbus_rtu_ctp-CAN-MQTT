import serial
import time

ser = serial.Serial('/dev/ttyACM0', 4800, timeout=2)

# 就发这8个字节，完全是你手动的那一串
cmd = bytes([0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B])
print(f"Sent: {cmd.hex().upper()}")
ser.write(cmd)

# 等一会儿，让设备回复
time.sleep(0.2)
resp = ser.read(50)
print(f"Received: {resp.hex().upper()}")

ser.close()
