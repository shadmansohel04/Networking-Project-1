#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "pdu.h"

/* Helper to track locally registered files so we can de-register all on quit */
void add_local_file(char local_files[][11], int *count, const char *filename) {
  int i;
  for (i = 0; i < *count; i++) {
    if (strcmp(local_files[i], filename) == 0) return;
  }
  if (*count < MAX_LOCAL_FILES) {
    strcpy(local_files[*count], filename);
    (*count)++;
  }
}

void remove_local_file(char local_files[][11], int *count, const char *filename) {
  int i, j;
  for (i = 0; i < *count; i++) {
    if (strcmp(local_files[i], filename) == 0) {
      for (j = i; j < *count - 1; j++) {
        strcpy(local_files[j], local_files[j + 1]);
      }
      (*count)--;
      return;
    }
  }
}

/* Helper function to send an R-type PDU */
int send_registration_pdu(const char *server_ip, int server_port, const char *peer_name, const char *content_name, int my_tcp_port) {
  int udp_sock;
  struct sockaddr_in index_addr;
  PDU spdu, rpdu;
  socklen_t index_len;

  if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("Cannot create UDP socket");
    return 0;
  }

  bzero((char *)&index_addr, sizeof(struct sockaddr_in));
  index_addr.sin_family = AF_INET;
  index_addr.sin_port = htons(server_port);
  inet_aton(server_ip, &index_addr.sin_addr);

  spdu.type = 'R';
  bzero(spdu.data, MAX_DATA_LEN);
  snprintf(spdu.data, MAX_DATA_LEN, "%-10s%-10s%d", peer_name, content_name, my_tcp_port);

  sendto(udp_sock, &spdu, sizeof(PDU), 0, (struct sockaddr *)&index_addr, sizeof(index_addr));

  index_len = sizeof(index_addr);
  recvfrom(udp_sock, &rpdu, sizeof(PDU), 0, (struct sockaddr *)&index_addr, &index_len);
  close(udp_sock);

  if (rpdu.type == 'A') {
    return 1;
  } else if (rpdu.type == 'E') {
    printf("[ Error from Index Server: %s ]\n", rpdu.data);
  }
  return 0;
}

/* Helper function to send a T-type PDU for de-registration */
int send_deregistration_pdu(const char *server_ip, int server_port, const char *peer_name, const char *content_name) {
  int udp_sock;
  struct sockaddr_in index_addr;
  PDU spdu, rpdu;
  socklen_t index_len;

  if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("Cannot create UDP socket");
    return 0;
  }

  bzero((char *)&index_addr, sizeof(struct sockaddr_in));
  index_addr.sin_family = AF_INET;
  index_addr.sin_port = htons(server_port);
  inet_aton(server_ip, &index_addr.sin_addr);

  spdu.type = 'T';
  bzero(spdu.data, MAX_DATA_LEN);
  snprintf(spdu.data, MAX_DATA_LEN, "%-10s%-10s", peer_name, content_name);

  sendto(udp_sock, &spdu, sizeof(PDU), 0, (struct sockaddr *)&index_addr, sizeof(index_addr));

  index_len = sizeof(index_addr);
  recvfrom(udp_sock, &rpdu, sizeof(PDU), 0, (struct sockaddr *)&index_addr, &index_len);
  close(udp_sock);

  if (rpdu.type == 'A') {
    return 1;
  } else if (rpdu.type == 'E') {
    printf("[ Error from Index Server: %s ]\n", rpdu.data);
  }
  return 0;
}

