  
/*
** Created by Wubin Tang & Jerry Nie on 9/21/20.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to

#define MAXDATASIZE 100 // max number of bytes we can get at once

#define BFSIZE 1024

// function of receive a file from socket
void receive_file(int socketfd){
    int n;
    FILE *fp;
    char buffer[BFSIZE] = {0};
    char* OK_respond = "HTTP/1.1 200 OK\r\n\r\n";

    fp = fopen("output", "wb");
    //cs_fd = fdopen(socketfd, "rb");
    if(fp == NULL){
        perror("cannot create file");
        exit(1);
    }
    // check respond
    bzero(buffer, BFSIZE);
    n = recv(socketfd, buffer, BFSIZE-1, 0);
    buffer[n] = '\0';
    //n = fread(buffer, 1, BFSIZE, cs_fd);
    if(n <= 0){
        perror("recv");
        exit(1);
    }
    if (strncmp(buffer, OK_respond, strlen(OK_respond)) != 0) {
        perror("respond not OK");
        exit(1);
    }

    for(int i = strlen(OK_respond); i < BFSIZE; i++){
        if(buffer[i] == '\0'){
            break;
        }
        fprintf(fp, "%c", buffer[i]);
        //fwrite(buffer, sizeof(char), BFSIZE, fp);
    }
    FILE* cs_fd = fdopen(socketfd, "rb");
    bzero(buffer, BFSIZE);
    while(!feof(cs_fd)){
        n = fread(buffer, 1, BFSIZE, cs_fd);
        fwrite(buffer, 1, n, fp);
        bzero(buffer, BFSIZE);
    }
    fclose(fp);
    return;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]){
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    char hostname[BFSIZE];
    char port[6];
    char file_path[BFSIZE];
    char input[strlen(argv[1])];
    //int numbytes;// data length we received
    //initialization
    memset(hostname, 0, BFSIZE);
    memset(port, 0, 6);
    memset(file_path, 0, BFSIZE);

    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }
    //get respond status
    if (strncmp(argv[1], "http://", 7) == 0){
        strcpy(input, argv[1]+7);
    }else{
        strcpy(input, argv[1]);
    }
    
    int len = 0;
    char *cur_pos_1;
    char *cur_pos_2;
    if((cur_pos_1 = strchr(input, ':')) != NULL){
        len = cur_pos_1 - input;
        strncpy(hostname, input, len);
        len = 0;
        if((cur_pos_2 = strchr(input, '/')) != NULL){// get port number
            len = cur_pos_2 - cur_pos_1;
            if(len <= 1){
                strcpy(port, "80");
            }else{
                strncpy(port, cur_pos_1 + 1, len - 1);
            }
            strcpy(file_path, cur_pos_2);
        }
    }else{// if port is not mentioned in header
        strcpy(port, "80");
        cur_pos_2 = strchr(input, '/');
        len = cur_pos_2 - input;
        strncpy(hostname, input, len);
        strcpy(file_path, cur_pos_2);
    }
    if(strlen(port) == 0){
        strcpy(port, "80");
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure
    //create and send header:
    char header_buff[BFSIZE];
    sprintf(header_buff, "GET %s HTTP/1.1\r\nUser-Agent: Wget/1.12(linux-gnu)\r\nHost: %s:%s\r\nConnection: Keep-Alive\r\n\r\n", file_path, hostname, port);
    send(sockfd, header_buff, strlen(header_buff), 0);
    receive_file(sockfd);

    close(sockfd);
}
