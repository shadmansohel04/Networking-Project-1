#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>

#define SERVER_UDP_PORT 3000
#define MAX_DATA_LEN 100
#define MAX_REG 100

typedef struct {
  char type;
  char data[MAX_DATA_LEN];
} PDU;

typedef struct {
  char peer_name[11];
  char content_name[11];
  char port[10];
  char ip[20];
} RegistryEntry;

int main(int argc, char **argv) {
  int sd, port;
  struct sockaddr_in server, client;
  socklen_t client_len;
  PDU rpdu, spdu;

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

    if (rpdu.type == 'R') {
      char peer_name[11];
      char content_name[11];
      char peer_port[10];
      char client_ip[20];

      /* Extract the IP address from the client socket structure */
      strcpy(client_ip, inet_ntoa(client.sin_addr));

      sscanf(rpdu.data, "%10s%10s%s", peer_name, content_name, peer_port);
      printf("Received Registration Request: Peer: %s, Content: %s, Port: %s, IP: %s\n", 
             peer_name, content_name, peer_port, client_ip);

      int conflict = 0;
      int i; 
      for (i = 0; i < reg_count; i++) {
        if (strcmp(registry[i].peer_name, peer_name) == 0 && 
            strcmp(registry[i].content_name, content_name) == 0) {
          conflict = 1;
          break;
        }
      }

      bzero(&spdu, sizeof(PDU));
      if (conflict == 1) {
        spdu.type = 'E';
        strcpy(spdu.data, "ERRMSG: Peer name conflict for this content.");
        sendto(sd, &spdu, sizeof(PDU), 0, (struct sockaddr *)&client, client_len);
        printf("Registration rejected: Conflict detected.\n");
      } 
      else {
        if (reg_count < MAX_REG) {
          strcpy(registry[reg_count].peer_name, peer_name);
          strcpy(registry[reg_count].content_name, content_name);
          strcpy(registry[reg_count].port, peer_port);
          strcpy(registry[reg_count].ip, client_ip); /* Save the IP address */
          reg_count++;
          
          spdu.type = 'A';
          sendto(sd, &spdu, sizeof(PDU), 0, (struct sockaddr *)&client, client_len);
          printf("Registration successful. Total entries: %d\n", reg_count);
        } 
        else {
          spdu.type = 'E';
          strcpy(spdu.data, "ERRMSG: Server registry full.");
          sendto(sd, &spdu, sizeof(PDU), 0, (struct sockaddr *)&client, client_len);
        }
      }
    }
  }

  close(sd);
  return 0;
}