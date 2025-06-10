#include "checkNetwork.h"
#include "include/basic.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/time.h>

#define BUFFER_SIZE 1024

/*********************
*Function:checkVideoConnect
*Descript:检测摄像头网络连接状态
*Input:NULL
*Output:NULL
**********************/
bool checkVideoConnect(unsigned char *ip, int port,int ms)
{
    if(NULL == ip) return false;
    if(port < 0) return false;

    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd < 0) return false;

    char *ip_addr = (char *)ip;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip_addr);

    struct timeval timeout;
    timeout.tv_sec = 0;//连接超时时间1s
    timeout.tv_usec = ms * 1000;

    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeval));

    if(connect(sockfd,(struct sockaddr*)&addr,sizeof(struct sockaddr_in)) < 0)
    {
        close(sockfd);
        return false;
    }
    close(sockfd);
    return true;
}
/*********************
*Function:
*Descript:
*Input:NULL
*Output:NULL
**********************/
int caleVideoDelay(char *ip, int port)
{
    if(NULL == ip) return fSPC_FAILURE;
    if(port < 0) return fSPC_FAILURE;


    int nFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (nFd < 0) {
        perror("socket creation failed");
        return -1;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(ip);

    char message[] = "test network";
    struct timeval start, end;
    gettimeofday(&start, NULL);
    sendto(nFd, message, strlen(message), 0, (struct sockaddr*)&server_address, sizeof(server_address));

    socklen_t addr_len = sizeof(server_address);
    char buffer[BUFFER_SIZE];
    recvfrom(nFd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&server_address, &addr_len);
    gettimeofday(&end, NULL);

    close(nFd);

    return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;  // 计算并返回延时时间，单位为毫秒
}
