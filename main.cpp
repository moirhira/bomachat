#include <stdio.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); return 1; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(6667);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    { perror("bind"); return 1; }

    if (listen(sockfd, 10) < 0)
    { perror("listen"); return 1; }

    struct pollfd fds[100];
    int nfds = 1;
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;

    while (1) {
        poll(fds, nfds, -1);
        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {  // fix 1: & not &&
                if (fds[i].fd == sockfd) {
                    int client_fd = accept(sockfd, NULL, NULL);
                    fds[nfds].fd = client_fd;
                    fds[nfds].events = POLLIN;  // fix 2: .events not .fd
                    nfds++;
                    printf("new client connected\n");
                } else {
                    char buffer[1024];
                    int byts = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);
                    if (byts <= 0) {
                        close(fds[i].fd);
                        fds[i] = fds[nfds - 1];  // fix 5: remove from array
                        nfds--;
                        i--;
                    } else {
                        buffer[byts] = '\0';
                        printf("client sent: %s\n", buffer);
                        for (int j = 1; j < nfds; j++) {  // fix 3: skip server socket
                            if (fds[j].fd != fds[i].fd)   // skip sender
                                send(fds[j].fd, buffer, byts, 0);  // fix 4: send byts
                        }
                    }
                }
            }
        }
    }
}