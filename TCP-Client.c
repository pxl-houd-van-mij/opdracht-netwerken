// CLIENT.C — UDP Guessing Game Client
// Compile: gcc client.c -o client
// Run:     ./client <server_ip> <port>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 128

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: ./client <server_ip> <port>\n");
        return 1;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in serverAddr;
    socklen_t serverLen = sizeof(serverAddr);

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);

    char buffer[BUF_SIZE];

    while (1) {
        printf("Enter guess (0-99): ");
        fgets(buffer, BUF_SIZE, stdin);

        sendto(sock, buffer, strlen(buffer), 0,
               (struct sockaddr*)&serverAddr, serverLen);

        struct timeval tv;
        tv.tv_sec = 16;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        int n = recvfrom(sock, buffer, BUF_SIZE, 0,
                         (struct sockaddr*)&serverAddr, &serverLen);

        if (n < 0) {
            printf("You lost ?\n");
            continue;
        }

        buffer[n] = '\0';
        printf("Server: %s\n", buffer);

        if (strcmp(buffer, "You won !") == 0) {
            printf("🎉 YOU ARE THE WINNER\n");
        }
    }

    close(sock);
    return 0;
}
