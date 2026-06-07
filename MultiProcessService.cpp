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

#define SERV_PORT 9999
#define BUFFER_SIZE 1024

void recycleChild(int signum){
    while(waitpid(-1, nullptr, WNOHANG) > 0);
}
int main()
{
    //忽略子进程的退出状态，让内核自动清理，不留下僵尸进程
    // signal(SIGCHLD, SIG_IGN);

    //用 sigaction
    struct sigaction saction{};
    saction.sa_handler = recycleChild;
    sigemptyset(&saction.sa_mask);
    saction.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &saction, nullptr) == -1){
        perror("sigaction error");
        return -1;
    }

    
    int lfd = socket(AF_INET,SOCK_STREAM,0);
    if(lfd == -1){
        perror(" SERVER: create socket error");
        return -1;
    }


    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(lfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1) {
        perror(" SERVER: bind error");
        return -1;
    }

    if(listen(lfd,128) == -1){
            perror(" SERVER: listen error");
            return -1;
    }

    struct sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);
    while(1){

        int cfd = accept(lfd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (cfd == -1) {
            perror("SERVER: accept error");
            continue;
        }
        
        int pid = fork();
        if(pid == 0){
            close(lfd);
            char buf[BUFFER_SIZE]{};
            while (true) {
                ssize_t rlen = read(cfd, buf, sizeof(buf));

                if (rlen == 0) {
                    std::cout << "Client closed connection" << std::endl;
                    break;
                } else if (rlen == -1) {
                    perror("SERVER: read error");
                    break;
                } else {
                    for (ssize_t i = 0; i < rlen; ++i) {
                        buf[i] = std::toupper(static_cast<unsigned char>(buf[i]));
                    }

                    write(cfd, buf, rlen);
                    write(STDOUT_FILENO, buf, rlen);
                }
            }
            close(cfd);
            _exit(0);
        }
        else if(pid > 0){
            close(cfd);
            continue;
        }
        else{
            perror(" SERVER: fork error");
            close(cfd);
        }
    }


    return 0;
}