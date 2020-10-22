
#ifndef _packet_
#define _packet__

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>


#define PACKET_SIZE 100  //Max length of payload in packet
#define PORT 8882   
#define PDR 10 // Packet drop rate
#define TIMEOUT 2
#define CHANNELS 2

typedef struct packet
{
    int sq_no;
    int channel_id;
    int is_last;
    int size;
    int data_or_ack; // 0: data, 1:ack
    char data[PACKET_SIZE];
}PKT;

typedef struct node
{
    int seq_no;
    int size;
    char data[PACKET_SIZE];
    struct node* next;
}Buffer;

#endif