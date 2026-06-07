#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<ctype.h>
#include<arpa/inet.h>
#include<iostream>

#define SERV_PORT 9999
#define BUFFER_SIZE 1024

int main(){

    int sockfd = socket(AF_INET,SOCK_DGRAM,0);


    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET,"127.0.0.1",&serv_addr.sin_addr);


    char buf[BUFFER_SIZE]{};
    
    while (fgets(buf,BUFFER_SIZE,stdin) != NULL)
    {
        int n = sendto(sockfd,buf,sizeof(buf),0,
                            (struct sockaddr*)&serv_addr,sizeof(serv_addr));
        if(n == -1){
            perror("server sendto error");
            break;
        }
        printf("send to service : %.*s",(int)n,buf);



        int ret = recvfrom(sockfd,&buf,sizeof(buf),0,NULL,0);

        if(ret == -1){
            perror("client recevfrom server error");
            break;
        }
        else{
            printf("Received from server : %.*s",(int)ret,buf);   
        }
           
    }
    close(sockfd);
    
    return 0;
}
