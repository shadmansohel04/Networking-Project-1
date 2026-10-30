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

/* Include the header files for our features */
#include "registration.h"
#include "getContent.h"

#define BUFLEN 256

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

    switch(choice) {
      case 1:
        printf("\n[ Action: Registering content... ]\n");
        register_content(host, port);
        break;
      case 2:
        printf("\n[ Action: Searching for content... ]\n");
        {
          /* Execute search but ignore the returned data for a standard search query */
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

  return 0;
}
