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
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));

    char buf[BUFFER_SIZE]{};
    struct sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);
    while (1)
    {
        int ret = recvfrom(sockfd,&buf,sizeof(buf),0,
                            (struct sockaddr*)&client_addr,&client_addr_len);
        if(ret == -1){
            perror("server recevfeom error");
            break;
        }
        else{
            printf("Received from client : %.*s",(int)ret,buf);

            for(int i = 0;i <ret; i++){
                buf[i] = toupper(buf[i]);
            }
            
            int n = sendto(sockfd,buf,sizeof(buf),0,
                            (struct sockaddr*)&client_addr,client_addr_len);

            if(n == -1){
                perror("server sendto error");
                break;
            }

            printf("sento  client : %.*s",(int)ret,buf);
        }
           
    }
    close(sockfd);
    
    return 0;
}