#include<iostream>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<ctype.h>
#include<arpa/inet.h>

#define SERV_PORT 9999
#define BUFFER_SIZE 1024

constexpr auto SERV_IP = "127.0.0.1";

int main(){

    int cfd = socket(AF_INET,SOCK_STREAM,0);
    if(cfd == -1){
        perror("connect error:");
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);

    inet_pton(AF_INET,SERV_IP,&serv_addr.sin_addr.s_addr);

    
    int ret = connect(cfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    if(ret == -1){
        perror("connect error :");
    }
    char buf[BUFFER_SIZE];
    int cnt = 10;
    const char *msg = "hello";
    // sleep(5);
    while(cnt--){
        ssize_t wlen = write(cfd,msg,5);
        ssize_t rlen = read(cfd,buf,sizeof(buf));
        write(STDOUT_FILENO, buf, rlen);
        write(STDOUT_FILENO, "\n", 1);
    }
    close(cfd);
    return 0;
}