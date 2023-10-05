/*
 * This module providese the client side for the messaging application.
 * Componets of the service:
 * - Web Server where clients connect to and exchange messages
 * - Database storing client information and messages (Future release)
 * - Encryption of messages (Future release)
 *
 * The parallell handling of connections was inspired by the following article:
 * https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

#define NUM_USERS 10  // max number of users
#define PORT 123456
#define QUEUE_LEN 10

void send_message(int*, char* , int );
int handle_client(int *);
void parse_message(char *);
int create_socket_inet(int*);
void bind_socket(int *);
void listen_on_socket(int *, int);
int wait_and_accept(int *, int*);

int setup_simple_webserver(void){
  // general procedure
  // create socket, bind socket to Port with socket options
  int sock_fd;
  int client_sockets[NUM_USERS];
  if (create_socket_inet(&sock_fd) == -1) {
    perror("Socket creation failed");
    exit(1);
    }    
  bind_socket(&sock_fd);
  listen_on_socket(&sock_fd, QUEUE_LEN);
  for (int i = 0; i < NUM_USERS; i++) {
    // set client sockets to 0, i.e. ununsed
    client_sockets[i] = 0;
  }
  wait_and_accept(&sock_fd, client_sockets);

  return 0;
}

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

void listen_on_socket(int *sock_fd, int queue_len){
    int err_or_not;
    err_or_not = listen(*sock_fd, queue_len);

    if (err_or_not == -1){
        printf("Error binding socket.\n");
        exit(-1);
    }
}

int wait_and_accept(int *master_socket, int client_socket[NUM_USERS]){
  int new_socket, activiy;
  int max_sd, sd;
  fd_set readfds;
  socklen_t sin_size = sizeof(struct sockaddr_in);
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);
  int address_len = sizeof(address);

  while(1){
    // Each iteration we reset the fd_set and see if something changed in the client sockets

    FD_ZERO(&readfds);  // clear socket set
    FD_SET(*master_socket, &readfds);
    max_sd = *master_socket;

    // add child sockets to set
    for (int i = 0; i < NUM_USERS ; i++) {
      sd = client_socket[i];
      if (sd > 0)
        // valid descriptor
        FD_SET(sd, &readfds);

      if (sd > max_sd)
        max_sd = sd;  // highest fd number. Needed for select function
    }  // end for

    // wait for activity on one of the sockets
    activiy = select(max_sd + 1, &readfds, NULL, NULL, NULL); // timeval = NULL <-> wait indefinitely

    if (activiy < 0 && errno!=EINTR)
      printf("Error during selection of active thread.\n");

    // check if something happened on master socket, if yes, then incoming connection
    if (FD_ISSET(*master_socket, &readfds)) {
      // try to establish connection
      if ((new_socket = accept(*master_socket, (struct sockaddr *) &address, (socklen_t *) &address_len))  < 0) {
        perror("Accept.");
        exit(EXIT_FAILURE);
      }
      // connection established
      printf("Established connection, socket_fd=%d, ip=%s, port=%d\n",
             new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));


      // send welcome message
      send_message(&new_socket, "Welcome in the chat room.\r\n", strlen("Welcome in the chat room.\r\n"));


      // add new sockets to client sockets
      for (int i = 0; i < NUM_USERS ; i++) {
        if (client_socket[i] == 0) {
          client_socket[i] = new_socket;
          break;
        }
      }
    } else { // else, not master socket active due to new connection but some other socket activity
      for(int i = 0; i < NUM_USERS; i++){
	sd = client_socket[i];

	if (FD_ISSET(sd, &readfds)){
          if (handle_client(&sd) == 0) {
	    // closed connection thus free client socket
	    client_socket[i] = 0;
          } else {
            // socket still active, do nothing            
	    ;
          }
          break;          
	} // end if
      } // end for
    }
  }
}


void send_message(int *socket_fd, char buf[], int buf_len){

  if ((send(*socket_fd, buf, strlen(buf), 0)) != strlen(buf)) {
    perror("Error while sending message.");
  }
  puts("Message sent.");

}

int handle_client(int *sock_fd){
  // handle client acitivity
  char buffer[1024];
  unsigned long read_val;
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);
  int address_len = sizeof(address);

  if ((read_val = read(*sock_fd, buffer, 1024)) == 0){
    // close connection
    // write client info into address
    getpeername(*sock_fd, (struct sockaddr*)&address, (socklen_t*)&address_len);
    printf("Host disconnected , ip %s , port %d \n",
           inet_ntoa(address.sin_addr),
           ntohs(address.sin_port));   

    //Close the socket and mark as 0 in list for reuse  
    close(*sock_fd);   
    return 0;
  } else {
    // not closing socket, need to parse message
    printf("Length of message: %lu\n", read_val);
    buffer[read_val] = '\0'; // add 0 byte to end of string read
    printf("buffer is read and terminated with 0 byte.\n");
    parse_message(buffer);
    return 1;    
  }    
}

void parse_message(char *buffer) {
  // parse message
  printf("Parsing message!\n");
  printf("Message is %s: \n", buffer);
  // TODO parser for messages
  // EXit for closing connection
  // show_clients for showing all other clients and printing these
  // send_to CLIENT: A message to send a message to the CLIENT
}


int main() {
  printf("Setting up simple webserver...\n");
  setup_simple_webserver();
}  
