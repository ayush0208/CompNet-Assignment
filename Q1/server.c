
#include "packet.h"

void die(char *s)
{
    perror(s);
    exit(1);
}
 


Buffer* add_buffer(int seq_no,int size,char* data)
{
    Buffer* temp = (Buffer*) malloc(sizeof(Buffer));
    temp->seq_no = seq_no;
    strcpy(temp->data,data);
    temp->size = size;    
    temp->next = NULL;
    return temp;
}

void make_send_pkt(PKT* send_pkt, PKT* packet)
{
    send_pkt->sq_no = packet->sq_no;
    send_pkt->channel_id = packet->channel_id;
    send_pkt->data_or_ack=1;
}


int main() {
    // declare variables
    int server_socket, maximum_client_num = CHANNELS;
    int clientSockets[CHANNELS];
    struct sockaddr_in si_server, si_client;
    fd_set readfds;
    int max_sd, socket_descriptor;
    int activity;
    int expected_seq_no = 0;
    int flag = 0;
    int last_flag=0;
    Buffer* head = NULL;

    
    srand(time(0));

    // initialize all clientSockets to 0 so they are not checked
    for(int i = 0; i < maximum_client_num; i++)
    {
        clientSockets[i] = 0;
    }

    // create a TCP master socket
    server_socket = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(server_socket < 0) {
        die("socket()");
    }

    // contructing structure of server address 
    memset(&si_server,0,sizeof(si_server));
    si_server.sin_family = AF_INET;  
    si_server.sin_port = htons(PORT);
    si_server.sin_addr.s_addr = htonl(INADDR_ANY); 


      
    int temp = bind(server_socket, (struct sockaddr*)&si_server, sizeof(si_server));
    if(temp < 0) 
    {
        die("bind()");
    }

    
    int temp1 = listen(server_socket, 3);
    if(temp1 < 0)
    {
        die("listen()");
    }

    //open a new file for write
    FILE *fp = fopen("output_file.txt","w");
    if(fp==NULL)
    {
        die("fopen");
        return 1;
    }
    
    while(1) {
        // clear the socket set
        FD_ZERO(&readfds);

        // add server scoket to set
        FD_SET(server_socket, &readfds);
        max_sd = server_socket;

        // add child sockets to set
        for(int i = 0; i < maximum_client_num; i++) {
            // socket descriptor
            socket_descriptor = clientSockets[i];  

            // if valid scoket descriptor then add to read list
            if(socket_descriptor > 0) {
                FD_SET(socket_descriptor, &readfds); 
            }

            // highest file descriptor number, need it for the select function
            if(socket_descriptor > max_sd) {
                max_sd = socket_descriptor;
            }
        }

        // wait for an activity on one of the sockets, timeout is NULL
        // so wait indefinitely
        activity = select(max_sd+1, &readfds, NULL, NULL, NULL);

        if(activity < 0) {
            printf("Select error\n");
        }

        // if something happened on the master socket
        // then its an incoming connection 
        if(FD_ISSET(server_socket, &readfds)) {
            int addSocket = accept(server_socket, (struct sockaddr*)NULL, NULL);
            if(addSocket < 0) {
                die("accept()");
            }          

            // add new socket to array of sockets
            for(int i = 0; i < maximum_client_num; i++) {
                // if position is empty
                if(clientSockets[i] == 0) {
                    clientSockets[i] = addSocket;
                    break;
                } 
            }       
        }

        // else its some IO operation on some other socket
        for(int i = 0; i < maximum_client_num; i++) {
            socket_descriptor = clientSockets[i];
            if(FD_ISSET(socket_descriptor, &readfds)) {

                PKT rcv_pkt, send_pkt;
                
                int recvSize = recv(clientSockets[i],&rcv_pkt,sizeof(PKT),0);
                if(recvSize < 0) {
                    die("recv()");
                }
                
                // code for packet drop
                int random = rand()%100;
                if(random < PDR || expected_seq_no > rcv_pkt.sq_no)
                {   
                    continue;
                }
                    
                //if packet received is in order
                if(rcv_pkt.sq_no == expected_seq_no) {
                    
                    //wrtie to file directly
                    int nwrite = fwrite(rcv_pkt.data, 1, rcv_pkt.size, fp);
                    if(nwrite < 0)
                        die("fwrite()");
                    else
                    {
                        printf("RCVD PKT: Seq. No. %d of bytes %d from channel %d\n",rcv_pkt.sq_no,rcv_pkt.size,rcv_pkt.channel_id);
                    }
                    
                    //construct the ack packet to be sent
                    expected_seq_no += rcv_pkt.size;
                    // send_pkt.sq_no = rcv_pkt.sq_no;
                    // send_pkt.channel_id=rcv_pkt.channel_id;
                    // send_pkt.data_or_ack = 1;
                    make_send_pkt(&send_pkt,&rcv_pkt);

                    //set last_flag to 1 for breaking out of loop
                    if(rcv_pkt.is_last == 1)
                    {
                        send_pkt.is_last = 1;
                        last_flag=1;
                    }
                    else
                        send_pkt.is_last = 0;

                    //send the ack packet
                    int sendSize = send(clientSockets[i],&send_pkt,sizeof(PKT),0);
                    if(sendSize < 0) {
                        die("send()");
                    }
                    else
                        printf("SNT ACK: for PKT with Seq. No. %d from channel %d\n",send_pkt.sq_no,send_pkt.channel_id);
                    
                    //Now check if buffer has stored any more packets after the current packet
                    if(head != NULL){
                        Buffer* curr = head;
                        Buffer* prev;
                        while(curr != NULL){

                            //if the required sequence number matches the packet sequence number, write to file
                            if(expected_seq_no == curr->seq_no){
                                if(fwrite(curr->data, 1, curr->size, fp) < 0)
                                    die("fwrite()");
                                expected_seq_no += curr->size;
                                prev = curr;
                                curr = curr->next;

                                //free the content that has been written to file
                                free(prev);
                            }
                        }
                        head = curr;
                    }
                }

                //Received out of order packet
                else { 
                                
                    //construct the ack packet to be sent
                    // send_pkt.sq_no = rcv_pkt.sq_no;
                    // send_pkt.data_or_ack = 1;
                    // send_pkt.channel_id = rcv_pkt.channel_id;
                    printf("RCVD PKT: Seq. No. %d of bytes %d from channel %d\n",rcv_pkt.sq_no,rcv_pkt.size,rcv_pkt.channel_id);
                     make_send_pkt(&send_pkt,&rcv_pkt);

                    if(rcv_pkt.is_last == 1)
                    {
                        send_pkt.is_last = 1;
                        last_flag=1;
                    }
                    else
                        send_pkt.is_last = 0;
                    
                    //send the ack packet
                    int sendSize = send(clientSockets[i],&send_pkt,sizeof(PKT),0);
                    if(sendSize < 0) {
                        die("send()");
                    }
                    else
                        printf("SNT ACK: for PKT with Seq. No. %d from channel %d\n",send_pkt.sq_no,send_pkt.channel_id);
                    
                    //store the packet received in buffer
                    if(head != NULL){
                        Buffer* curr = head;
                        Buffer* prev = NULL;
                        while(curr){
                            if(curr->seq_no > rcv_pkt.sq_no)
                                break;
                            prev = curr;
                            curr = curr->next;
                        }
                        if(prev){
                            prev->next = add_buffer(rcv_pkt.sq_no,rcv_pkt.size,rcv_pkt.data);
                            prev->next->next = curr;
                        }
                        else{
                            head = add_buffer(rcv_pkt.sq_no,rcv_pkt.size, rcv_pkt.data );
                            head->next = curr;
                        }
                    }
                    else{
                        head = add_buffer(rcv_pkt.sq_no,rcv_pkt.size,rcv_pkt.data);
                    }               
                }

                //if the last packet has been received and the buffer is empty, terminate connection
                if(last_flag== 1 && head == NULL){
                    flag = 1;
                    break;
                }
            }
        } 
        //Terminate connection
        if(flag == 1)
            break;
    }

    return 0;
}