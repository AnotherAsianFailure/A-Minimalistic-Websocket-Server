// System Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

// Non-system Libraries
#include "sha1.h"
#include "base64.h"

// Global Variables
int max_con = 64; // Maximum amount of Client Connections or Worker Threads
int *connections; // Connection FD Buffer

// Worker Thread does the WebSocket Communication for Every Client
void *worker(void *arg){
	// Convert Void to Int
	int id = *((int*)arg);

	printf("\nThread %d: Started - Connection FD: %d\n", id, connections[id]);

	// Frame Header Data Buffer
	uint8_t extract[3];

	for(;;){
		// Clean
		memset(extract, 0, 3);

		// Read from Socket
		if(recv(connections[id], extract, 2, 0) > 0){
			printf("\nThread %d: Frame Received\n", id);

			uint64_t payload_length;

			// Extracting Fram Header Data
			uint8_t fin = extract[0] >> 7;
			uint8_t opcode = extract[0] & 0b00001111;
			uint8_t p_len = extract[1] & 0b01111111;

			printf("- FIN: %d\n- OPCODE: %d\n", fin, opcode);

			// Payload length (7-bit) is weird
			if(p_len <= 125){
				// p_len is Payload Length
				payload_length = p_len;
			}else if(p_len == 126){
				// The next 2 bytes is Payload Length
				uint8_t p[2];
				if(read(connections[id],p,2)){
					payload_length = (p[0]<<8)|p[1];
				}
			}else{
				// The next 8 bytes is Payload Length
				uint8_t p[8];
				if(read(connections[id],p,8)){
					payload_length = (((uint64_t)p[0]<<56)&0b01111111)|((uint64_t)p[1]<<48)|((uint64_t)p[2]<<40)|
					((uint64_t)p[3]<<32)|((uint64_t)p[4]<<24)|((uint64_t)p[5]<<16)|((uint64_t)p[6]<<8)|(uint64_t)p[7];
				}
			}
			printf("- Payload Length: %llu\n", payload_length);

			// Next 4 bytes is Masking Key
			uint8_t mask[4];
			if(read(connections[id], mask, 4)){
				// Create Buffer for Payload Data
				uint8_t encoded[payload_length];

				if(read(connections[id], encoded, payload_length)){
					printf("- Raw: %s\n", encoded);

					// Buffer for Decoded Data
					char decoded[payload_length+1];
					decoded[payload_length] = 0;

					// Loop through Encoded to Decode
					for(uint64_t i=0; i<payload_length; i++){
						// Use the 4 Masking Bytes to XOR the Raw Data
						decoded[i] = encoded[i] ^ mask[i % 4];
					}

					// The Real Data has been Extracted!
					printf("- Decoded Data: %s\n", decoded);

					// This Server "Reflects" All Data it Receives to All Clients
					if(opcode == 0 || opcode == 1 || opcode == 2){
						// Calculate Response Length
						uint64_t r_len = payload_length+2;
						if(p_len == 126){
							r_len += 2;
						}else if(p_len == 127){
							r_len += 8;
						}

						// Make New Buffer for Reflect Data
						uint8_t reflect[r_len];
						memset(reflect, 0, r_len);

						// Add Frame Header Data
						reflect[0] = fin << 7 | opcode;

						int position = 1;

						// Set Payload Length in Header
						if(p_len <= 125){
							// If smaller than 125
							reflect[1] = p_len;
							position++;
						}else if(p_len == 126){
							// Extended 16-bit Length
							reflect[1] = (uint8_t)126;
							reflect[2] = (uint8_t)(payload_length >> 8);
							reflect[3] = (uint8_t)payload_length;
							position += 3;
						}else if(p_len == 127){
							// Excessively Extended 64-bit Length
							reflect[1] = (uint8_t)127;
							reflect[2] = (uint8_t)(payload_length >> 56);
							reflect[3] = (uint8_t)(payload_length >> 48);
							reflect[4] = (uint8_t)(payload_length >> 40);
							reflect[5] = (uint8_t)(payload_length >> 32);
							reflect[6] = (uint8_t)(payload_length >> 24);
							reflect[7] = (uint8_t)(payload_length >> 16);
							reflect[8] = (uint8_t)(payload_length >> 8);
							reflect[9] = (uint8_t)payload_length;
							position += 9;
						}

						// Copy over the Payload Data
						for(uint64_t i=0; i<payload_length; i++){
							reflect[i+position] = decoded[i];
						}

						// Loop through All Clients and Send a Copy to Each
						for(int i=0; i<max_con; i++){
							if(connections[i]){
								printf("Thread %d: Frame Reflected to Connection %d\n", id, i);
								// Write into Socket
								write(connections[i], reflect, r_len);
							}
						}
					}else if(opcode == 8){
						close(connections[id]);
						printf("Thread %d: Connection Closed\n", id);
						connections[id] = 0;
						return NULL;
					}
				}else{
					break;
				}
			}else{
				break;
			}
		}else{
			break;
		}
	}

	// Delete self from existance
	printf("Thread %d: Stopping...\n", id);
	close(connections[id]);
	connections[id] = 0;
	return NULL;
}

