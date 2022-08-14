//Jacob Schiemenz
//Assignment 5 One Time Pads
//Due Date: 3-8-2021

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()
#include <ctype.h>
#include <errno.h>

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// Error function used for reporting issues
void error(const char* msg) {
    perror(msg);
    exit(0);
}

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address,
    int portNumber,
    char* hostname) {

    // Clear out the address struct
    memset((char*)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);

    // Get the DNS entry for this host name
    struct hostent* hostInfo = gethostbyname("localhost");
    if (hostInfo == NULL) {
        fprintf(stderr, "CLIENT: ERROR, no such host\n");
        exit(0);
    }
    // Copy the first IP address from the DNS entry to sin_addr.s_addr
    memcpy((char*)&address->sin_addr.s_addr,
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

int main(int argc, char* argv[]) {
    int count;
    count = argc;
    int socketFD, portNumber, charsWritten, charsRead;
    struct sockaddr_in serverAddress;

    //Holds the initial plaintext file
    char buffer_0[71000] = { 0 };
    char buffer_1[71000] = { 0 };
    char buffer_2[142000] = { 0 };

    // Open plaintext file and get data
    FILE* fp = fopen(argv[1], "r");
    char* plaintext = NULL;
    size_t ptSize;
    int ptLength = getline(&plaintext, &ptSize, fp);
    fclose(fp);
    ptLength--;

    int fp_count = ptLength;
    int i;

    for (i = 0; i < fp_count; i++) {
        if ((isalpha(plaintext[i]) == 0) && (isspace(plaintext[i]) == 0)) {
            perror("ILLEGAL CHARARCTER IN INPUT FILE...EXITING.\n");
            fflush(stdout);
            exit(1);
        }
    }
    strncpy(buffer_0, plaintext, ptLength);

    // Open key file and get data
    fp = fopen(argv[2], "r");
    char* key = NULL;
    size_t keySize;
    int keyLength = getline(&key, &keySize, fp);
    fclose(fp);

    int fp2_count = keyLength;

    strncpy(buffer_1, key, keyLength);
    //If key is shorter than file then action cannot be completed
    if (fp_count > fp2_count) {
        printf("ERROR: KEY IS SHORTER THAN PLAINTEXT FILE! ACTION CANNOT BE COMPLETED.\n");
        fflush(stdout);
        return 0;
    }
    //Combines all buffers into buffer 2 seperated by a newline character 
    strcpy(buffer_2, buffer_0);
    strcat(buffer_2, "\n");
    strcat(buffer_2, buffer_1);

    // Check usage & args
    if (argc < 3) {
        fprintf(stderr, "USAGE: %s hostname port\n", argv[0]);
        exit(0);
    }

    // Create a socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0) {
        error("CLIENT: ERROR opening socket");
    }

    // Set up the server address struct
    // Passes the LAST TWO arguments in argv into the address struct
    setupAddressStruct(&serverAddress, atoi(argv[argc - 1]), argv[argc - 2]);

    // Connect to server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        error("CLIENT: ERROR connecting");
    }

    int dec_code = 123;

    //Send challenge to server
    charsWritten = send(socketFD, &dec_code, sizeof(dec_code), 0);

    int recv_check = 0;

    //Recieve status code from server 
    charsRead = recv(socketFD, &recv_check, sizeof(recv_check), 0);
    //If serer does not send positive status then no work is done and program exits
    if (recv_check != 0) {
          goto SKIP_WORK;
    }

    int fp_full;
    fp_full = fp_count + fp2_count;

    //Send message length to server
    charsWritten = send(socketFD, &fp_full, (sizeof(fp_full)), 0);


    // Send message to server
    int write_count = 0;
    int left_to_write = fp_full;
    int loop_count = 0;
    charsWritten = send(socketFD, buffer_2, fp_full, 0);
    //While message is not fully sent, loop send()
    while (write_count < fp_full) {
        charsWritten = send(socketFD, buffer_2, left_to_write, 0);
        if (charsRead < 0) {
            error("ERROR reading from socket\n");
        }
        else {
            write_count += charsWritten;
            left_to_write -= charsWritten;
        }
        //Controls potential runaway loops
        loop_count++;
        if (loop_count > 20) {
            error("LOOP COUNT EXCEEDED\n");
            break;
        }
    }
    if (charsWritten < 0) {
        error("CLIENT: ERROR writing to socket");
    }
    if (charsWritten < fp_count) {
        printf("CLIENT: WARNING: Not all data written to socket!\n");
    }

    // Clear out the buffer again for reuse
    memset(buffer_1, '\0', sizeof(buffer_1));

    loop_count = 0;
    int read_count;
    read_count = 0;
    int left_to_read;
    left_to_read = fp_count;
    
    //Loop recv for taking in message from dec server
    int pos = 0;
    while (pos < fp_count) {
        charsRead = recv(socketFD, buffer_1 + pos, fp_count - pos, 0);
        pos += charsRead;
        //printf("CHARS READ: %d POS: %d FP_COUNT: %d\n", charsRead, pos, fp_count);
        loop_count++;
        if (loop_count > 20) {
            error("LOOP COUNT EXCEEDED\n");
            break;
        }
    }
    //Printing decrypted message to stdout
    printf("%s\n", buffer_1);
SKIP_WORK:;
    // Close the socket
    close(socketFD);
    return 0;
}