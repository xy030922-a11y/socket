#include<iostream>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<ctype.h>
#include <arpa/inet.h>




constexpr int  SERV_PORT  = 9527;
constexpr int BUFFER_SIZE = 4096;

int main(int argc,char *argv[]){

    int lfd = socket(AF_INET,SOCK_STREAM,0);
    
    if(lfd == -1){
        perror("socket error");
    }

    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind(lfd,(const struct sockaddr *)&serv_addr,sizeof(serv_addr));
    if (bind(lfd,(const struct sockaddr *)&serv_addr,sizeof(serv_addr)) == -1) {
        perror("bind error");
        return -1;
    }



    // listen(lfd,128);
    if (listen(lfd, 128) == -1) {
        perror("listen error");
        return -1;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    client_addr_len = sizeof(client_addr);
    
    int cfd = 0;
    cfd = accept(lfd,(struct sockaddr *)&client_addr,&client_addr_len);
    if(cfd == -1){
        perror("accept error");
    }
    std::cout<< "网络地址" << std::endl;
    std::cout<< "ip  :" << client_addr.sin_addr.s_addr <<std::endl;
    std::cout<< "port:" << client_addr.sin_port <<std::endl;

    char client_IP[1024];
    std::cout<< "转换后 网络地址" << std::endl;
    std::cout<< "ip  :" << inet_ntop(AF_INET,&client_addr.sin_addr.s_addr,client_IP,sizeof(client_IP))  
        <<std::endl;
    std::cout<< "port:" << ntohs(client_addr.sin_port) <<std::endl;
    
    



    char buf[BUFFER_SIZE];
    while(1){
        int ret = read(cfd,buf,sizeof(buf));
        for(int i = 0;i <ret; i++){
            buf[i] = toupper(buf[i]);
        }
        write(cfd,buf,ret);
        
    }
    
    close(cfd);
    close(lfd);

    return 0;
}