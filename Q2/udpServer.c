/*  Simple udp server */
#include "packet.h"

#define PORT 8003
#define PACKET_SIZE 10
#define PROB 10  
#define WINDOW_SIZE 4
#define DELAY 1
 
typedef struct packet{
    int size;
    int sq_no;
    int type; // 0 :- DATA  , 1 :- ACK
    int isLast; // 1 :- Last Packet
    char data[PACKET_SIZE+1];
}DATA_PKT;

void pkt_copy(DATA_PKT *pkt1, DATA_PKT *pkt2)
{
    pkt2->size = pkt1->size;
    pkt2->sq_no = pkt1->sq_no;
    pkt2->type = pkt1->type;
    pkt2->isLast = pkt1->isLast;
    strcpy(pkt2->data, pkt1->data);
}

void die(char *s)
{
    perror(s);
    exit(1);
}
 
int main(int argc , char *argv[])
{
    struct sockaddr_in si_me, si_other;
    int s, i, slen = sizeof(si_other) , recv_len;
    DATA_PKT recv_pkt, send_pkt;
    DATA_PKT buf_window[WINDOW_SIZE];   
    
    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {   
        die("socket");
    }

    FILE *fp = fopen("out.txt", "w");
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }
 
    int min_seq = 0;
    //keep listening for data
    while(1)
    {
        int isLast = 0;
        printf("Waiting for data...");
        fflush(stdout);

        int i = 0;
        int filled_buf = WINDOW_SIZE; 
        while(i < filled_buf)
        {
            if ((recv_len = recvfrom(s, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr *) &si_other, &slen)) == -1)
            {
                die("recvfrom()");
            }
            if(i==0)
                min_seq = recv_pkt.sq_no;

            printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
            

            pkt_copy(&recv_pkt, &(buf_window[(recv_pkt.sq_no - min_seq) / PACKET_SIZE]));
            i++;

            send_pkt.type = 1;
            send_pkt.sq_no = recv_pkt.sq_no;
            send_pkt.size = 0;
            send_pkt.isLast = recv_pkt.isLast;
            
            if (sendto(s, &send_pkt, sizeof(send_pkt), 0, (struct sockaddr*) &si_other, slen) == -1)
            {
                die("sendto()");
            }

            if(recv_pkt.isLast)
            {
                isLast = 1;
                filled_buf = ((recv_pkt.sq_no - min_seq) / PACKET_SIZE)+1;
                printf("%d\n", filled_buf);
            }    
        }

        for(int j = 0 ; j < filled_buf ; j++)
        {
            fwrite(buf_window[j].data, 1, buf_window[j].size, fp);
        }

        if(isLast)
            break;
    }
    fclose(fp);
    close(s);
    return 0;
}
