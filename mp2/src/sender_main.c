/*
 * File:   sender_main.c
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
#include <string.h>
#include <sys/time.h>

#define BFSIZE 1024
#define SBS 256    // sbs for "sender buffer size"
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))


enum sock_state{
    SLOW_START, CONGESTION_AVOIDANCE, FAST_RECOVERY
};

typedef struct{
    int type; // DATA 1 FIN 2 ACK 3
    int data_size;
    int seq_num;
    int ack;
    char data[BFSIZE];
}packet_t;

struct sockaddr_in si_other;

int s;
socklen_t slen;

double cwnd = 1;   // congestion window size
int numbytes, sockfd;
int ssthresh = 64;
int dupACKcount = 0;
int send_base = 0; // start position of cw
int send_tail = 0; //
int time_out_interval = 100;
int update_pointer = 0;
int cur_seq_num = 0;

int EOFFLAG = 0;
unsigned long long int bytes_to_transfer = 0, pkts_have_recv = 0, num_pkts = 0;
enum sock_state cur_state = SLOW_START;
char buffer[sizeof(packet_t)];

void diep(char *s) {
    perror(s);
    exit(1);
}

// Generate a packet that will be put into sender buffer later.
packet_t generatePacket(FILE * fp){
    char packet_buffer[BFSIZE + 1];
    int n = 0, trans_size = 0;
    packet_t pkt;
    trans_size = (int) min((unsigned long long int) bytes_to_transfer, (unsigned long long int) BFSIZE);
    memset(packet_buffer, 0, BFSIZE+1);
    n = fread(packet_buffer, sizeof(char), trans_size, fp);

    if(n < 0){
        diep("Error: Data size wrong");
    }
    if(n == 0){
        printf("reach EOF\n");
        EOFFLAG = 1;
    }

    pkt.seq_num = cur_seq_num;
    pkt.data_size = n;
    pkt.type = 1;
    memcpy(pkt.data, &packet_buffer, sizeof(char) * trans_size);
    bytes_to_transfer = bytes_to_transfer - n;
    //printf("the packer we generate: %d\n", pkt.seq_num);
    return pkt;
}


// Use new packet to cover the old packet in sender buffer.
void updateSenderBuffer(FILE* fp, packet_t* sender_buffer){
    if(send_base < update_pointer){
        while(update_pointer != send_base){
            if(EOFFLAG == 1){
                return;
            }
            packet_t pkt = generatePacket(fp);
            cur_seq_num = cur_seq_num + 1;
            //sender_buffer[update_pointer] = pkt; // Does this line work?
            memcpy(&sender_buffer[update_pointer], &pkt, sizeof(packet_t));
            update_pointer++;
            if(update_pointer == SBS){ 
                // special case：if send base reaches SBS it will be 0 immediately
                update_pointer = 0;
            }
        }
    }
    while(update_pointer < send_base){
        if(EOFFLAG == 1){
            return;
        }
        packet_t pkt = generatePacket(fp);
        cur_seq_num = cur_seq_num + 1;
        memcpy(&sender_buffer[update_pointer], &pkt, sizeof(packet_t));
        update_pointer++;
    }
}

int send_pkts(int sockfd, packet_t *send_buffer){
    if(pkts_have_recv == num_pkts){
        //printf("already receive all\n");
        return 0;
    }
    int last_pkt_index = send_base + (int) cwnd;
    if(send_tail < send_base){
        send_tail += SBS;
    }
    //printf("sp position 1\n");
    if((long long int)(num_pkts - pkts_have_recv) < last_pkt_index - send_tail){
        // send_tail is the old send tail
        // last_pkt_index refers to the last packet is going to be sent
        last_pkt_index = send_tail - pkts_have_recv + num_pkts;
    }
    if(last_pkt_index > send_tail){
        for(int idx = send_tail; idx < last_pkt_index; idx++){
            int sender_index = idx % SBS;
            memset(buffer, 0, BFSIZE);
            memcpy(buffer, &send_buffer[sender_index], sizeof(packet_t));
            if ((numbytes = sendto(sockfd, buffer, sizeof(packet_t), 0,
                                   (struct sockaddr *) &si_other, sizeof si_other)) == -1) {
                diep("ERROR: fail to sendto when call send_pkts ");
            }
        }
        //printf("sp position 2\n");
    }
    // printf("------------------");
    // printf("LPI is: %d", last_pkt_index);
    // printf("------------------\n");
    send_tail = max(last_pkt_index, send_tail) % SBS;
    return 0;
}

// state gram changing function, our logic is a combination of TCP and GBN method
int state_gram(packet_t pkt, int sockfd, packet_t *sender_buffer){
    int ack_num = pkt.ack % SBS;
    if(cur_state == SLOW_START){
        if(ack_num == send_base){
            dupACKcount++;
            if(dupACKcount == 3){
                send_tail = send_base;
                cur_state = FAST_RECOVERY;
                ssthresh = (int)cwnd / 2;
                cwnd = ssthresh + 3 * 1.0;
            }
        }else if(ack_num > send_base || (ack_num <= send_tail && send_tail < send_base)){
            pkts_have_recv = pkt.ack;
            send_base = ack_num;
            if(cwnd < SBS-1){
                cwnd++;
            }
            dupACKcount = 0;
            if(cwnd >= ssthresh){
                cur_state = CONGESTION_AVOIDANCE;
            }
        }
    } else if(cur_state == CONGESTION_AVOIDANCE){
        if(ack_num == send_base){
            dupACKcount++;
            if(dupACKcount == 3){
                send_tail = send_base;
                cur_state = FAST_RECOVERY;
                ssthresh = (int)cwnd / 2;
                cwnd = ssthresh + 3 * 1.0;
            }
        }else if(ack_num > send_base || (ack_num <= send_tail && send_tail < send_base)){
            //pkts_have_recv += ack_num - send_base;
            pkts_have_recv = pkt.ack;
            send_base = ack_num;
            if(cwnd < SBS-1){
                cwnd = cwnd + 1.0 / cwnd;
            }
            dupACKcount = 0;
        }
    } else if(cur_state == FAST_RECOVERY){
        if(ack_num == send_base){
            if(cwnd < SBS-1){
                cwnd++;
            }
        }else if(ack_num > send_base || (ack_num <= send_tail && send_tail < send_base)){
            //pkts_have_recv += ack_num - send_base;
            pkts_have_recv = pkt.ack;
            send_base = ack_num;
            cwnd = ssthresh;
            dupACKcount = 0;
            cur_state = CONGESTION_AVOIDANCE;
        }
    }
    return 0;
}

int timeout_state(int sockfd, packet_t *sender_buffer){
    memcpy(buffer, &sender_buffer[send_base],sizeof(packet_t));
    numbytes = sendto(sockfd, buffer, sizeof(packet_t), 0, (struct sockaddr *) &si_other, slen);
    if(numbytes == -1){
        diep("ERROR: Fail to time out sending");
    }
    cur_state = SLOW_START;
    ssthresh = cwnd / 2;
    cwnd = 1.0;
    dupACKcount = 0;
    return 0;
}


void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    //Open the file
    bytes_to_transfer = bytesToTransfer;
    //number of packets we should sent
    num_pkts = (unsigned long long int) ((bytesToTransfer-1) * 1.0 / BFSIZE + 1);
    //printf("number of packets we received: %llu\n", num_pkts);
    packet_t sender_buffer[SBS];

    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }

    //read data from file and store it into packet_buffer
    int count = 0;
    packet_t pkt;
    //printf("position 1\n");
    // initialize sender buffer, first SBS packets
    for(count = 0; count < SBS && bytes_to_transfer != 0; count++) {
        pkt = generatePacket(fp);
        memcpy(&sender_buffer[count], &pkt, sizeof(packet_t));
        cur_seq_num = cur_seq_num + 1;
    }
    if(count < 0){
        diep("ERROR: Error in creating sender_buffer");
    }

    /* Determine how many bytes to transfer */

    slen = sizeof (si_other);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    //printf("position 3\n");
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = time_out_interval * 1000;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        diep("Error: setsockopt");
    }
    send_pkts(s, sender_buffer);
    //printf("position 4\n");
    while(pkts_have_recv < num_pkts){
        numbytes = recvfrom(s, buffer, sizeof(packet_t), 0, (struct sockaddr *) &si_other, &slen);
        if(numbytes == -1){ //Time out
            int ts = timeout_state(s, sender_buffer);
            if(EOFFLAG == 0){
                updateSenderBuffer(fp, sender_buffer);
            }
            send_tail = send_base + (int)cwnd;
            if(ts != 0){
                diep("ERROR: Fail timeout_state");
            }
        }
        packet_t recv_pkt;
        memcpy(&recv_pkt, buffer, sizeof(packet_t));
        if(recv_pkt.type == 3){
            //printf("received ack: %d\n", recv_pkt.ack);
            state_gram(recv_pkt, s, sender_buffer);
            if(EOFFLAG == 0){
                updateSenderBuffer(fp, sender_buffer);
            }
            //printf("now send!\n");
            send_pkts(s,sender_buffer);
            /* debug code here */
            // printf("position 5\n");
            // printf("==============================\n");
            // printf("window size: %lf\n", cwnd);
            // printf("send base is: %d\n", send_base);
            // printf("send tail is: %d\n", send_tail);
            // printf("curstate is: %d\n", cur_state);
            // printf("tsh is: %d\n", ssthresh);
            // printf("up is: %d\n", update_pointer);
            // printf("==============================\n");
            continue;
        }
    }
    int n = 0;
    fclose(fp);
    //printf("position 6\n");
    packet_t finpkt;
    finpkt.type = 2;
    finpkt.seq_num = -1;
    finpkt.data_size = 0;
    finpkt.ack = -1;
    memcpy(buffer, &finpkt, sizeof(packet_t)); 
    if((n = sendto(s, buffer, sizeof(packet_t), 0, (struct sockaddr *) &si_other, slen))== -1){
        diep("send fin failed");
        exit(1);
    }
    /* Send data and receive acknowledgements on s*/
    printf("Closing the socket\n");
    close(s);
    return;
}

/*
 *
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);



    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);


    return (EXIT_SUCCESS);
}
