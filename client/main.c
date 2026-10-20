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

#define BUFLEN 256

#include "registration.h" 

int main(int argc, char **argv) {
  char host[BUFLEN];
  int port;
  char input_buffer[BUFLEN];

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
    
    /* Variables for case 3 */
    char peer_ip[BUFLEN];
    char port_str[BUFLEN];
    int peer_port;

    switch(choice) {
      case 1:
        printf("\n[ Action: Registering content... ]\n");
        register_content(host, port);
        break;
      case 2:
        printf("\n[ Action: Searching for content... ]\n");
        break;
      case 3:
        printf("\n[ Action: Downloading content... ]\n");
        printf("Enter the IP address of the Content Server: ");
        if (fgets(peer_ip, BUFLEN, stdin) != NULL) {
          peer_ip[strcspn(peer_ip, "\n")] = '\0';
        }

        printf("Enter the port of the Content Server: ");
        if (fgets(port_str, BUFLEN, stdin) != NULL) {
          peer_port = atoi(port_str);
        }
        
        download_file(peer_ip, peer_port);
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

  return 0;
}