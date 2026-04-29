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

// Struct om info van de ronde bij te houden
typedef struct {
    int bestGuess;                 // beste gok tot nu toe
    struct sockaddr_in bestClient; // IP + poort van beste gokker
    int hasGuess;                  // is er al een gok ontvangen?
} GameState;

// Nieuwe ronde starten
void resetGame(GameState *state, int *secret) {
    *secret = rand() % 100;        // nieuw geheim getal
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

    // UDP socket aanmaken
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    // Server adres instellen
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(atoi(argv[1]));

    // Socket binden aan poort
    if (bind(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind");
        return 1;
    }

    GameState state;
    int secret;
    resetGame(&state, &secret);

    int timeout = 8; // start time-out
    struct timeval tv;

    char buffer[BUF_SIZE];

    while (1) {
        // Time-out instellen
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        // Wachten op gok
        int n = recvfrom(sock, buffer, BUF_SIZE, 0,
                         (struct sockaddr*)&clientAddr, &clientLen);

        if (n < 0) {
            // TIME-OUT EVENT
            if (state.hasGuess) {
                // Nominatie van winnaar
                sendto(sock, "You won ?", 9, 0,
                       (struct sockaddr*)&state.bestClient, clientLen);

                // Late messages 16 sec
                tv.tv_sec = 16;
                tv.tv_usec = 0;
                setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

                while (1) {
                    n = recvfrom(sock, buffer, BUF_SIZE, 0,
                                 (struct sockaddr*)&clientAddr, &clientLen);

                    if (n < 0) break;

                    // Late message → verlies
                    sendto(sock, "You lost !", 10, 0,
                           (struct sockaddr*)&clientAddr, clientLen);
                }

                // Echte winnaar
                sendto(sock, "You won !", 9, 0,
                       (struct sockaddr*)&state.bestClient, clientLen);
            }

            // Nieuwe ronde
            resetGame(&state, &secret);
            timeout = 8;
            continue;
        }

        buffer[n] = '\0';
        int guess = atoi(buffer);
        printf("Received guess %d\n", guess);

        int diff = abs(secret - guess);

        // Is dit de beste gok?
        if (!state.hasGuess || diff < abs(secret - state.bestGuess)) {
            state.bestGuess = guess;
            state.bestClient = clientAddr;
            state.hasGuess = 1;
        }

        // Time-out halveren
        timeout /= 2;
        if (timeout < 1) timeout = 1;
    }

    close(sock);
    return 0;
}
