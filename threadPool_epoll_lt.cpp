#include <iostream>
#include <thread>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <algorithm>
#include <atomic>
#include <future>
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
#include <memory>
#include <stdexcept>
#include <chrono>
#include <type_traits>

#define BUFFER_SIZE 1024
#define SERVER_PORT 11111

class ThreadPool{
    public:
        ThreadPool(int _MAX_THREAD = 32,int _MIN_THRED = 4,int _MAX_TAKS = 100)
            :MAX_THREAD(_MAX_THREAD)
            ,MIN_THREAD(_MIN_THRED)
            ,LIVE_THREAD(0)
            ,BUSY_THREAD(0)
            ,MAX_TASKS(_MAX_TAKS)
            ,stop(false)
        {
            workers.reserve(MAX_THREAD);

            initWorkers();

            adminThred = std::thread(&ThreadPool::adminLoop,this);
        }
        ~ThreadPool(){
            {
                std::lock_guard<std::mutex> lock(mtx);
                stop = true;
            }

            cvTask.notify_all();
            cvQueueFull.notify_all();

            if(adminThred.joinable()){
                adminThred.join();
            }

            for(auto& worker : workers){
                if(worker.joinable()){
                    worker.join();
                }
            }
        }
    public:
        template<class F,class ...Args>
        auto addTask(F &&f, Args && ...args)
         -> std::future<std::invoke_result_t<F, Args...>>
        {
            using RetType = std::invoke_result_t<F, Args...>;

            auto taskPtr = std::make_shared<std::packaged_task<RetType()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );

            std::future<RetType> fut = taskPtr->get_future();

            {
                std::unique_lock<std::mutex> lock(mtx);

                cvQueueFull.wait(lock, [this]() {
                    return stop.load() || tasks.size() < static_cast<size_t>(MAX_TASKS);
                });

                if (stop.load()) {
                    throw std::runtime_error("ThreadPool has stopped");
                }

                tasks.emplace([taskPtr]() {
                    (*taskPtr)();
                });
            }

            cvTask.notify_one();

            return fut;
        }
    private:
        void initWorkers(){
            for(int i = 0;i < MIN_THREAD;i++){
                workers.emplace_back(&ThreadPool::workerLoop,this);
                LIVE_THREAD++;
            }
        }
        void adminLoop(){
            while(!stop){
                std::this_thread::sleep_for(std::chrono::seconds(2));

                std::unique_lock<std::mutex> lock(mtx);

                if(stop){
                    break;
                }

                int queueSize = static_cast<int>(tasks.size());
                int live = LIVE_THREAD;
                int busy = BUSY_THREAD;

                //expen
                // if(queueSize > live && live < MAX_THREAD){
                //     int addCount = std::min(queueSize - live, MAX_THREAD);
                    
                //     for(int i = 0; i < addCount; i++){
                //         workers.emplace_back(&ThreadPool::workerLoop,this);
                //         LIVE_THREAD++;
                //     }

                //     cvTask.notify_all();
                // }
                // //delete
                // if(busy * 2 < LIVE_THREAD && LIVE_THREAD > MIN_THREAD){
                    
                // }
            }
        }
        void workerLoop(){
            while(true){
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(mtx);

                    cvTask.wait(lock, [this]() {
                        return stop || !tasks.empty();
                    });

                    //delete thread
                    // while(tasks.empty() && !stop){

                    // }

                    if(stop && tasks.empty()){
                        LIVE_THREAD--;
                        return;
                    }

                    task = std::move(tasks.front());
                    tasks.pop();

                    cvQueueFull.notify_one();

                    BUSY_THREAD++;
                }

                    task();
                    // std::cout << "thread id = "
                    //             << std::this_thread::get_id()
                    //             << " finish task result :" 
                    //             << std::endl;

                    BUSY_THREAD--;
            }
        }
    private:
        int MAX_THREAD;
        int MIN_THREAD;
        
        std::atomic<int> LIVE_THREAD;
        std::atomic<int> BUSY_THREAD;


        int MAX_TASKS;

        std::thread adminThred;

        std::vector<std::thread>            workers;
        std::queue<std::function<void()>>   tasks;

        std::mutex mtx;
        std::condition_variable cvTask;
        std::condition_variable cvQueueFull;

        std::atomic<bool> stop; 

        
};

int main(){

    ThreadPool pool(8, 2, 10);
    
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
                

                pool.addTask([epfd,fd](){
                    char buffer[BUFFER_SIZE];
                    ssize_t byte_read = read(fd,buffer,sizeof(buffer));

                    if(byte_read == 0){
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                        close(fd);
                        // continue;
                    }
                    else if(byte_read < 0){
                        perror("SERVER : read");
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                        close(fd);
                        // continue;
                    }
                    else {
                        printf("Received from client : %.*s",(int)byte_read,buffer);

                        for(int j = 0;j<byte_read;j++){
                            buffer[j] = toupper((unsigned char)buffer[j]);
                        }

                        write(fd,buffer,byte_read);
                        write(STDOUT_FILENO,buffer,byte_read);

                    }
                });

                
            }
        }
    }

    return 0;
}