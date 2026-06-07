#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>



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
    int maxfd = lfd;

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(lfd, &read_fds);

    fd_set allcfds;
    FD_ZERO(&allcfds);
    FD_SET(lfd, &allcfds);

        while (1) {
        read_fds = allcfds;

        int ret = select(maxfd + 1, &read_fds, nullptr, nullptr, nullptr);

        if (ret < 0) {
            perror("SERVER : select");
            break;
        }

        if (ret == 0) {
            continue;
        }

        if (FD_ISSET(lfd, &read_fds)) {
            struct sockaddr_in client_addr{};
            socklen_t client_addr_len = sizeof(client_addr);

            int cfd = accept(lfd, (struct sockaddr*)&client_addr, &client_addr_len);
            if (cfd < 0) {
                perror("SERVER : accept");
                continue;
            }

            FD_SET(cfd, &allcfds);

            if (cfd > maxfd) {
                maxfd = cfd;
            }

            printf("New client connected, fd = %d\n", cfd);

            ret--;

            if (ret == 0) {
                continue;
            }
        }

        for (int i = lfd + 1; i <= maxfd; ++i) {
            if (FD_ISSET(i, &read_fds)) {
                char buffer[BUFFER_SIZE];

                ssize_t bytes_read = read(i, buffer, sizeof(buffer));

                if (bytes_read > 0) {
                    printf("Received from client: %.*s", (int)bytes_read, buffer);

                    for (ssize_t j = 0; j < bytes_read; ++j) {
                        buffer[j] = toupper((unsigned char)buffer[j]);
                    }

                    write(STDOUT_FILENO, buffer, bytes_read);
                    write(i, buffer, bytes_read);
                }
                else if (bytes_read == 0) {
                    printf("Client disconnected, fd = %d\n", i);
                    close(i);
                    FD_CLR(i, &allcfds);
                }
                else {
                    perror("SERVER : read");
                    close(i);
                    FD_CLR(i, &allcfds);
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