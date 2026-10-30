#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#include "registration.h"
#include "getContent.h"

#define BUFLEN 256
#define MY_TCP_PORT 8080

void start_peer_server(int tcp_port) {
  int sd, new_sd;
  struct sockaddr_in server, client;
  socklen_t client_len = sizeof(client);

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Peer server: Can't create listening socket");
    exit(1);
  }

  bzero((char *)&server, sizeof(struct sockaddr_in));
  server.sin_family = AF_INET;
  server.sin_port = htons(tcp_port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
    perror("Peer server: Can't bind listening socket");
    exit(1);
  }

  listen(sd, 5);

  while (1) {
    new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
    if (new_sd < 0) continue;

    if (fork() == 0) {
      close(sd);
      PDU rpdu, spdu;
      
      int n = read(new_sd, &rpdu, sizeof(PDU));
      if (n > 0 && rpdu.type == 'D') {
        printf("\n[ Peer Server: Received request for file '%s' ]\n", rpdu.data);
        
        FILE *fp = fopen(rpdu.data, "rb");
        bzero(&spdu, sizeof(PDU));
        
        if (fp == NULL) {
          spdu.type = 'E';
          strcpy(spdu.data, "ERRMSG: File not found on this peer.");
          write(new_sd, &spdu, sizeof(PDU));
          printf("[ Peer Server: File not found. Error sent to peer. ]\n");
        } else {
          spdu.type = 'F';
          int bytes_read;
          
          while ((bytes_read = fread(spdu.data, 1, MAX_DATA_LEN - 1, fp)) > 0) {
            write(new_sd, &spdu, bytes_read + 1);
            bzero(spdu.data, MAX_DATA_LEN); 
          }
          fclose(fp);
          printf("[ Peer Server: File transfer complete. ]\n");
        }
      }
      
      close(new_sd);
      exit(0); 
    }
    close(new_sd); 
  }
}

int main(int argc, char **argv) {
  char host[BUFLEN];
  int port;
  char input_buffer[BUFLEN];

  pid_t server_pid = fork();
  if (server_pid == 0) {
    start_peer_server(MY_TCP_PORT);
    exit(0);
  }
  
  printf("Enter the IP address of the index server: ");
  if (fgets(host, BUFLEN, stdin) != NULL) {
    host[strcspn(host, "\n")] = '\0';
  }

  printf("Enter the port of the index server: ");
  if (fgets(input_buffer, BUFLEN, stdin) != NULL) {
    port = atoi(input_buffer);
  }

  while(1) {
    printf("\n--- P2P Application Menu ---\n");
    printf("1. Register Content (R-type)\n");
    printf("2. Search Content (S-type)\n");
    printf("3. Download Content (D-type)\n");
    printf("4. List On-Line Registered Content (O-type)\n");
    printf("5. De-Register Content (T-type)\n");
    printf("Type 'exit' to quit\n");
    printf("\nEnter your choice: ");

    if (fgets(input_buffer, BUFLEN, stdin) == NULL) {
      break;
    }

    input_buffer[strcspn(input_buffer, "\n")] = '\0';

    if (strcmp(input_buffer, "exit") == 0) {
      printf("Exiting application. Goodbye!\n");
      break;
    }

    int choice = atoi(input_buffer);

    switch(choice) {
      case 1:
        printf("\n[ Action: Registering content... ]\n");
        register_content(host, port);
        break;
      case 2:
        printf("\n[ Action: Searching for content... ]\n");
        {
          char dummy_ip[21];
          int dummy_port;
          char dummy_file[11];
          search_content(host, port, dummy_ip, &dummy_port, dummy_file);
        }
        break;
      case 3:
        printf("\n[ Action: Downloading content... ]\n");
        {
          char peer_ip[21];
          int peer_port;
          char filename[11];
          
          if (search_content(host, port, peer_ip, &peer_port, filename) == 1) {
            printf("[ Initiating connection to %s:%d to download '%s'... ]\n", peer_ip, peer_port, filename);
            download_content(peer_ip, peer_port, filename);
          } else {
            printf("[ Download aborted: Could not locate content on the index server. ]\n");
          }
        }
        break;
      case 4:
        printf("\n[ Action: Listing registered content... ]\n");
        break;
      case 5:
        printf("\n[ Action: De-registering content... ]\n");
        break;
      default:
        printf("\n[ Error: Invalid choice. Please enter a number 1-5 or 'exit'. ]\n");
    }
  }

  kill(server_pid, SIGTERM);
  waitpid(server_pid, NULL, 0);

  return 0;
}
