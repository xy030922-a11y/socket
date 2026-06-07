#include <unistd.h>
#include <sys/socket.h>
#include <error.h>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <sys/wait.h>
#include <thread>
#include <pthread.h>

#define SERV_PORT 9999
#define BUFFER_SIZE 1024
#define USE_STD_THREAD 1

int main()
{
    int lfd = socket(AF_INET,SOCK_STREAM,0);
    if(lfd == -1){
        perror(" SERVER: create socket error");
        return -1;
    }

    int opt = 1;
    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt error");
        close(lfd);
        return -1;
    }

    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(lfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1){
        perror(" SERVER: bind error");
        return -1;
    }

    if(listen(lfd,128) == -1){
            perror(" SERVER: listen error");
            return -1;
    }


    while(1)
    {
        struct sockaddr_in client_addr{};
        client_addr.sin_family = AF_INET;
        socklen_t client_addr_len = sizeof(client_addr);

        int cfd = accept(lfd, (struct sockaddr*)&client_addr, &client_addr_len);
        if(cfd == -1) {
            perror("SERVER: accept error");
            continue;
        }
        else{
            if(USE_STD_THREAD){
                //C++11::thread
                std::thread t([cfd, client_addr](){
                    char buffer[BUFFER_SIZE];
                    while(1){
                        ssize_t bytesRead = read(cfd, buffer, BUFFER_SIZE);
                        if(bytesRead <0){
                            perror("recv error");
                            close(cfd);
                            break;
                        }
                        else if (bytesRead > 0) {
                            std::cout << "Received from client: " << buffer << std::endl;
                            for(size_t i = 0; i < bytesRead; ++i){
                                buffer[i] = std::toupper(static_cast<unsigned char>(buffer[i]));
                            }
                            write(cfd, buffer, bytesRead);
                        } else if (bytesRead == 0) {
                            std::cout << "Client disconnected." << std::endl;
                            close(cfd);
                            break;
                        }
                    }
                });
                t.detach();
            }
            else{
                //linux::pthread_create
                pthread_t tid;
                int* pfd = new int(cfd);
                pthread_create(&tid, nullptr, [](void* arg)->void*{
                    int cfd = *static_cast<int*>(arg);
                    delete static_cast<int*>(arg);

                    char buffer[BUFFER_SIZE];
                    while(1)
                    {
                        ssize_t bytesRead = read(cfd, buffer, BUFFER_SIZE);
                        if(bytesRead <0){
                            perror("recv error");
                            close(cfd);
                            break;
                        }
                        else if (bytesRead > 0) {
                            std::cout << "Received from client: " << buffer << std::endl;
                            for(size_t i = 0; i < bytesRead; ++i){
                                buffer[i] = std::toupper(static_cast<unsigned char>(buffer[i]));
                            }
                            write(cfd, buffer, bytesRead);
                        } else if (bytesRead == 0) {
                            std::cout << "Client disconnected." << std::endl;
                            close(cfd);
                            break;
                        }
                    }
                    return nullptr;
                }, pfd);
                pthread_detach(tid);
            }
        }
    }
}