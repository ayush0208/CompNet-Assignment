#include "packet.h"

void die(char *s)
{
    perror(s);
    exit(1);
}

void fn_send_pkt(PKT* send_pkt,int seqNo, int bytes_read, int id)
{
    send_pkt->sq_no=seqNo;
    send_pkt->size = bytes_read;
    send_pkt->channel_id=id;
    send_pkt->data_or_ack=0;
}

int main(void)
{
    struct sockaddr_in si_server;
    int max_client_no = CHANNELS;
    int s, i;
    fd_set readfds;
    int nread;
    int Sockets[CHANNELS], States[CHANNELS], Retransmit[CHANNELS], activity;
    int global_seq_no = 0; // global sequence number
    int flag = 0;   //flag for terminating the connection
    int timeout_flag[CHANNELS] = {0};  //flag for checking if timeout has occurred in any of the channels
    int last_flag = 0;  //flag for checking if ack received is for last packet sent

    
    struct timeval timeout;

    
    PKT send_pkt[CHANNELS],rcv_pkt;

    //open the file to read from
    FILE *fp = fopen("input.txt","r");
    if(fp==NULL)
    {
        die("fopen");
        return 1;
    } 

    fseek(fp, 0, SEEK_END); // seek to end of file
    int fileSize = ftell(fp);
    
    fp = fopen("input.txt","r");
    //Initialise a socket for each channel
    for(i = 0; i < max_client_no; i++){
        if ((s=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
            die("socket");
        Sockets[i] = s;
        States[i] = 0;
        Retransmit[i] = 0;
    }
    
    //construct server address structure
    memset((char *) &si_server, 0, sizeof(si_server));
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(PORT);
	si_server.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    
    for(i = 0; i < max_client_no; i++){
        int c = connect (Sockets[i], (struct sockaddr*) &si_server , sizeof(si_server));
        if (c < 0) 
            die("connection");
    }

    while(1)
    {   
        //begin the send and recv cycle
        for(i = 0; i < max_client_no; i++)
        {  
              
            s = Sockets[i];
            int seq_no = global_seq_no;

            //2 states in the FSM
            switch(States[i])
            { 
                //State for sending packet
                case 0:
                {
                    //New Packet is being sent, i.e., no retransmission 
                    if(Retransmit[i] == 0){
                        
                        
                        memset(send_pkt[i].data, '\0', PACKET_SIZE);
                        nread = fread(send_pkt[i].data, 1, PACKET_SIZE, fp);
                        global_seq_no+=nread;

                        //construct the send packet.
                        // send_pkt[i].sq_no = seq_no; 
                        // send_pkt[i].size = nread;
                        // send_pkt[i].data_or_ack = 0;
                        // send_pkt[i].channel_id = i;

                        fn_send_pkt(&send_pkt[i],seq_no,nread,i);

                        if((fileSize == global_seq_no && nread == PACKET_SIZE)|| nread < PACKET_SIZE)
                            send_pkt[i].is_last = 1;
                        else
                            send_pkt[i].is_last = 0;
                        
                        
                        if(nread == 0){
                            break;
                        } 

                        //Send the packet to the server
                        if (send(s, &send_pkt[i], sizeof(send_pkt[i]), 0) == -1)
                            die("send()");

                        printf("SNT PKT: Seq. No. %d of bytes %d from channel %d \n",send_pkt[i].sq_no,send_pkt[i].size,send_pkt[i].channel_id);
                        //Set the state to recv for ack from server
                        States[i] = 1; 
                    }

                    //Retransmit the previously sent packet due to timeout
                    else{
                        if (send(s, &send_pkt[i], sizeof(send_pkt[i]), 0) == -1)
                            die("send()");
                        printf("Retransmitting packet\n");
                        printf("SNT PKT: Seq. No. %d of bytes %d from channel %d \n",send_pkt[i].sq_no,send_pkt[i].size,send_pkt[i].channel_id);
                        States[i] = 1;

                        //Reset the Retransmit flag to 0
                        Retransmit[i] = 0;
                        
                    }

                    break;
                }

                //State for receiving ack packets from server
                case 1:
                { 
                    
                    FD_ZERO(&readfds);
                    
                    
                    FD_SET(s, &readfds);

                    //re-initialize the timeval
                    timeout.tv_sec = TIMEOUT;
                    timeout.tv_usec = 0; 
                    
                    //wait for an activity for on the socket till timeout
                    activity = select(s + 1, &readfds, NULL, NULL, &timeout);
                    //Error while calling select()
                    if(activity < 0){
                        die("select()");
                        break;
                    }

                    //Timeout occurred
                    if(activity == 0){
                        printf("Timeout occured at Channel %d\n", i);
                        
                        //Set the state to zero for re-sending the packet
                        States[i] = 0;
                        
                        //Set the retransmit flag to 1 for re-sending the previously sent packet
                        Retransmit[i] = 1;

                        /* Set the timeout flag to 1 for the current channel so that client does not
                        end connection prematurely */
                        timeout_flag[i] = 1;
                        break;
                    }

                    //Else recv the incoming ack packet from the socket 
                    if (recv(s, &rcv_pkt, sizeof(rcv_pkt), 0) == -1)
                    {
                        die("recv()");
                        break;
                    }
                    
                    //last_flag is set to 1 if the ack for the last packet has been received
                    if(rcv_pkt.is_last==1)
                        last_flag = 1;
                    
                    //receive ack packet
                    if(rcv_pkt.channel_id == i){
                        printf("RCVD ACK: for PKT with Seq. No. %d from channel %d\n",rcv_pkt.sq_no,rcv_pkt.channel_id);
                        
                        //Set the timeout flag to 0 since ack packet has been received
                        timeout_flag[i] = 0;
                        
                        //Set state to 0 for sending the next packet
                        States[i] = 0; 
                        break;
                    }
                    else{
                        printf("Timeout occured at Channel %d\n", i);
                        
                        //Set the state to zero for re-sending the packet
                        States[i] = 0;
                        
                        //Set the retransmit flag to 1 for re-sending the previously sent packet
                        Retransmit[i] = 1;

                        
                        timeout_flag[i] = 1;
                        break;
                    }

                }
                
            }

            // If the last packet is sent and no timeouts have occurred on any channel,then terminate connection
            if(last_flag && !timeout_flag[0] && !timeout_flag[1])
            {
                flag = 1;
                break;
            }
        }

        
        if(flag)
            break;
    }

    
    for(i = 0; i < max_client_no; i++){
        s = Sockets[i];
        close(s);
    }
    return 0;
}