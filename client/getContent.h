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

int search_content(const char *server_ip, int server_port, char *out_ip, int *out_port, char *out_filename) {
  int udp_sock;
  struct sockaddr_in index_addr;
  char peer_name[11];
  char content_name[11];
  char input_buf[256];
  PDU spdu, rpdu;

  if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("Cannot create UDP socket");
    return 0;
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
    char resp_peer[11];
    char resp_ip[21];
    char resp_port[11];
    
    sscanf(rpdu.data, "%10s%20s%10s", resp_peer, resp_ip, resp_port);
    
    printf("\n[ Success: Content found at Peer: %s, IP: %s, Port: %s ]\n", resp_peer, resp_ip, resp_port);
    
    if (out_ip != NULL) strcpy(out_ip, resp_ip);
    if (out_port != NULL) *out_port = atoi(resp_port);
    if (out_filename != NULL) strcpy(out_filename, content_name);
    
    close(udp_sock);
    return 1;
  } else if (rpdu.type == 'E') {
    printf("\n[ Error: Content not found or unavailable. ]\n");
  } else {
    printf("\n[ Error: Unknown response from server. ]\n");
  }

  close(udp_sock);
  return 0;
}

int download_content(const char *ip, int port, const char *filename) {
  int sd, n;
  struct sockaddr_in peer_addr;
  PDU spdu, rpdu;

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Can't create a socket");
    return 0;
  }

  bzero((char *)&peer_addr, sizeof(struct sockaddr_in));
  peer_addr.sin_family = AF_INET;
  peer_addr.sin_port = htons(port);
  
  if (inet_aton(ip, &peer_addr.sin_addr) == 0) {
    fprintf(stderr, "Invalid peer IP address\n");
    close(sd);
    return 0;
  }

  if (connect(sd, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) == -1) {
    perror("Can't connect to peer");
    close(sd);
    return 0;
  }

  bzero(&spdu, sizeof(PDU));
  spdu.type = 'D'; 
  strncpy(spdu.data, filename, MAX_DATA_LEN - 1);
  write(sd, &spdu, sizeof(PDU));

  n = read(sd, &rpdu, sizeof(PDU));
  
  if (n > 0) {
    if (rpdu.type == 'E') {
      write(1, rpdu.data, n - 1);
      
      while ((n = read(sd, &rpdu, sizeof(PDU))) > 0) {
        write(1, rpdu.data, n - 1);
      }
      printf("\n");
    } 
    else if (rpdu.type == 'F') {
      FILE *fp = fopen(filename, "wb");
      if (fp != NULL) {
        fwrite(rpdu.data, 1, n - 1, fp);
        
        while ((n = read(sd, &rpdu, sizeof(PDU))) > 0) {
          fwrite(rpdu.data, 1, n - 1, fp);
        }
        
        fclose(fp);
        printf("\n[ Success: File downloaded successfully to '%s'. ]\n", filename);
      } else {
        perror("Failed to open local file for writing");
      }
    }
  }

  close(sd);
  return 1;
}

#endif
