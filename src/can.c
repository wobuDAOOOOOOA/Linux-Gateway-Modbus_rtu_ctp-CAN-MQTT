#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include <linux/can.h>
#include<linux/can/raw.h>
#include <errno.h>
// can.c 顶部，和其他 #include 放在一起
#include <fcntl.h>   // 解决 fcntl 未定义
#include <errno.h>   // 解决 errno 未定义
#include"log.h"
#include"config.h"
extern gateway_config_t cfg;  // 告诉编译器：cfg 在别的文件里定义好了

int s; //水管
struct ifreq ifr;//指定水管要接的口
struct sockaddr_can addr; //接起来
struct can_frame frame;//供水系统要提供什么样的水
struct can_filter filter[1];   // 过滤器结构体
int nbytes;
int can_Init(void)
{

    printf("=== cfg.can_interface = [%s] ===\n", cfg.can_interface);


//先创建socket，也就是告诉内核我要申请水管
if((s = socket(PF_CAN, SOCK_RAW , CAN_RAW)) < 0)  //协议域是can，然后采用原始套接字（不过udp和tcp），可以自由读写完整的can帧数据
{
   perror("socket");    
   LOG_ERROR("CAN:创建socket失败");      
   return -1;
}
 // ====================== 新增：非阻塞模式（关键！）======================
    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);
// ======================================================================
//指定水管接口
strcpy(ifr.ifr_name, cfg.can_interface); //
// 加错误检查
if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {//S是水管的编号。Socket IO Control ：Get InterFace Index

    perror("ioctl");
    LOG_ERROR("CAN:获取接口索引失败，接口名=[%s]", cfg.can_interface);
    return -1;
}

LOG_DEBUG("CAN:申请接口请求，接口名=[%s]", cfg.can_interface);

addr.can_family = AF_CAN;
addr.can_ifindex = ifr.ifr_ifindex;
   if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }


     int recv_own_msgs = 1; // 1=开启，0=关闭
    setsockopt(s, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recv_own_msgs, sizeof(recv_own_msgs));
    LOG_INFO("CAN:已开启自收自发模式");


    
   printf("IIIIIIII:%d\n",s);

    printf("%s\n",ifr.ifr_name);
LOG_DEBUG("CAN:绑定接口");
LOG_INFO("CAN:初始化完成");

//  // 设置过滤器
//     // 只接收 can_id == 0x123 的帧
//     filter[0].can_id   = 0x123;              // 要匹配的 ID
//     filter[0].can_mask = CAN_SFF_MASK;       // 掩码：全匹配（11 位全比较）
//     setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER,
//                &filter, sizeof(filter));
//  LOG_INFO("CAN:过滤器设置完成0x%x\n",0x123);
return 0;

}
int my_can_send(u_int16_t id, u_int16_t dlc, short unsigned int* data_M)
{
    static struct can_frame tx_frame;

        LOG_DEBUG("CAN:进入can发送函数");
LOG_INFO("准备发送, ID=%d, data=%d", id, data_M[0]);
    tx_frame.can_id = 0x123;
tx_frame.can_dlc = dlc;
for(int i=0;i<dlc;i++)
{
    tx_frame.data[i] = data_M[i];
}
printf("TTTTTffsdfTTs:%d\n",s);
if (write(s, &tx_frame, sizeof(tx_frame)) != sizeof(tx_frame)) {
    perror("write");
    LOG_ERROR("CAN:数据发送失败");

    return 1;
}

LOG_INFO("CAN:数据发送成功");
return 0;
}

//can接收函数
//can接收函数
int can_receive(unsigned char *buffer , int *len)
{  
    LOG_DEBUG("进入can接收函数");
    nbytes = read(s, &frame, sizeof(frame));
    printf("RRRRRRRs:%d\n",s);

    LOG_DEBUG("nbnnbnb");

    // ====================== 修改：非阻塞错误处理（关键！）======================
    if (nbytes < 0) {
        // EAGAIN = 非阻塞模式下暂无数据，正常现象，不打印错误
        if (errno != EAGAIN) { 
            perror("read");  // 只有真正的异常才打印
        }
        return 1;
    }
    // ==========================================================================

    LOG_INFO("收到过滤后: ID=0x%03X, DLC=%d, Data=", frame.can_id, frame.can_dlc);
    *len = frame.can_dlc;
    for (int i = 0; i < frame.can_dlc; i++) {
        buffer[i] = frame.data[i];
        LOG_INFO("can_receive:%02X ", frame.data[i]);
    }
    return 0;
}
// CAN 接收函数（只接收一帧）

   
// int can_receive(unsigned char *buffer, int *len) {
//     struct can_frame frame;
//     int nbytes = read(can_socket_fd, &frame, sizeof(frame));
//     if (nbytes > 0) {
//         *len = frame.can_dlc;
//         memcpy(buffer, frame.data, frame.can_dlc);
//         return 0;
//     }
//     return -1;
// }

