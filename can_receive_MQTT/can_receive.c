#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>

int main() {
    int s;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;
    struct can_filter filter[1];   // 过滤器结构体
    int nbytes;

    // 1. 创建 socket（同上）
    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) {
        perror("socket");
        return 1;
    }

    // 2. 指定接口（同上）
    strcpy(ifr.ifr_name, "vcan0");
    ioctl(s, SIOCGIFINDEX, &ifr);

    // 3. 绑定（同上）
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(s, (struct sockaddr *)&addr, sizeof(addr));

    // ========== 关键：设置过滤器 ==========
    // 只接收 can_id == 0x123 的帧
    filter[0].can_id   = 0x123;              // 要匹配的 ID
    filter[0].can_mask = CAN_SFF_MASK;       // 掩码：全匹配（11 位全比较）
    setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER,
               &filter, sizeof(filter));

    // 4. 循环接收（现在只会收到 ID 为 0x123 的帧）
    while (1) {
        nbytes = read(s, &frame, sizeof(frame));
        if (nbytes < 0) {
            perror("read");
            return 1;
        }

        printf("收到过滤后: ID=0x%03X, DLC=%d, Data=",
               frame.can_id, frame.can_dlc);
        for (int i = 0; i < frame.can_dlc; i++) {
            printf("%02X ", frame.data[i]);
        }
        printf("\n");
    }

    close(s);
    return 0;
}