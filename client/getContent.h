#ifndef GETCONTENT_H
#define GETCONTENT_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#ifndef MAX_DATA_LEN
#define MAX_DATA_LEN 100

typedef struct {
  char type;
  char data[MAX_DATA_LEN];
} PDU;
#endif

void get_content(const char *server_ip, int server_port) {
  int udp_sock;
  struct sockaddr_in index_addr;
  char peer_name[11];
  char content_name[11];
  char input_buf[256];
  PDU spdu, rpdu;

  if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("Cannot create UDP socket");
    return;
  }

  bzero((char *)&index_addr, sizeof(struct sockaddr_in));
  index_addr.sin_family = AF_INET;
  index_addr.sin_port = htons(server_port);
  inet_aton(server_ip, &index_addr.sin_addr);

  printf("Enter Peer Name to search (max 10 chars): ");
  if (fgets(input_buf, sizeof(input_buf), stdin) != NULL) {
    input_buf[strcspn(input_buf, "\n")] = '\0';
    strncpy(peer_name, input_buf, 10);
    peer_name[10] = '\0';
  }

  printf("Enter Content Name to search (max 10 chars): ");
  if (fgets(input_buf, sizeof(input_buf), stdin) != NULL) {
    input_buf[strcspn(input_buf, "\n")] = '\0';
    strncpy(content_name, input_buf, 10);
    content_name[10] = '\0';
  }

  spdu.type = 'S';
  bzero(spdu.data, MAX_DATA_LEN);
  snprintf(spdu.data, MAX_DATA_LEN, "%-10s%-10s", peer_name, content_name);

  sendto(udp_sock, &spdu, sizeof(PDU), 0, (struct sockaddr *)&index_addr, sizeof(index_addr));

  socklen_t index_len = sizeof(index_addr);
  recvfrom(udp_sock, &rpdu, sizeof(PDU), 0, (struct sockaddr *)&index_addr, &index_len);

  if (rpdu.type == 'S') {
    printf("\n[ Success: Content found! ]\n");
    printf("[ attempting download... ]\n");
  } else if (rpdu.type == 'E') {
    printf("\n[ Error: Content not found or unavailable. ]\n");
  } else {
    printf("\n[ Error: Unknown response from server. ]\n");
  }

  close(udp_sock);
}

#endif