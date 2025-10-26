// System Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>

// Non-system Libraries
#include "sha1.h"
#include "base64.h"

// int main.
int main(int argc, char *argv[]){
	printf("WebSocket Server v0.0.7\n\n");

	// Variables
	int port = 50100;
	int serving_fd, connection_fd, n;
	struct sockaddr_in address;
	uint8_t responce[4097];
	uint8_t received[4097];

	// Reset Errno
	errno = 0;

	// Read Command-line Inputs
	for(int i=1; i<argc; i++){
		// Look for "--port" in argv
		if((strcmp(argv[i], "--port") == 0) && argv[i+1]){
			// Convert port string to long int
			char *endptr;
			port = strtol(argv[i+1], &endptr, 10);

			// Check for problems
			if(errno != 0){
				printf("Error: StrToL Conversion Error\n");
				exit(1);
			}
			if(endptr == argv[i+1]){
				printf("Warning: Argument Not a Number\n");
			}else if(*endptr != '\0'){
				printf("Info: Argument Not a Full Integer\n");
			}
		}
	}

	// Use Default Port if Custom Port is invalid
	if(port <= 0 || port > 65535){
		printf("Warning: Invalid Port Number\n");
		port = 50100;
	}

	printf("Using Port: %d\n\n", port);

	// Socket
	if((serving_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("Error: Socket Error\n");
		exit(1);
	}

	// Address
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	// Bind and Listen
	if((bind(serving_fd, (struct sockaddr*)&address, sizeof(address))) < 0){
		printf("Error: Binding Error\n");
		exit(1);
	}

	if((listen(serving_fd, 8)) < 0){
		printf("Error: Listening Error\n");
		exit(1);
	}

	// Infinite Loop
	for(;;){
		struct sockaddr_in addr;
		socklen_t addrLength;

		// Accept Connection
		connection_fd = accept(serving_fd, (struct sockaddr*)NULL, NULL);

		// Clean buffer
		memset(received, 0, 4096);

		// Read message in 4096 byte chunks
		if((n=read(connection_fd, received, 4095)) > 4){
			// Output to std
			printf("%s", received);

			// Detect if request end exists (not a good way)
			if(received[n-1] == '\n' && received[n-2] == '\r' && received[n-3] == '\n' && received[n-4] == '\r'){
				// Checks for WebSocket Upgrade Request
				int checks = 0;

				int get_next_token = 0;
				char sec_ws_key[64];

				// Make copy of received data
				char receivedstr[strlen(received)+1];
				strcpy(receivedstr, received);

				// Split data by \n,\r, and whitespace
				char *token;
				token = strtok(receivedstr, "\n\r ");
				while(token != NULL){
					// Copy the string if get_next_token is true
					if(get_next_token == 1){
						strcpy(sec_ws_key, token);
						get_next_token = 0;
					}

					// There is no switch in C (T_T)
					if(strcmp(token, "GET") == 0){
						// Increase int checks by 1 for each successful check.
						checks++;
					}
					if(strcmp(token, "HTTP/1.1") == 0){
						checks++;
					}
					if(strcmp(token, "Upgrade:") == 0){
						checks++;
					}
					if(strcmp(token, "websocket") == 0){
						checks++;
					}
					if(strcmp(token, "Connection:") == 0){
						checks++;
					}
					if(strcmp(token, "Upgrade") == 0){
						checks++;
					}
					if(strcmp(token, "Sec-WebSocket-Version:") == 0){
						checks++;
					}
					if(strcmp(token, "Sec-WebSocket-Key:") == 0){
						checks++;
						get_next_token = 1;
					}
					// Get next token (split string segment)
					token = strtok(NULL, "\n\r ");
				}

				// If all 8 checks passed, then success
				if(checks == 8){
					// Concatenate Magic String to Client's Key 
					strcat(sec_ws_key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

					// SHA-1 Hash
					SHA1_CTX sha;
					uint8_t hash_result[20];
					char *hash_buffer = sec_ws_key;

					SHA1Init(&sha);
					SHA1Update(&sha, (uint8_t*)hash_buffer, strlen(hash_buffer));
					SHA1Final(hash_result, &sha);

					// Base64 Encode
					char *b64_result = base64_encode(hash_result, strlen(hash_result), NULL);

					printf("Info: Received WebSocket Upgrade Request\nSec-WebSocket-Accept: %s\n", b64_result);

					// Respond with WebSocket Handshake
					snprintf((char*)responce, sizeof(responce), "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\nHi", b64_result);
					write(connection_fd, (char*)responce, strlen((char*)responce));
					//
					close(connection_fd);
				}else{
					// Respond with Polite Greeting
					snprintf((char*)responce, sizeof(responce), "HTTP/1.0 200 OK\r\n\r\nHi");
					write(connection_fd, (char*)responce, strlen((char*)responce));
					close(connection_fd);
				}
			}else{
				// Respond with Error 431 if it gets cut off (request > 4kib)
				snprintf((char*)responce, sizeof(responce), "HTTP/1.0 431 Request Header Fields Too Large\r\n\r\n");
				write(connection_fd, (char*)responce, strlen((char*)responce));
				close(connection_fd);
			}

			// Clean buffer
			memset(received, 0, 4096);
		}

		// Check for issues
		if(n < 0){
			printf("Error: Read Error\n");
			exit(1);
		}
	}

	// This line should (in theory) never get executed,
	// as this program can only be either terminated or killed [insert knife emoji].
	return 0;
}
