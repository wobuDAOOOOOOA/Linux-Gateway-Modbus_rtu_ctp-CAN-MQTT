#include <stdio.h>
#include <modbus/modbus.h>
#include <errno.h>
  uint16_t tab_reg[64];
  int rc;
modbus_t* modbus_tcp_bconnect() {
    
    modbus_t *ctx;


    // 1. 创建 Modbus TCP 上下文
    // 使用一个公共的 Modbus TCP 测试服务器，地址和端口
    ctx = modbus_new_tcp("127.0.0.1", 5020);
    if (ctx == NULL) {
        fprintf(stderr, "无法创建mak Modbus TCP 上下文\n");
        return -1;
    }

    // 2. 设置从站地址 (Slave ID)
    // 在这个测试服务器上，从站地址通常是 1 或 17
    modbus_set_slave(ctx, 1);

    // 3. 建立连接
    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "连接失败: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }
        printf("成功连接到 Modbus TCP 服务器!\n");

   return ctx;
}
void modbus_read(modbus_t *ctx)
{
    // 4. 读取保持寄存器
    // 从地址 0 开始，读取 10 个保持寄存器
    rc = modbus_read_registers(ctx, 0, 10, tab_reg);
    if (rc == -1) {
    fprintf(stderr, "[ERROR] 读取失败: %s\n", modbus_strerror(errno));
    // 这里可以尝试重连逻辑（可选）
} else if (rc != 10) { // 假设你要读10个寄存器
    fprintf(stderr, "[WARN] 期望读10个，实际读了%d个\n", rc);
} else {
    printf("[OK] 成功读取10个寄存器\n");
    for (int i = 0; i < rc; i++) {
        printf("reg[%d] = %d\n", i, tab_reg[i]);
    }
}


}