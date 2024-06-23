#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdint.h>

void* recv_thread(void* argv) {
    int c = (intptr_t)argv; // intptr_t convert int
    while(1) {
        char buff[128] = {0};
        int n = recv(c, buff, 127, 0);// c:receive data buff:store 
        if (n <= 0) {
            break;
        }
        printf("recv(%d): %s\n", c, buff);
        send(c, "ok", 2, 0);
    }
    printf("client close\n");
    close(c); // close client
    return NULL; 
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // socket(ipv4,tcp,default
    assert(sockfd != -1);

    struct sockaddr_in saddr, caddr; 
    memset(&saddr, 0, sizeof(saddr)); // init

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(6000); // 端口号：主机服务器短整形：6000
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // ip地址

    int res = bind(sockfd, (struct sockaddr*)&saddr, sizeof(saddr)); // 绑定
    assert(res != -1);

    res = listen(sockfd, 5);
    assert(res != -1);

    while (1) {
        socklen_t len = sizeof(caddr);
        int c = accept(sockfd, (struct sockaddr*)&caddr, &len);
        if (c < 0) {
            continue;
        }

        printf("accept c = %d\n", c);
        pthread_t id;
        pthread_create(&id, NULL, recv_thread, (void*)(intptr_t)c); // 创建线程
        pthread_detach(id); // 分离线程，防止线程僵死
    }

    close(sockfd);
    return 0;
}