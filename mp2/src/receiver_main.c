/*
 * File:   receiver_main.c
 * Author: Jerry Nie & Wubin Tang
 *
 * Created on Oct.04.2020
 * Finished on 0ct.22.2020
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/stat.h>
#include <signal.h>
#include <sys/time.h>

#define BFSIZE 1024

typedef struct packet{
    int type; // DATA 1 FIN 2 ACK 3
    int data_size;
    int seq_num;
    int ack;
    char data[BFSIZE];
}packet_t;


struct sockaddr_in si_me, si_other;
int s;
socklen_t slen;
int cur_seq_num = 0;

char ackbuf[sizeof(packet_t)];

void diep(char *s) {
    perror(s);
    exit(1);
}

// reply an ACK to transmitter
void sendACK(int cur_seq_num, int type){
    packet_t ackpkt;
    ackpkt.type = type;
    ackpkt.seq_num = -1;
    ackpkt.ack = cur_seq_num;
    ackpkt.data_size = 0;
    memcpy(ackbuf, &ackpkt, sizeof(packet_t));
    int n = 0;
    if((n = sendto(s, ackbuf, sizeof(packet_t), 0, (struct sockaddr *) &si_other, slen))== -1){
        diep("send ack/fin failed");
        exit(1);
    }
}

void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {
    char buffer[sizeof(packet_t)]; // buffer to contain the packet we recevied
    slen = sizeof (si_other); // length of address info

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");

    /* Now receive data and send acknowledgements */
    //preparation process
    FILE* fp = fopen(destinationFile,"wb");
    if(fp == NULL){
        diep("file open");
        return;
    }

    int n = 0; // number of bytes received
    while(1){
        //local packet, check packet attributes
        if ((n = recvfrom(s, buffer, sizeof(packet_t), 0, (struct sockaddr *) &si_other, &slen)) == -1) {
            diep("Receive failure!");
        }
        packet_t pkt;
        memcpy(&pkt, buffer, sizeof(packet_t)); //initialize the packet we received
        //check FIN type
        if(pkt.type == 2){
            sendACK(cur_seq_num + 1, 2);
            break; // data transmisstion finished, jump out the loop
        }
        // check seq_num
        printf("the apcket we received: %d\n", pkt.seq_num);
        printf("cur_seq_num is: %d\n", cur_seq_num);
        if(pkt.seq_num == cur_seq_num){
            fwrite(pkt.data, sizeof(char), pkt.data_size, fp); // write data into file
            sendACK(cur_seq_num + 1, 3);
            cur_seq_num++;
        }else if(pkt.seq_num < cur_seq_num){ //duplicate packet receved
            sendACK(cur_seq_num, 3);
        }else{ // ahead packets received
            sendACK(cur_seq_num,3);
        }
    }
    close(s);
    printf("%s received.", destinationFile);
    return;
}

/*
 *
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}
