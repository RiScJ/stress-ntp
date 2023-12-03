#include "stress-ntp.h"

int create_client(const in_addr_t daddr, int* sockfd) {
	struct sockaddr_in daddr_sin;
	unsigned int sport;

	// Create UDP socket for the client to communicate with the server
	if ((*sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		return SN_ERR_SOCKET; 
	}

	int flags = fcntl(*sockfd, F_GETFL, 0);
	if (flags == -1) return SN_ERR_FC_GET;
	flags |= O_NONBLOCK;
	if (fcntl(*sockfd, F_SETFL, flags) == -1) return SN_ERR_FC_SET;

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

void* send_ntp(void* arg) {
	SendThreadArg* thread_arg = (SendThreadArg*)arg;

	struct sockaddr_in ntp_server_sin;
	memset(&ntp_server_sin, 0, sizeof(ntp_server_sin));
	ntp_server_sin.sin_family = AF_INET;
	ntp_server_sin.sin_port = htons(NTP_PORT);
	ntp_server_sin.sin_addr.s_addr = thread_arg->daddr;

	unsigned char packet[NTP_SIZE] = {0};
	packet[0] = 0b11100011;

	for (int j = 0; j < SN_DEFAULT_REQS_CLT; j++) {
	for (int i = thread_arg->start_client; i < thread_arg->end_client; i++) {
		if (sendto(thread_arg->clients[i], packet, NTP_SIZE, 0, 
				(struct sockaddr*)&ntp_server_sin, sizeof(ntp_server_sin)) 
				< 0) {
			fprintf(stderr, "Error sending packet\n");
			exit(EXIT_FAILURE);
		}
	}
	usleep(1000);
	}

	return NULL;
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
				for (int j = 0; j < i; j++) close(clients[j]);
				exit(EXIT_FAILURE);
			case SN_ERR_FC_GET:
				fprintf(stderr, "Failed to get socket flags\n");
				for (int j = 0; j < i; j++) close(clients[j]);
				exit(EXIT_FAILURE);
			case SN_ERR_FC_SET:
				fprintf(stderr, "Failed to set socket flags\n");
				exit(EXIT_FAILURE);
			case SN_ERR_CONNECT:
				fprintf(stderr, "Failed to connect socket for client %d\n", i);
				for (int j = 0; j < i; j++) close(clients[j]);
				exit(EXIT_FAILURE);
		}
	}

	int n_threads = sysconf(_SC_NPROCESSORS_ONLN);
	if (n_clients < n_threads) n_threads = n_clients;
	pthread_t send_threads[n_threads];
	SendThreadArg send_thread_args[n_threads];

	int base_clients_per_thread = n_clients / n_threads;
	int rem_clients = n_clients % n_threads;
	int start_client = 0;

	struct timespec start_time, end_time;

	printf("Sending NTP requests...\n");
	printf("    Clients: %d\n", n_clients);
	printf("    Threads: %d\n", n_threads);
	printf("    Packets per client: %d\n", SN_DEFAULT_REQS_CLT);
	printf("    Total packets: %d\n", SN_DEFAULT_REQS_CLT * n_clients);

	clock_gettime(CLOCK_MONOTONIC, &start_time);

	for (int i = 0; i < n_threads; i++) {
		int clients_for_thread = base_clients_per_thread 
				+ (i < rem_clients ? 1 : 0);
		
		send_thread_args[i].start_client = start_client;
		send_thread_args[i].end_client = start_client + clients_for_thread;
		send_thread_args[i].clients = clients;
		send_thread_args[i].daddr = daddr;

		start_client += clients_for_thread;

		if (pthread_create(&send_threads[i], NULL, send_ntp, 
				&send_thread_args[i]) != 0) {
			fprintf(stderr, "Failed to create send thread\n");
			for (int i = 0; i < n_clients; i++) {
				close(clients[i]);
			}
			exit(EXIT_FAILURE);
		}
	}

	for (int i = 0; i < n_threads; i++) {
		pthread_join(send_threads[i], NULL);
	}

	clock_gettime(CLOCK_MONOTONIC, &end_time);	

	double dt = end_time.tv_sec - start_time.tv_sec + (end_time.tv_nsec
			- start_time.tv_nsec) / 1e9;
	
	printf("Sent requests in %f seconds\n", dt);
	printf("    Rate: %g pps\n", SN_DEFAULT_REQS_CLT * n_clients / dt);

	for (int i = 0; i < n_clients; i++) {
		close(clients[i]);
	}	
	return 0;
}
