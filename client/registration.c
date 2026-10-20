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
  char input_buf[256];
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
    
    /* Ask for Peer Name and validate length */
    int valid_peer = 0;
    while (valid_peer == 0) {
      printf("Enter Peer Name (max 10 chars): ");
      if (fgets(input_buf, sizeof(input_buf), stdin) != NULL) {
        input_buf[strcspn(input_buf, "\n")] = '\0';
        
        if (strlen(input_buf) > 10) {
          printf("[ Error: Peer Name exceeds 10 characters. Please try again. ]\n");
        } else if (strlen(input_buf) == 0) {
          printf("[ Error: Peer Name cannot be empty. Please try again. ]\n");
        } else {
          strcpy(peer_name, input_buf);
          valid_peer = 1;
        }
      }
    }
    
    /* Ask for Content Name and validate length */
    int valid_content = 0;
    while (valid_content == 0) {
      printf("Enter Content Name (max 10 chars): ");
      if (fgets(input_buf, sizeof(input_buf), stdin) != NULL) {
        input_buf[strcspn(input_buf, "\n")] = '\0';
        
        if (strlen(input_buf) > 10) {
          printf("[ Error: Content Name exceeds 10 characters. Please try again. ]\n");
        } else if (strlen(input_buf) == 0) {
          printf("[ Error: Content Name cannot be empty. Please try again. ]\n");
        } else {
          strcpy(content_name, input_buf);
          valid_content = 1;
        }
      }
    }

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