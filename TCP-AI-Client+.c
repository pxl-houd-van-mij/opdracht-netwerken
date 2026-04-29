// AI_CLIENT.C — automatische gokker
// Compile: gcc ai_client.c -o ai
// Run:     ./ai <server_ip> <port>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define BUF_SIZE 128

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: ./ai <server_ip> <port>\n");
        return 1;
    }

    srand(time(NULL));

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in serverAddr;
    socklen_t serverLen = sizeof(serverAddr);

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);

    char buffer[BUF_SIZE];
    int guess = 50;

    while (1) {
        sprintf(buffer, "%d", guess);
        printf("AI guess: %d\n", guess);

        sendto(sock, buffer, strlen(buffer), 0,
               (struct sockaddr*)&serverAddr, serverLen);

        struct timeval tv;
        tv.tv_sec = 16;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        int n = recvfrom(sock, buffer, BUF_SIZE, 0,
                         (struct sockaddr*)&serverAddr, &serverLen);

        if (n > 0) {
            buffer[n] = '\0';
            printf("Server: %s\n", buffer);
        }

        guess += (rand() % 21) - 10; // random -10 tot +10
        if (guess < 0) guess = 0;
        if (guess > 99) guess = 99;

        sleep(1);
    }

    close(sock);
    return 0;
}
