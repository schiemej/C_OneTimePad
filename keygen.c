#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()


int main(int argc, char* argv[]){
	int i;
	char* a = argv[1];
	int num = atoi(a);

	srand(time(0));

	for (i = 0; i < num; i++) {
		char randomletter = 'A' + (rand() % 26);
		printf("%c",randomletter);
		fflush(stdout);
	}
	printf("\n");
	fflush(stdout);

	exit(0);
}