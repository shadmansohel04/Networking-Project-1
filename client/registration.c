#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_DATA_LEN 100

typedef struct {
  char type;
  char data[MAX_DATA_LEN];
} PDU;

void register_content(const char *server_ip, int server_port) {
  int udp_sock;
  struct sockaddr_in index_addr;
  char peer_name[11];
  char content_name[11];
  PDU spdu, rpdu;

  /* Placeholder port until TCP download logic is implemented later */
  int my_tcp_port = 8080; 

  if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("Cannot create UDP socket");
    return;
  }

  bzero((char *)&index_addr, sizeof(struct sockaddr_in));
  index_addr.sin_family = AF_INET;
  index_addr.sin_port = htons(server_port);
  inet_aton(server_ip, &index_addr.sin_addr);

  int registered = 0;

  while (registered == 0) {
    printf("Enter Peer Name (max 10 chars): ");
    scanf("%10s", peer_name);
    
    printf("Enter Content Name (max 10 chars): ");
    scanf("%10s", content_name);
    
    while (getchar() != '\n'); 

    spdu.type = 'R';
    bzero(spdu.data, MAX_DATA_LEN);
    snprintf(spdu.data, MAX_DATA_LEN, "%-10s%-10s%d", peer_name, content_name, my_tcp_port);

    sendto(udp_sock, &spdu, sizeof(PDU), 0, (struct sockaddr *)&index_addr, sizeof(index_addr));

    socklen_t index_len = sizeof(index_addr);
    recvfrom(udp_sock, &rpdu, sizeof(PDU), 0, (struct sockaddr *)&index_addr, &index_len);

    if (rpdu.type == 'E') {
      printf("[ Error: Peer name already exists or registration failed. Please try a different name. ]\n");
    } else if (rpdu.type == 'A') {
      printf("\n[ Success: Content registered successfully! ]\n");
      registered = 1;
    } else {
      printf("[ Error: Unknown response from server. ]\n");
      break;
    }
  }

  close(udp_sock);
}

#endif