
/*
 The client side of the messager.
 Basic functionalities to provide:
 0. connect to server
 1. which peers are connected.
 2. send a message to a peer
 3. close connection.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>  // contains the macros to handle file descriptor sets
#include <unistd.h>
#include <sys/select.h>

#define SERVER_PORT 123456
#define PORT 123457
#define SERVER_ADDRESS "127.0.0.1"



int create_socket_inet(int *sock_fd){
  // create socket descriptor for communication
  
  *sock_fd = socket(PF_INET,SOCK_STREAM, 0);
  if (*sock_fd == -1){
    return -1;
    printf("Error creating socket.");
    exit(-1);
  }
  return 0;
}


void connect_to_server(int *sock_fd, struct sockaddr *server_address, socklen_t address_len){
  // connect to the serve
  int err_or_not;
  if ((err_or_not = connect(*sock_fd, server_address, address_len) )!= 0)
      perror("Error connecting to server.\n");
  // else connection established
}


void bind_socket(int *sock_fd){
    int err_or_not;
    struct sockaddr_in host_addr;
    socklen_t sin_size;
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(PORT);
    host_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(host_addr.sin_zero), 0, 8);
    err_or_not = bind(*sock_fd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr));
    if (err_or_not == -1){
        printf("Error binding socket.\n");
        exit(-1);
  }
   }


void init_client_app(void){
  char user_in[1024];
  int len;
  int client_socket;
  // define server address struct
  struct sockaddr_in server_address;
  socklen_t sin_size;

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(SERVER_PORT);
  inet_aton(SERVER_ADDRESS, &server_address.sin_addr);
  memset(&(server_address.sin_zero), 0, 8);
  socklen_t server_add_len = sizeof(server_address);

  // set up client socket
  create_socket_inet(&client_socket);

  // bind client socket
  bind_socket(&client_socket);
  
  // connect to server
  connect_to_server(&client_socket, (struct sockaddr*)&server_address,  server_add_len);
  printf("Connected to server successfully.\n");
  while(1){
    // accepting user input for as long as not typed 'exit'
    printf("Message: ");
    fgets(user_in, 1024, stdin);
    printf("Read string\n");
    len = strlen(user_in);
    printf("Read user input of len: %d \n", len);
    // send message to server
    if (send(client_socket, user_in, len, 0) != len){
      perror("Message has not been sent completely.");
    }
  }
  }

int main(void){
  printf("Main running.\n");
  printf("Connecting to server at %s.\n", SERVER_ADDRESS);
  init_client_app();
}
