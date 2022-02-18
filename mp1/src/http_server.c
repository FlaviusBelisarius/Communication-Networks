/*
** Created by Wubin Tang & Jerry Nie on 9/21/20.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

#define BFSIZE 1024

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//respond client with a header
void respond_client(int client_socket, int respond_type){
    char buffer[BFSIZE];
    if(respond_type == 1){// 200 ok
        sprintf(buffer, "HTTP/1.1 200 OK\r\n\r\n");
        send(client_socket, buffer, strlen(buffer), 0);
    }else if(respond_type == 2){// 404 NOT FOUND
        sprintf(buffer, "HTTP/1.1 404 NOT FOUND\r\n\r\n");
        send(client_socket, buffer, strlen(buffer), 0);
    }else if (respond_type == 3){
        sprintf(buffer, "HTTP/1.1 400 BAD REQUEST\r\n\r\n");
        send(client_socket, buffer, strlen(buffer), 0);
    }
}

// send file function
// get file data according to the file_path, and then
// send a buffer size of data to client socket
void send_file(int client_socket, char* file_path){
    int read_in_length;
    char file_name[BFSIZE];
    bzero(file_name, BFSIZE);
    if(file_path[0] == '/'){
        strcpy(file_name, file_path+1);
    }else{
        strcpy(file_name, file_path);
    }
    FILE *fp = fopen(file_name, "rb");
    FILE *cs_fd = fdopen(client_socket, "wb");// cs_fd represents client socket's file descriptor
    if(fp == NULL){
        respond_client(client_socket, 2);
        perror("404 Not found!!!!!! ");
        exit(1);
    }else{
        respond_client(client_socket, 1);
    }
    char buffer[BFSIZE] = {0};
    bzero(buffer, BFSIZE);
    while(!feof(fp)){
        read_in_length = fread(buffer, 1 , sizeof(buffer), fp);
        fwrite(buffer, 1, read_in_length, cs_fd);
        bzero(buffer, BFSIZE);
    }
    fclose(cs_fd);
    fclose(fp);

}

// receive and process header
void handle_client_request(int client_socket, char* method, char* file_path){
    // receiving header
    int numbytes;
    char buffer[BFSIZE]; // buffer which contains the request command
    if ((numbytes = recv(client_socket, buffer, BFSIZE - 1, 0)) == -1) {//receive failed
        perror("recv");
        exit(1);
    }
    buffer[BFSIZE-1] = '\0';
    //process header
    const char * space = " ";
    method = strtok(buffer, space);
    file_path = strtok(NULL, space);
    //http_version = strtok(NULL, space);
    if(strcmp(method, "GET") != 0){
        respond_client(client_socket, 3);
        return;
    }
    send_file(client_socket, file_path);
}

int main(int argc, char *argv[]){
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    if (argc != 2) {
        fprintf(stderr,"no server port\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;//both ipv4 and ipv6 are good
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    // return 0 if succeeds
    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) { // this for loop handles failed situations
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,     // Set various options for a socket
                       sizeof(int)) == -1) {                       
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {  //print the reason of binding failure
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);            // listen failed
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask); //initialise and empty a signal set
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size); //accept connection
        // new_fd is the client socket's file discriptor
        if (new_fd == -1) {
            perror("accept");//accept failed
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);

        char method[16] = {0};
        char file_path[BFSIZE] = {0};

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener, becasue it already start to handle client request
            handle_client_request(new_fd, method, file_path);
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this and connection break
    }
}
