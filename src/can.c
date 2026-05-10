#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include <linux/can.h>
#include<linux/can/raw.h>
#include"log.h"
int s; //水管
struct ifreq ifr;//指定水管要接的口
struct sockaddr_can addr; //接起来
struct can_frame frame;//供水系统要提供什么样的水
struct can_filter filter[1];   // 过滤器结构体
int nbytes;
int can_Init(void)
{
       LOG_DEBUG("CAN:进入can发送函数");      



//先创建socket，也就是告诉内核我要申请水管
if((s = socket(PF_CAN, SOCK_RAW , CAN_RAW)) < 0)  //协议域是can，然后采用原始套接字（不过udp和tcp），可以自由读写完整的can帧数据
{
   perror("socket");    
   LOG_ERROR("CAN:创建socket失败");      
   return -1;
}
 
//指定水管接口
strcpy(ifr.ifr_name, "vcan0"); //
ioctl(s,SIOCGIFINDEX,&ifr); //S是水管的编号。Socket IO Control ：Get InterFace Index
LOG_DEBUG("CAN:申请接口请求");

addr.can_family = AF_CAN;
addr.can_ifindex = ifr.ifr_ifindex;
bind(s, (struct sockaddr *)&addr, sizeof(addr));
LOG_DEBUG("CAN:绑定接口");
LOG_INFO("CAN:初始化完成");

return 0;

}
int my_can_send(u_int16_t id, u_int16_t dlc, short unsigned int* data_M)
{
    frame.can_id = id;
frame.can_dlc = dlc;
for(int i=0;i<dlc;i++)
{
    frame.data[i] = data_M[i];
}

if (write(s, &frame, sizeof(frame)) != sizeof(frame)) {
    perror("write");
    return 1;
    LOG_ERROR("CAN:数据发送失败");
}
 // ========== 关键：设置过滤器 ==========
    // 只接收 can_id == 0x123 的帧
    filter[0].can_id   = 0x123;              // 要匹配的 ID
    filter[0].can_mask = CAN_SFF_MASK;       // 掩码：全匹配（11 位全比较）
    setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER,
               &filter, sizeof(filter));
 LOG_INFO("CAN:过滤器设置完成0x%x\n",0x123);
LOG_INFO("CAN:数据发送成功");
return 0;
}
//can接收函数
int can_receive()
{ 
   

    // 4. 循环接收（现在只会收到 ID 为 0x123 的帧）
    while (1) {
        nbytes = read(s, &frame, sizeof(frame));
        if (nbytes < 0) {
            perror("read");
            return 1;
        }

        LOG_INFO("收到过滤后: ID=0x%03X, DLC=%d, Data=",
               frame.can_id, frame.can_dlc);
        for (int i = 0; i < frame.can_dlc; i++) {
            LOG_INFO("can_receive:%02X ", frame.data[i]);
        }
        printf("\n");
    }

    close(s);
    return 0;
}
   
