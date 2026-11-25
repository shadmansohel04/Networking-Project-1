#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include "pdu.h"
#include "registration.h"

int search_content(const char *server_ip, int server_port, const char *my_peer_name, char *out_ip, int *out_port, char *out_filename) {
  int udp_sock;
  struct sockaddr_in index_addr;
  char content_name[11];
  char input_buf[256];
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

  printf("Enter Content Name to search (max 10 chars): ");
  if (fgets(input_buf, sizeof(input_buf), stdin) != NULL) {
    input_buf[strcspn(input_buf, "\n")] = '\0';
    strncpy(content_name, input_buf, 10);
    content_name[10] = '\0';
  }

  spdu.type = 'S';
  bzero(spdu.data, MAX_DATA_LEN);
  snprintf(spdu.data, MAX_DATA_LEN, "%-10s%-10s", my_peer_name, content_name);

  sendto(udp_sock, &spdu, sizeof(PDU), 0, (struct sockaddr *)&index_addr, sizeof(index_addr));

  index_len = sizeof(index_addr);
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
  int success = 0;

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
    else if (rpdu.type == 'C') {
      FILE *fp = fopen(filename, "wb");
      if (fp != NULL) {
        fwrite(rpdu.data, 1, n - 1, fp);
        
        while ((n = read(sd, &rpdu, sizeof(PDU))) > 0) {
          fwrite(rpdu.data, 1, n - 1, fp);
        }
        
        fclose(fp);
        printf("\n[ Success: File downloaded successfully to '%s'. ]\n", filename);
        success = 1;
      } else {
        perror("Failed to open local file for writing");
      }
    }
  }

  close(sd);
  return success;
}

void handle_download_request(int new_sd) {
  PDU rpdu, spdu;
  int n = read(new_sd, &rpdu, sizeof(PDU));
  
  if (n > 0 && rpdu.type == 'D') {
    FILE *fp;
    printf("\n[ Peer Server: Received request for file '%s' ]\n", rpdu.data);
    
    fp = fopen(rpdu.data, "rb");
    bzero(&spdu, sizeof(PDU));
    
    if (fp == NULL) {
      spdu.type = 'E';
      strcpy(spdu.data, "ERRMSG: File not found on this peer.");
      write(new_sd, &spdu, sizeof(PDU));
      printf("[ Peer Server: File not found. Error sent to peer. ]\n");
    } else {
      int bytes_read;
      spdu.type = 'C';
      
      while ((bytes_read = fread(spdu.data, 1, MAX_DATA_LEN - 1, fp)) > 0) {
        write(new_sd, &spdu, bytes_read + 1);
        bzero(spdu.data, MAX_DATA_LEN); 
      }
      fclose(fp);
      printf("[ Peer Server: File transfer complete. ]\n");
    }
  }
  close(new_sd);
}

