#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Error function used for reporting issues
void error(const char* msg) {
    perror(msg);
    exit(1);
}

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address,
    int portNumber) {

    // Clear out the address struct
    memset((char*)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);
    // Allow a client at any address to connect to this server
    address->sin_addr.s_addr = INADDR_ANY;
}

int main(int argc, char* argv[]) {
    int connectionSocket, charsRead;

    //Init buffers to hold plaintext, key, and return message
    char buffer[140000 * sizeof(char)];
    char buffer_1[70000 * sizeof(char)];
    char buffer_2[70000 * sizeof(char)];

    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);

    // Check usage & args
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    // Create the socket that will listen for connections
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        error("ERROR opening socket");
    }

    // Set up the address struct for the server socket
    setupAddressStruct(&serverAddress, atoi(argv[1]));

    // Associate the socket to the port
    if (bind(listenSocket,
        (struct sockaddr*)&serverAddress,
        sizeof(serverAddress)) < 0) {
        error("ERROR on binding");
    }

    // Start listening for connetions. Allow up to 5 connections to queue up
    listen(listenSocket, 5);

    // Accept a connection, blocking if one is not available until one connects
    while (1) {
        // Accept the connection request which creates a connection socket
        connectionSocket = accept(listenSocket,
            (struct sockaddr*)&clientAddress,
            &sizeOfClientInfo);

        if (connectionSocket < 0) {
            error("ERROR on accept");
        }
        
        //Holds the recv code unique to the dec client server relationship
        int recv_code = 0;
        int dec_code = 123;
        int send_check = 1;

        //Challenge and Password
        charsRead = recv(connectionSocket, &recv_code, sizeof(dec_code), 0);
        if (recv_code != dec_code) {
            //If recv code not equal to dec code then print error and skip servers work while continuing to listen for new connections.
            perror("DEC SERVER CAN ONLY BE ACCESSED BY ENC CLIENT\n");
            charsRead = send(connectionSocket, &send_check, (sizeof(send_check)), 0);
            close(connectionSocket);
            goto CONT_LISTEN;
        }
        //Else send a status of 0 indicating that code matches
        else {
            send_check = 0;
            charsRead = send(connectionSocket, &send_check, (sizeof(send_check)), 0);
        }

        //Ensures buffers are zeroed
        memset(buffer, '\0', sizeof(buffer));
        memset(buffer_1, '\0', sizeof(buffer_1));
        memset(buffer_2, '\0', sizeof(buffer_2));

        int file1_length = 0;
        //Recieves message length from client
        charsRead = recv(connectionSocket, &file1_length, sizeof(file1_length), 0);


        //Ensure buffer is cleared before recieving message
        memset(buffer, '\0', sizeof(buffer));

        //Init counters for recv looops
        //loop_count used in error handling
        int loop_count = 0;
        //read_count keeps track of characters read
        int read_count = 0;
        //left_to_read keeps track of how many characters remain in total file length
        int left_to_read = file1_length;

        //Init pointer to buffer
        char* buffer_cpy = buffer;
        while (read_count < file1_length) {
            charsRead = recv(connectionSocket, buffer_cpy, left_to_read, 0);
            buffer_cpy += charsRead;
            // If read_count is less than zero then that means that an error has occured and we close the program
            if (read_count < 0) {
                error("SERVER ERROR reading from socket\n");
            }
            // Else we increment the read_count and decrement the left_to_read count
            else {
                read_count += charsRead;
                left_to_read -= charsRead;
            }
            //Keeps track of loops run and exits if over a certain amount.
            //Stops the program from running into and infinite loop.
            loop_count++;
            if (loop_count > 20) {
                error("LOOP COUNT EXCEEDED\n");
                break;
            }
        }
        // Test print statement
        // printf("SERVER GOT: %s\n", buffer);
        if (read_count < 0) {
            error("ERROR reading from socket");
        }
        // test print statement
        //  printf("SERVER: SERVER EXPECTED: %d SERVER READ: %d, LEFT TO READ: %d\n", file1_length, read_count, left_to_read);

        // Iterates through buffer and moves data into buffer_1 until it hits a newline
        int y = 0;
        while (buffer[y] != '\n') {
            buffer_1[y] = buffer[y];
            y++;
        }

        y++;

        // Iterates through buffer and moves data into buffer_2 until it hits a newline
        int z = 0;
        while (buffer[z] != '\n') {
            buffer_2[z] = buffer[y];
            y++;
            z++;
        }


        // Decrpyt file

        int x;
        //Iterates through plaintext file and performs decryption
        for (x = 0; x < strlen(buffer_1)-1; x++) {
            if (buffer_1[x] == '\0') {
                goto EXIT_LOOP;
            }
            if (buffer_1[x] == 32) {
                goto SKIP;
            }
            int a = buffer_1[x] - 65;
            int b = buffer_2[x] - 65;
            int c = a - b;

            if(c < 0) {
                c = c + 26;
            }
            buffer_1[x] = 65 + c;

        SKIP:;
        }
    EXIT_LOOP:;
        //Send encrypted message to client
        charsRead = send(connectionSocket, buffer_1, strlen(buffer_1), 0);

        printf("SERVER: SENT %d\n", charsRead);

        if (charsRead < 0) {
            error("ERROR writing to socket");
        }
         sleep(1);
         // Close the connection socket for this client
        close(connectionSocket);

        memset(buffer, '\0', sizeof(buffer));
        memset(buffer_1, '\0', sizeof(buffer_1));
        memset(buffer_2, '\0', sizeof(buffer_2));
    CONT_LISTEN:;
    }
    // Close the listening socket
    close(listenSocket);
    return 0;
}