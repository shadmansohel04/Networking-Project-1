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
  int use_count;
} RegistryEntry;

void handle_registration(int sd, struct sockaddr_in *client, socklen_t client_len, PDU *rpdu, RegistryEntry *registry, int *reg_count);
void handle_search(int sd, struct sockaddr_in *client, socklen_t client_len, PDU *rpdu, RegistryEntry *registry, int reg_count);
void handle_deregistration(int sd, struct sockaddr_in *client, socklen_t client_len, PDU *rpdu, RegistryEntry *registry, int *reg_count);
void handle_list(int sd, struct sockaddr_in *client, socklen_t client_len, RegistryEntry *registry, int reg_count);

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

void handle_registration(int sd, struct sockaddr_in *client, socklen_t client_len, PDU *rpdu, RegistryEntry *registry, int *reg_count) {
  PDU spdu;
  char peer_name[11];
  char content_name[11];
  char peer_port[10];
  char client_ip[20];

  strcpy(client_ip, inet_ntoa(client->sin_addr));

  sscanf(rpdu->data, "%10s%10s%s", peer_name, content_name, peer_port);
  printf("Received Registration Request: Peer: %s, Content: %s, Port: %s, IP: %s\n", 
         peer_name, content_name, peer_port, client_ip);

  int conflict = 0;
  int i; 
  for (i = 0; i < *reg_count; i++) {
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
    sendto(sd, &spdu, sizeof(PDU), 0, (struct sockaddr *)client, client_len);
    printf("Registration rejected: Conflict detected.\n");
  } 
  else {
    if (*reg_count < MAX_REG) {
      strcpy(registry[*reg_count].peer_name, peer_name);
      strcpy(registry[*reg_count].content_name, content_name);
      strcpy(registry[*reg_count].port, peer_port);
      strcpy(registry[*reg_count].ip, client_ip);
      registry[*reg_count].use_count = 0;
      (*reg_count)++;
      
      spdu.type = 'A';
      sendto(sd, &spdu, sizeof(PDU), 0, (struct sockaddr *)client, client_len);
      printf("Registration successful. Total entries: %d\n", *reg_count);
    } 
    else {
      spdu.type = 'E';
      strcpy(spdu.data, "ERRMSG: Server registry full.");
      sendto(sd, &spdu, sizeof(PDU), 0, (struct sockaddr *)client, client_len);
      printf("Registration rejected: Server registry full.\n");
    }
  }
}

void handle_search(int sd, struct sockaddr_in *client, socklen_t client_len, PDU *rpdu, RegistryEntry *registry, int reg_count) {
  PDU spdu;
  char req_peer[11];
  char req_content[11];
  
  sscanf(rpdu->data, "%10s%10s", req_peer, req_content);
  printf("Received Search Request from Peer: %s for Content: %s\n", req_peer, req_content);

  int best_idx = -1;
  int i;
  for (i = 0; i < reg_count; i++) {
    if (strcmp(registry[i].content_name, req_content) == 0) {
      if (best_idx == -1 || registry[i].use_count < registry[best_idx].use_count) {
        best_idx = i;
      }
    }
  }

  bzero(&spdu, sizeof(PDU));
  if (best_idx != -1) {
    registry[best_idx].use_count++;
    
    spdu.type = 'S';
    snprintf(spdu.data, MAX_DATA_LEN, "%-10s%-20s%-10s", 
             registry[best_idx].peer_name, 
             registry[best_idx].ip, 
             registry[best_idx].port);
    sendto(sd, &spdu, sizeof(PDU), 0, (struct sockaddr *)client, client_len);
    printf("Search successful. Directed '%s' to Peer '%s' (use_count: %d)\n", 
           req_content, registry[best_idx].peer_name, registry[best_idx].use_count);
  } 
  else {
    spdu.type = 'E';
    strcpy(spdu.data, "ERRMSG: Content not found.");
    sendto(sd, &spdu, sizeof(PDU), 0, (struct sockaddr *)client, client_len);
    printf("Search failed: Content %s not found.\n", req_content);
  }
}

void handle_list(int sd, struct sockaddr_in *client, socklen_t client_len, RegistryEntry *registry, int reg_count) {
  PDU spdu;
  char sent_names[MAX_REG][11];
  int sent_count = 0;

  printf("Received List Request (O-type).\n");

  for (int i = 0; i < reg_count; i++) {
    /* Deduplicate so each unique content name is listed once */
    int already_sent = 0;
    for (int j = 0; j < sent_count; j++) {
      if (strcmp(sent_names[j], registry[i].content_name) == 0) {
        already_sent = 1;
        break;
      }
    }

    if (!already_sent) {
      strcpy(sent_names[sent_count++], registry[i].content_name);

      bzero(&spdu, sizeof(PDU));
      spdu.type = 'O';
      strncpy(spdu.data, registry[i].content_name, MAX_DATA_LEN - 1);
      sendto(sd, &spdu, sizeof(PDU), 0, (struct sockaddr *)client, client_len);
    }
  }

  /* Send A-type PDU to mark the end of the content list */
  bzero(&spdu, sizeof(PDU));
  spdu.type = 'A';
  sendto(sd, &spdu, sizeof(PDU), 0, (struct sockaddr *)client, client_len);
  printf("Sent %d unique content item(s) to peer.\n", sent_count);
}

void handle_deregistration(int sd, struct sockaddr_in *client, socklen_t client_len, PDU *rpdu, RegistryEntry *registry, int *reg_count) {
  PDU spdu;
  char peer_name[11];
  char content_name[11];

  sscanf(rpdu->data, "%10s%10s", peer_name, content_name);
  printf("Received De-Registration Request: Peer: %s, Content: %s\n", peer_name, content_name);

  int found_idx = -1;
  for (int i = 0; i < *reg_count; i++) {
    if (strcmp(registry[i].peer_name, peer_name) == 0 &&
        strcmp(registry[i].content_name, content_name) == 0) {
      found_idx = i;
      break;
    }
  }

  bzero(&spdu, sizeof(PDU));
  if (found_idx != -1) {
    for (int j = found_idx; j < *reg_count - 1; j++) {
      registry[j] = registry[j + 1];
    }
    (*reg_count)--;

    spdu.type = 'A';
    sendto(sd, &spdu, sizeof(PDU), 0, (struct sockaddr *)client, client_len);
    printf("De-registration successful. Total entries remaining: %d\n", *reg_count);
  } 
  else {
    spdu.type = 'E';
    strcpy(spdu.data, "ERRMSG: Matching peer/content entry not found.");
    sendto(sd, &spdu, sizeof(PDU), 0, (struct sockaddr *)client, client_len);
    printf("De-registration failed: Entry not found.\n");
  }
}
