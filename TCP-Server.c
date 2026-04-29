// SERVER.C — UDP Guessing Game Server
// Compile: gcc server.c -o server
// Run:     ./server <port>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

#define BUF_SIZE 128

typedef struct {
    int bestGuess;
    struct sockaddr_in bestClient;
    int hasGuess;
} GameState;

void resetGame(GameState *state, int *secret) {
    *secret = rand() % 100;
    state->bestGuess = 9999;
    state->hasGuess = 0;
    printf("\n--- NEW ROUND --- Secret = %d\n", *secret);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./server <port>\n");
        return 1;
    }

    srand(time(NULL));

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(atoi(argv[1]));

    if (bind(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind");
        return 1;
    }

    GameState state;
    int secret;
    resetGame(&state, &secret);

    int timeout = 8; // seconds
    struct timeval tv;

    char buffer[BUF_SIZE];

    while (1) {
        tv.tv_sec = timeout;
        tv.tv_usec = 0;

        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        int n = recvfrom(sock, buffer, BUF_SIZE, 0,
                         (struct sockaddr*)&clientAddr, &clientLen);

        if (n < 0) {
            // TIMEOUT EVENT
            if (state.hasGuess) {
                // Send "You won ?" to nominated client
                sendto(sock, "You won ?", 9, 0,
                       (struct sockaddr*)&state.bestClient, clientLen);

                // Now accept late messages for 16 seconds
                tv.tv_sec = 16;
                tv.tv_usec = 0;
                setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

                while (1) {
                    n = recvfrom(sock, buffer, BUF_SIZE, 0,
                                 (struct sockaddr*)&clientAddr, &clientLen);

                    if (n < 0) break; // 16 sec passed

                    // Late message → "You lost !"
                    sendto(sock, "You lost !", 10, 0,
                           (struct sockaddr*)&clientAddr, clientLen);
                }

                // Winner gets final "You won !"
                sendto(sock, "You won !", 9, 0,
                       (struct sockaddr*)&state.bestClient, clientLen);
            }

            // Start new round
            resetGame(&state, &secret);
            timeout = 8;
            continue;
        }

        buffer[n] = '\0';
        int guess = atoi(buffer);
        printf("Received guess %d\n", guess);

        int diff = abs(secret - guess);

        if (!state.hasGuess || diff < abs(secret - state.bestGuess)) {
            state.bestGuess = guess;
            state.bestClient = clientAddr;
            state.hasGuess = 1;
        }

        timeout /= 2;
        if (timeout < 1) timeout = 1;
    }

    close(sock);
    return 0;
}