// int main.
int main(int argc, char *argv[]){
	printf("WebSocket Reflector v0.1.2 Minimal\n\n");

	// Variables
	int port = 50100;
	int serving_fd, connection_fd, n;
	struct sockaddr_in address;
	uint8_t response[2049];
	uint8_t received[2049];

	// Read Command-line Inputs
	for(int i=1; i<argc; i++){
		// Look for "--port" in argv
		if(strcmp(argv[i], "--port") == 0 && argv[i+1]){
			// Convert String to Integer
			port = atoi(argv[i+1]);
		}else if((strcmp(argv[i], "--max-con") == 0) && argv[i+1]){
			// Convert String to Integer
			max_con = atoi(argv[i+1]);
		}
	}

	// Use Default Port if Custom Port is invalid
	if(port <= 0 || port > 65535){
		printf("Warning: Invalid Port Number\n");
		port = 50100;
	}

	// Use Default Max Connection if Custom Limit is invalid
	if(max_con <= 0 || max_con > 1024){
		printf("Warning: Invalid Maximum Connection Limit\n");
		max_con = 64;
	}

	printf("Using Port: %d\nMaximum Connections: %d\n\n", port, max_con);

	// Allocate Memory for Connection FD Buffer
	connections = malloc(max_con*4);
	// Clean
	memset(connections, 0, max_con*4);

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

	if((listen(serving_fd, 16)) < 0){
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
			printf("\nConnection Number: %d\n\n%s", connection_fd, received);

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
					
					// Loop through Connection FD Buffer to find Free Space
					for(int i=0; i<max_con; i++){
						if(connections[i] == 0){ // Free
							connections[i] = connection_fd;

							printf("Info: Received WebSocket Upgrade Request\nSec-WebSocket-Accept: %s\n", b64_result);

							// Respond with WebSocket Handshake
							snprintf((char*)response, sizeof(response), "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n", b64_result);
							write(connection_fd, (char*)response, strlen((char*)response));

							// Create New Worker Thread to Handle this Connection
							pthread_t thread_id;
							pthread_create(&thread_id, NULL, worker, (void*)&i);

							break;
						}else if(i == max_con-1){// No Free
							printf("Warning: Reached Maximum Connection Limit, Refusing New Connection.\n");
							// Respond with Error 503 to refuse WebSocket Connection
							snprintf((char*)response, sizeof(response), "HTTP/1.1 503 Service Unavailable\r\n\r\n");
							write(connection_fd, (char*)response, strlen((char*)response));
							close(connection_fd);
						}
					}
				}else{
					// Respond with Polite Greeting
					snprintf((char*)response, sizeof(response), "HTTP/1.0 200 OK\r\n\r\nHi");
					write(connection_fd, (char*)response, strlen((char*)response));
					close(connection_fd);
				}
			}else{
				// Respond with Error 400 if it gets cut off (request > 2kib)
				snprintf((char*)response, sizeof(response), "HTTP/1.0 400 Bad Request\r\n\r\n");
				write(connection_fd, (char*)response, strlen((char*)response));
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
