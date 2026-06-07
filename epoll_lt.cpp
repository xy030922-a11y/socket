#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/epoll.h>

#define BUFFER_SIZE 1024
#define SERVER_PORT 11111

int main(){

    int lfd = socket(AF_INET,SOCK_STREAM,0);
    if(lfd < 0){
        perror("SERVER : socket");
        return -1;
    }

    int opt =1;
    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("SERVER : setsockopt");
        close(lfd);
        return -1;
    }

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(lfd,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0){
        perror("SERVER : bind");
        close(lfd);
        return -1;
    }

    if(listen(lfd,128) < 0){
        perror("SERVER : listen");
        close(lfd);
        return -1;
    }

    int epfd = epoll_create1(0);
    if(epfd < 0){
        perror("SERVER : epoll_create");
        return -1;
    }

    struct epoll_event lfd_event{};
    lfd_event.events = EPOLLIN;
    lfd_event.data.fd = lfd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &lfd_event) < 0) {
        perror("SERVER : epoll_ctl add lfd");
        close(lfd);
        close(epfd);
        return -1;
    }

    struct epoll_event ep_events[1024];
    while(1){
        int ret = epoll_wait(epfd,ep_events,1024,-1);

        if(ret < 0){
            perror("SERVER : epoll_wait");
            continue;
        }
        if(ret == 0){
            continue;
        }

        for(int i = 0; i < ret;i++){
            int fd = ep_events[i].data.fd;

            if(fd == lfd){
                struct sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);

                int cfd = accept(lfd,(struct sockaddr*)&client_addr,&client_len);
                if(cfd < 0){
                    perror("SERVER : accept");
                    continue;
                }

                struct epoll_event temp_event{};
                temp_event.events = EPOLLIN;
                temp_event.data.fd = cfd;

                if((epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&temp_event)) < 0){
                    perror("SERVER : epol_ctl");
                    close(cfd);
                    continue;
                }
            }
            else{
                char buffer[BUFFER_SIZE];

                ssize_t byte_read = read(fd,buffer,sizeof(buffer));

                if(byte_read == 0){
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    continue;
                }
                else if(byte_read < 0){
                    perror("SERVER : read");
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    continue;
                }
                else {
                    printf("Received from client : %.*s",(int)byte_read,buffer);

                    for(int j = 0;j<byte_read;j++){
                        buffer[j] = toupper((unsigned char)buffer[j]);
                    }

                    write(fd,buffer,byte_read);
                    write(STDOUT_FILENO,buffer,byte_read);

                }
            }
        }
    }


    return 0;
}