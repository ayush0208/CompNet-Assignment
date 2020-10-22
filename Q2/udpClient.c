#include "packet.h"

#define PORT_RELAY1 8001
#define PORT_RELAY2 8002
#define PACKET_SIZE 10
#define PROB 10  
#define WINDOW_SIZE 4

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

int main(void)
{
    struct sockaddr_in si_relay1, si_relay2, si_other;
    int s, i, slen=sizeof(si_relay1);
    char buf[PACKET_SIZE];
    char message[PACKET_SIZE];
    DATA_PKT recv_pkt,send_pkt;
    DATA_PKT buf_window[WINDOW_SIZE];
    fd_set readfds;
    struct timeval timeout;

    FILE* fp = fopen("input.txt","ab");
    if(fp==NULL)
    {
        printf("\n Error in opening file");
        exit(0);
    }
    fseek(fp, 0, SEEK_END);
    int filesize = ftell(fp);
    fclose(fp);

    fp = fopen("input.txt", "rb");
 
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
 
    memset((char *) &si_relay1, 0, sizeof(si_relay1));
    si_relay1.sin_family = AF_INET;
    si_relay1.sin_port = htons(PORT_RELAY1);
    si_relay1.sin_addr.s_addr = inet_addr("127.0.0.1");

    memset((char *) &si_relay2, 0, sizeof(si_relay2));
    si_relay2.sin_family = AF_INET;
    si_relay2.sin_port = htons(PORT_RELAY2);
    si_relay2.sin_addr.s_addr = inet_addr("127.0.0.2");
      
    while(1)
    {
        // printf("Enter message : ");
        // gets(message);
        int i = 0;
        int min_seq;
        while(i < WINDOW_SIZE && (send_pkt.size = fread(send_pkt.data, 1, PACKET_SIZE, fp)))
        {
            send_pkt.data[send_pkt.size] = '\0';
            
            send_pkt.sq_no = ftell(fp)- send_pkt.size;
            
            if(i==0)
                min_seq = send_pkt.sq_no;
            send_pkt.type = 0;
            if(send_pkt.sq_no + PACKET_SIZE < filesize)
            {
                send_pkt.isLast = 0;
            }
            else
            {
                
                send_pkt.isLast = 1;
            }
            pkt_copy(&send_pkt, &(buf_window[i]));
            if((send_pkt.sq_no / PACKET_SIZE) % 2 )
            {
                // ODD CASE
                
                if (sendto(s, &send_pkt, sizeof(send_pkt) , 0 , (struct sockaddr *) &si_relay1, slen)==-1)
                {
                    die("sendto()");
                }    
            }
            else
            {
                // EVEN CASE
                
                if (sendto(s, &send_pkt, sizeof(send_pkt), 0 , (struct sockaddr *) &si_relay2, slen)==-1)
                {
                    die("sendto()");
                }
            }
            
            i++;
        }
        int filled_buf = i;
        i = 0;
        while(1)
        { 
            if(i == filled_buf)
                break;
            FD_ZERO(&readfds);
            FD_SET(s, &readfds);

            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            int pkt_ack = select(s+1, &readfds, NULL, NULL, &timeout); 
            if(pkt_ack <= 0)
            {
                // Connection Timeout
                for(int j = 0 ; j < filled_buf ; j++)
                {
                    if(buf_window[j].type != 2)
                    {
                        pkt_copy(&(buf_window[j]), &send_pkt);
                        if((send_pkt.sq_no / PACKET_SIZE) % 2 )
                        {
                            // ODD CASE
                            if (sendto(s, &send_pkt, sizeof(send_pkt), 0 , (struct sockaddr *) &si_relay1, slen)==-1)
                            {
                                die("sendto()");
                            }    
                        }
                        else
                        {
                            // EVEN CASE
                            if (sendto(s, &send_pkt, sizeof(send_pkt), 0 , (struct sockaddr *) &si_relay2, slen)==-1)
                            {
                                die("sendto()");
                            }
                        }
                                
                    }
                }
            }
            else
            {
                // TODO : DIFFRENTIATE BETWEEN THE ACKS FROM BOTH THE RELAYS
                if (recvfrom(s, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr *) &si_other, &slen) == -1)
                {
                    die("recvfrom()");
                }
                buf_window[(recv_pkt.sq_no - min_seq)/PACKET_SIZE].type = 2;
                i++; 
                printf("Acknowlegement for packet %d received.\n", recv_pkt.sq_no/PACKET_SIZE);
            }
        }

        if(feof(fp))
            break;
    }
 
    close(s);
    return 0;
}
