				Instructions for running
1. Open four terminals simultaneously 
2. In the first terminal compile server file using the command :unamused: gcc -o server udpServer.c
3. In the same terminal run the server executable using the command :unamused: ./server
4. Now the server is available for listening 
5. In the second terminal compile relay file using the command :unamused: gcc -o rel1 udpRelay.c
6. In the same terminal run the relay executable using the command :unamused: ./rel1 1
7. In the third terminal compile relay file using the command :unamused: gcc -o rel2 udpRelay.c
8. In the same terminal run the relay executable using the command :unamused: ./rel2 2
9. In the fourth terminal compile client file using the command :unamused: gcc -o client udpClient.c
10. In the same terminal run the client executable using the command :unamused: ./client
11. The text file "input.txt" will be uploaded from the client to the server 