int main(int argc, char **argv) {
  char host[BUFLEN];
  int index_port;
  char my_peer_name[11];
  char input_buffer[BUFLEN];

  char local_files[MAX_LOCAL_FILES][11];
  int local_count = 0;

  int tcp_sock;
  struct sockaddr_in reg_addr;
  socklen_t alen = sizeof(struct sockaddr_in);
  int my_tcp_port;

  fd_set rfds, afds;
  int print_menu = 1;
  
  const char *menu_text = 
    "\n--- P2P Application Menu ---\n"
    "1. Register Content (R-type)\n"
    "2. Search Content (S-type)\n"
    "3. Download Content (D-type)\n"
    "4. List On-Line Registered Content (O-type)\n"
    "5. De-Register Content (T-type)\n"
    "Type 'exit' to quit\n"
    "\nEnter your choice: ";

  if ((tcp_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Cannot create TCP socket");
    exit(1);
  }

  bzero((char *)&reg_addr, sizeof(struct sockaddr_in));
  reg_addr.sin_family = AF_INET;
  reg_addr.sin_port = htons(0);
  reg_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(tcp_sock, (struct sockaddr *)&reg_addr, sizeof(reg_addr)) == -1) {
    perror("Cannot bind TCP socket");
    exit(1);
  }
  listen(tcp_sock, 5);

  getsockname(tcp_sock, (struct sockaddr *)&reg_addr, &alen);
  my_tcp_port = ntohs(reg_addr.sin_port);

  printf("[ Started Local Peer Server on TCP port %d ]\n\n", my_tcp_port);

  while (1) {
    printf("Enter your Peer Name (max 10 chars): ");
    if (fgets(input_buffer, BUFLEN, stdin) != NULL) {
      input_buffer[strcspn(input_buffer, "\n")] = '\0';
      if (strlen(input_buffer) > 0 && strlen(input_buffer) <= 10) {
        strcpy(my_peer_name, input_buffer);
        break;
      }
      printf("[ Error: Peer Name must be 1-10 characters. ]\n");
    }
  }
  
  printf("Enter the IP address of the index server: ");
  if (fgets(host, BUFLEN, stdin) != NULL) {
    host[strcspn(host, "\n")] = '\0';
  }

  printf("Enter the port of the index server: ");
  if (fgets(input_buffer, BUFLEN, stdin) != NULL) {
    index_port = atoi(input_buffer);
  }

  FD_ZERO(&afds);
  FD_SET(0, &afds);
  FD_SET(tcp_sock, &afds);

  signal(SIGCHLD, SIG_IGN); 

  while(1) {
    if (print_menu) {
      printf("%s", menu_text);
      fflush(stdout);
      print_menu = 0;
    }

    rfds = afds;

    if (select(FD_SETSIZE, &rfds, NULL, NULL, NULL) < 0) {
      perror("Select error");
      break;
    }

    if (FD_ISSET(tcp_sock, &rfds)) {
      struct sockaddr_in client;
      socklen_t client_len = sizeof(client);
      int new_sd = accept(tcp_sock, (struct sockaddr *)&client, &client_len);
      
      if (new_sd >= 0) {
        if (fork() == 0) {
          close(tcp_sock);
          handle_download_request(new_sd);
          exit(0);
        }
        close(new_sd);
      }
    }

    if (FD_ISSET(0, &rfds)) {
      int choice;
      if (fgets(input_buffer, BUFLEN, stdin) == NULL) {
        break;
      }

      input_buffer[strcspn(input_buffer, "\n")] = '\0';

      if (strcmp(input_buffer, "exit") == 0) {
        deregister_all_on_quit(host, index_port, my_peer_name, local_files, &local_count);
        printf("Exiting application. Goodbye!\n");
        break;
      }

      choice = atoi(input_buffer);

      switch(choice) {
        case 1:
          printf("\n[ Action: Registering content... ]\n");
          register_content(host, index_port, my_peer_name, my_tcp_port, local_files, &local_count);
          break;
        case 2:
          printf("\n[ Action: Searching for content... ]\n");
          {
            char dummy_ip[21];
            int dummy_port;
            char dummy_file[11];
            search_content(host, index_port, my_peer_name, dummy_ip, &dummy_port, dummy_file);
          }
          break;
        case 3:
          printf("\n[ Action: Downloading content... ]\n");
          {
            char peer_ip[21];
            int peer_port;
            char filename[11];
            
            if (search_content(host, index_port, my_peer_name, peer_ip, &peer_port, filename) == 1) {
              printf("[ Initiating connection to %s:%d to download '%s'... ]\n", peer_ip, peer_port, filename);
              if (download_content(peer_ip, peer_port, filename) == 1) {
                if (send_registration_pdu(host, index_port, my_peer_name, filename, my_tcp_port)) {
                  add_local_file(local_files, &local_count, filename);
                  printf("[ Auto-Registered: This peer is now serving '%s'. ]\n", filename);
                }
              }
            } else {
              printf("[ Download aborted: Could not locate content on the index server. ]\n");
            }
          }
          break;
        case 4:
          printf("\n[ Action: Listing registered content... ]\n");
          list_online_content(host, index_port);
          break;
        case 5:
          printf("\n[ Action: De-registering content... ]\n");
          deregister_content(host, index_port, my_peer_name, local_files, &local_count);
          break;
        default:
          printf("\n[ Error: Invalid choice. Please enter a number 1-5 or 'exit'. ]\n");
      }
      
      print_menu = 1;
    }
  }

  close(tcp_sock);
  return 0;
}
