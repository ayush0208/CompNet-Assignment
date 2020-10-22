							FILE TRANSFER USING MULTI-CHANNEL STOP-AND-WAIT PROTOCOL

A modified version of stop-and-wait protocol has been implemented as per the question requirements. The client uploads a text file to the server over two different channels (TCP connections). select() and fd_set() have been used to handle multiple connections. To handle the problem of multiple timers, a select() was used on each socket separately for timeout. Transmission over two simultaneous connections was managed by using different sockets for each connection.The select command waits till data is available on a particular connection and as soon as a connection is ready to send data it selects that connection. A loop through each socket checks if it is ready using select() command. If the socket is not ready till timeout seconds, packet is retransmitted through the same socket.
A text file "input.txt" containing data is to be uploaded from client to server. After executing the steps below, all the data will be uploaded into a new file "output.txt" under the same directory.

					INSTRUCTIONS TO RUN?

1. Compile the server file first using 
	gcc -o server server.c
2. Next compile the client in a different terminal using:
	gcc -o client client.c
3. The text file "input.txt" will be uploaded from the client to the server and the entire data will be stored in a new file "ouput.txt" 


