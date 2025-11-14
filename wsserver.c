// System Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

// Non-system Libraries
#include "sha1.h"
#include "base64.h"

// int main.
int main(int argc, char *argv[]){
	printf("WebSocket Server v0.0.8 Minimal\n\n");

	// Variables
	int port = 50100;
	int serving_fd, connection_fd, n;
	struct sockaddr_in address;
	uint8_t responce[2049];
	uint8_t received[2049];

	// Read Command-line Inputs
	for(int i=1; i<argc; i++){
		// Look for "--port" in argv
		if(strcmp(argv[i], "--port") == 0 && argv[i+1]){
			// Convert String to Integer
			port = atoi(argv[i+1]);
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
		// Accept Connection
		connection_fd = accept(serving_fd, (struct sockaddr*)NULL, NULL);

		// Clean buffer
		memset(received, 0, 2048);

		// Read message in 2048 byte chunks
		if((n=read(connection_fd, received, 2047)) > 4){
			// Output to std
			printf("%s", received);

			// Detect if request end exists (not a good way)
			if(received[n-1] == '\n' && received[n-2] == '\r' && received[n-3] == '\n' && received[n-4] == '\r'){
				// Checks for WebSocket Upgrade Request
				int checks = 0;

				int get_next_token = 0;
				char *sec_ws_key;

				// Split data by \n,\r, and whitespace
				char *token;
				token = strtok(received, "\n\r ");
				while(token != NULL){
					// Copy the string if get_next_token is 1
					if(get_next_token == 1){
						// Allocate memory for key with space for magic string
						sec_ws_key = malloc(strlen(token)+37);
						// Copy
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
						// Set get_next_token to 1 if this token is "Sec-WebSocket-Key:"
						get_next_token = 1;
					}
					// Get next token (split string segment)
					token = strtok(NULL, "\n\r ");
				}

				// If all 8 checks passed, then success
				if(checks >= 8){
					// Concatenate Magic String to Client's Key 
					strcat(sec_ws_key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

					// New SHA-1 Hash
					Sha1Digest hash_digest = Sha1_get(sec_ws_key, strlen(sec_ws_key));
					free(sec_ws_key); // No longer needed

					// Allocate 20 bytes
					uint8_t hash_result[20];

					// Loop through 32bit hash digest
					for(int i=0; i<5; i++){
						uint32_t a = htonl(hash_digest.digest[i]);

						// Convert uint32 to 4x uint8
						for(int j=0; j<4; j++){
							hash_result[i*4+j] = ((uint8_t*)&a)[3-j];
						}
					}

					// Base64 Encode
					char *b64_result = base64_encode(hash_result, (size_t)20, NULL); // The input length is always 20

					printf("Info: Received WebSocket Upgrade Request\nSec-WebSocket-Accept: %s\n", b64_result);

					// Respond with WebSocket Handshake
					snprintf((char*)responce, sizeof(responce), "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n", b64_result);
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
				// Respond with Error 400 if it gets cut off (request > 4kib)
				snprintf((char*)responce, sizeof(responce), "HTTP/1.0 400 Bad Request\r\n\r\n");
				write(connection_fd, (char*)responce, strlen((char*)responce));
				close(connection_fd);
			}

			// Clean buffer
			memset(received, 0, 2048);
		}

		// Check for issues
		if(n < 0){
			printf("Error: Read Error\n");
			exit(1);
		}
	}

	// This line should (in theory) never get executed.
	return 0;
}