/* Interactive registration */
void register_content(const char *server_ip, int server_port, char *my_peer_name, int my_tcp_port, char local_files[][11], int *local_count) {
  char content_name[11];
  char input_buf[256];
  int valid_content = 0;

  while (!valid_content) {
    printf("Enter Content Name to register (max 10 chars): ");
    if (fgets(input_buf, sizeof(input_buf), stdin) != NULL) {
      input_buf[strcspn(input_buf, "\n")] = '\0';
      if (strlen(input_buf) > 10 || strlen(input_buf) == 0) {
        printf("[ Error: Content Name must be 1-10 characters. ]\n");
      } else {
        strcpy(content_name, input_buf);
        valid_content = 1;
      }
    }
  }

  while (1) {
    if (send_registration_pdu(server_ip, server_port, my_peer_name, content_name, my_tcp_port)) {
      add_local_file(local_files, local_count, content_name);
      printf("\n[ Success: '%s' registered under peer '%s'! ]\n", content_name, my_peer_name);
      break;
    } else {
      printf("Please enter a different Peer Name (max 10 chars): ");
      if (fgets(input_buf, sizeof(input_buf), stdin) != NULL) {
        input_buf[strcspn(input_buf, "\n")] = '\0';
        if (strlen(input_buf) > 0 && strlen(input_buf) <= 10) {
          strcpy(my_peer_name, input_buf);
        }
      }
    }
  }
}

/* Interactive de-registration (Option 5) */
void deregister_content(const char *server_ip, int server_port, const char *my_peer_name, char local_files[][11], int *local_count) {
  char content_name[11];
  char input_buf[256];

  printf("Enter Content Name to de-register (max 10 chars): ");
  if (fgets(input_buf, sizeof(input_buf), stdin) != NULL) {
    input_buf[strcspn(input_buf, "\n")] = '\0';
    if (strlen(input_buf) == 0 || strlen(input_buf) > 10) {
      printf("[ Error: Content Name must be 1-10 characters. ]\n");
      return;
    }
    strcpy(content_name, input_buf);
  }

  if (send_deregistration_pdu(server_ip, server_port, my_peer_name, content_name)) {
    remove_local_file(local_files, local_count, content_name);
    printf("\n[ Success: '%s' de-registered from Index Server! ]\n", content_name);
  } else {
    printf("\n[ Failed to de-register '%s'. ]\n", content_name);
  }
}

/* De-registers all tracked content when the user quits */
void deregister_all_on_quit(const char *server_ip, int server_port, const char *my_peer_name, char local_files[][11], int *local_count) {
  int i;
  if (*local_count == 0) return;

  printf("[ De-registering %d registered file(s) before quitting... ]\n", *local_count);
  for (i = 0; i < *local_count; i++) {
    if (send_deregistration_pdu(server_ip, server_port, my_peer_name, local_files[i])) {
      printf("  -> De-registered '%s'\n", local_files[i]);
    }
  }
  *local_count = 0;
}

/* List on-line registered content (Option 4 - O-type PDU) */
void list_online_content(const char *server_ip, int server_port) {
  int udp_sock;
  struct sockaddr_in index_addr;
  PDU spdu, rpdu;
  socklen_t index_len;
  int count = 0;

  if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("Cannot create UDP socket");
    return;
  }

  bzero((char *)&index_addr, sizeof(struct sockaddr_in));
  index_addr.sin_family = AF_INET;
  index_addr.sin_port = htons(server_port);
  inet_aton(server_ip, &index_addr.sin_addr);

  bzero(&spdu, sizeof(PDU));
  spdu.type = 'O';

  sendto(udp_sock, &spdu, sizeof(PDU), 0, (struct sockaddr *)&index_addr, sizeof(index_addr));

  index_len = sizeof(index_addr);

  printf("\n=== On-Line Registered Content ===\n");
  while (1) {
    bzero(&rpdu, sizeof(PDU));
    if (recvfrom(udp_sock, &rpdu, sizeof(PDU), 0, (struct sockaddr *)&index_addr, &index_len) < 0) {
      perror("Error receiving content list");
      break;
    }

    if (rpdu.type == 'O') {
      count++;
      printf("  %d. %s\n", count, rpdu.data);
    } else if (rpdu.type == 'A') {
      if (count == 0) {
        printf("  (No content currently registered on the server)\n");
      }
      break;
    } else if (rpdu.type == 'E') {
      printf("  [ Error: %s ]\n", rpdu.data);
      break;
    } else {
      printf("  [ Error: Unexpected response from server ]\n");
      break;
    }
  }
  printf("==================================\n");

  close(udp_sock);
}

#endif /* REGISTRATION_H */
