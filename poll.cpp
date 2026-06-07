#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <poll.h>

#define BUFFER_SIZE 1024
#define SERVER_PORT 11111


int main(){

    int lfd = socket(AF_INET,SOCK_STREAM,0);
    if(lfd < 0){
        perror("SERVER : socket");
        return -1;
    }

    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(lfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("SERVER : bind");
        return -1;
    }

    listen(lfd,128);

    struct pollfd pfds[1024];
    for (int i = 0; i < 1024; i++) {
        pfds[i].fd = -1;
        pfds[i].events = 0;
        pfds[i].revents = 0;
    }
    pfds[0].fd = lfd;
    pfds[0].events = POLLIN;

    int curMaxIndex = 0;

    while(1){

        int ret = poll(pfds,curMaxIndex + 1,-1);

        if(ret < 0){
            perror("poll error");
            continue;
        }
        if(ret == 0){
            continue;
        }
        if(pfds[0].revents & POLLIN){
            struct sockaddr_in client_addr{};
            socklen_t client_addr_len = sizeof(client_addr);

            int cfd = accept(lfd,(struct sockaddr*)&client_addr,&client_addr_len);

            if(cfd < 0){
                perror("SERVER : accept error");
                continue;
            }

            int index = -1;
            for(int i = 1; i < 1024;i++){
                if(pfds[i].fd == -1){
                    index = i;
                    break;
                }
            }
            
            while(curMaxIndex > 0 && pfds[curMaxIndex].fd == -1){
                curMaxIndex--;
            }

            if(index == -1){
                printf("too many clients\n");
                close(cfd);
            }
            else{
                pfds[index].fd = cfd;
                pfds[index].events = POLLIN;
                pfds[index].revents = 0;
                
                if(index > curMaxIndex){
                    curMaxIndex = index;
                }
            }

            if(--ret == 0){
                continue;
            }
        }


    
        for(int i = 1;i <= curMaxIndex;i++){
            if(pfds[i].revents & POLLIN){
                char buffer[BUFFER_SIZE];

                ssize_t bytes_read = read(pfds[i].fd, buffer, sizeof(buffer));

                if (bytes_read > 0) {
                    printf("Received from client: %.*s", (int)bytes_read, buffer);

                    for (ssize_t j = 0; j < bytes_read; ++j) {
                        buffer[j] = toupper((unsigned char)buffer[j]);
                    }

                    write(STDOUT_FILENO, buffer, bytes_read);
                    write(pfds[i].fd, buffer, bytes_read);
                }
                else if (bytes_read == 0) {
                    printf("Client disconnected, fd = %d\n", i);
                    close(pfds[i].fd);
                    pfds[i].fd = -1;
                    pfds[i].events = 0;
                    pfds[i].revents = 0;

                }
                else {
                    perror("SERVER : read");
                    close(pfds[i].fd);
                    pfds[i].fd = -1;
                    pfds[i].events = 0;
                    pfds[i].revents = 0;
                }

                ret--;

                if (ret == 0) {
                    break;
                }
            }
        }

    }

    return 0;
}