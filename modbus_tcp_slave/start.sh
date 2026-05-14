#!/bin/bash
cd "$(dirname "$0")"
source venv/bin/activate
python slave.py
source /home/wdz/gateway/modbus_tcp_slave/venv/bin/activate   # 如果虚拟环境在 gateway 下
python slave.py
