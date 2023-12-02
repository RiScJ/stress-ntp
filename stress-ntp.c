#include "stress-ntp.h"

int create_client(const in_addr_t daddr, int* sockfd) {
	struct sockaddr_in daddr_sin;
	unsigned int sport;

	// Create UDP socket for the client to communicate with the server
	if ((*sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		return SN_ERR_SOCKET; 
	}

	// Specify information for the server
	daddr_sin.sin_family = AF_INET; // IPv4 address family
	daddr_sin.sin_port = htons(NTP_PORT); // NTP service
	daddr_sin.sin_addr.s_addr = daddr; // Address of the server

	// Choose a random source port and bind the socket
	struct sockaddr_in saddr_sin;
	memset(&saddr_sin, 0, sizeof(saddr_sin));
	saddr_sin.sin_family = AF_INET;
	saddr_sin.sin_addr.s_addr = htonl(INADDR_ANY);
	while (true) {
		sport = rand() % (SN_MAX_SPORT - SN_MIN_SPORT + 1) + SN_MIN_SPORT;
		saddr_sin.sin_port = htons(sport);

		if (bind(*sockfd, (const struct sockaddr*)&saddr_sin, 
				sizeof(saddr_sin)) >= 0) {
			break;
		}
	}

	// Connect the socket to the server
	if (connect(*sockfd, (const struct sockaddr*)&daddr_sin,
			sizeof(daddr_sin)) < 0) {
		return SN_ERR_CONNECT; 
	}

	return SN_SUCCESS;
}

int resolve_fqdn(char* fqdn, in_addr_t* daddr) {
    struct hostent* h;
    h = gethostbyname(fqdn);
    if (h == NULL) return SN_ERR_NP;
    struct in_addr* target;
    target = (struct in_addr*) (h->h_addr);
    *daddr = target->s_addr;
    return 0;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "Insufficient arguments\n");
		exit(EXIT_FAILURE);
	}

	char* host = argv[1];	

	in_addr_t daddr = 0;
	switch (resolve_fqdn(host, &daddr)) {
		case SN_ERR_NP:
			fprintf(stderr, "Failure resolving FQDN\n");
			exit(EXIT_FAILURE);
	}
    
	int n_clients = SN_DEFAULT_N_CLIENTS;
	int clients[n_clients];

	srand(time(NULL));	
	
	for (int i = 0; i < n_clients; i++) {
		switch (create_client(daddr, &clients[i])) {
			case SN_ERR_SOCKET:
				fprintf(stderr, "Failed to create socket for client %d\n", i);
				for (int j = 0; j < i; j++) {
					close(clients[j]);
				}
				exit(EXIT_FAILURE);
			case SN_ERR_BIND:
				fprintf(stderr, "Failed to bind socket for client %d\n", i);
				for (int j = 0; j < i; j++) {
					close(clients[j]);
				}
				exit(EXIT_FAILURE);
			case SN_ERR_CONNECT:
				fprintf(stderr, "Failed to connect socket for client %d\n", i);
				for (int j = 0; j < i; j++) {
					close(clients[j]);
				}
				exit(EXIT_FAILURE);
		}
	}

	for (int i = 0; i < n_clients; i++) {
		close(clients[i]);
	}	
	return 0;
}
