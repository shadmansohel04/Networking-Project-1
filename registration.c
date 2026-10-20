#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct PDU {
  char type;
  char data[100];
};

void register_content(const char *server_ip, int server_port) {
  int sd;
  struct sockaddr_in server;
  struct PDU pdu_send, pdu_recv;
  char peer_name[BUFLEN];
  char content_name[BUFLEN];
  socklen_t server_len = sizeof(server);

  if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("Can't create UDP socket");
    return;
  }

  bzero((char *)&server, sizeof(struct sockaddr_in));
  server.sin_family = AF_INET;
  server.sin_port = htons(server_port);
  inet_aton(server_ip, &server.sin_addr);

  while (1) {
    printf("Enter Peer Name (max 10 chars): ");
    if (fgets(peer_name, BUFLEN, stdin) != NULL) {
      peer_name[strcspn(peer_name, "\n")] = '\0';
    }

    printf("Enter Content Name (max 10 chars): ");
    if (fgets(content_name, BUFLEN, stdin) != NULL) {
      content_name[strcspn(content_name, "\n")] = '\0';
    }

    pdu_send.type = 'R';
    bzero(pdu_send.data, 100);
    
    /* Format forces peer_name and content_name to be exactly 10 bytes */
    /* Appending '0' as a dummy placeholder for the address port for now */
    snprintf(pdu_send.data, 100, "%-10s%-10s%d", peer_name, content_name, 0);

    sendto(sd, &pdu_send, sizeof(pdu_send), 0, (struct sockaddr *)&server, sizeof(server));

    if (recvfrom(sd, &pdu_recv, sizeof(pdu_recv), 0, (struct sockaddr *)&server, &server_len) > 0) {
      if (pdu_recv.type == 'E') {
        printf("\n[ Error: Peer name conflict or registration failed. Please try again. ]\n\n");
      } 
      else if (pdu_recv.type == 'A') {
        printf("\n[ Success: now starting file transfer ]\n");
        break;
      }
    }
  }
  
  close(sd);
}

#endif