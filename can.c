#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include <linux/can.h>
#include <linux/can/raw.h>

int my_can_Init()
{
int s; //水管
struct ifreq ifr;//指定水管要接的口
struct sockaddr_can addr; //接起来
struct can_frame frame;//供水系统要提供什么样的水

//先创建socket，也就是告诉内核我要申请水管
if((s = socket(PF_CAN, SOCK_RAW , CAN_RAW)) < 0)  //协议域是can，然后采用原始套接字（不过udp和tcp），可以自由读写完整的can帧数据
{
   perror("socket");          
   return -1;
}

//指定水管接口
strcpy(ifr.ifr_name, "vcan0"); //
ioctl(s,SIOCGIFINDEX,&ifr); //S是水管的编号。Socket IO Control ：Get InterFace Index

addr.can_family = AF_CAN;
addr.can_ifindex = ifr.ifr_ifindex;
bind(s, (struct sockaddr *)&addr, sizeof(addr));

frame.can_id = 0x123;
frame.can_dlc = 2;
frame.data[0] = 0xAA;
frame.data[1] = 0xBB;

if (write(s, &frame, sizeof(frame)) != sizeof(frame)) {
    perror("write");
    return 1;
}

return 0;
}
