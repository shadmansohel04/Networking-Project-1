#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include "pdu.h"
#include "registry.h"

#define SERVER_UDP_PORT 3000

int main(int argc, char **argv) {
  int sd, port;
  struct sockaddr_in server, client;
  socklen_t client_len;
  PDU rpdu;

  RegistryEntry registry[MAX_REG];
  int reg_count = 0;

  switch(argc) {
    case 1:
      port = SERVER_UDP_PORT;
      break;
    case 2:
      port = atoi(argv[1]);
      break;
    default:
      fprintf(stderr, "Usage: %s [port]\n", argv[0]);
      exit(1);
  }

  if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    fprintf(stderr, "Can't create a socket\n");
    exit(1);
  }

  bzero((char *)&server, sizeof(struct sockaddr_in));
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  
  if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
    fprintf(stderr, "Can't bind name to socket\n");
    exit(1);
  }

  printf("Index Server listening on UDP port %d...\n", port);

  while(1) {
    client_len = sizeof(client);
    bzero(&rpdu, sizeof(PDU));
    
    if (recvfrom(sd, &rpdu, sizeof(PDU), 0, (struct sockaddr *)&client, &client_len) < 0) {
      continue;
    }

    switch (rpdu.type) {
      case 'R':
        handle_registration(sd, &client, client_len, &rpdu, registry, &reg_count);
        break;
      case 'S':
        handle_search(sd, &client, client_len, &rpdu, registry, reg_count);
        break;
      case 'O':
        handle_list(sd, &client, client_len, registry, reg_count);
        break;
      case 'T':
        handle_deregistration(sd, &client, client_len, &rpdu, registry, &reg_count);
        break;
      default:
        printf("Received unknown PDU type: '%c'\n", rpdu.type);
        break;
    }
  }

  close(sd);
  return 0;
}
