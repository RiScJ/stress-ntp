#ifndef STRESS_NTP_H
#define STRESS_NTP_H

#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>

#include "sn_err.h"
#include "sn_def.h"

typedef struct {
	int start_client;
	int end_client;
	int* clients;
	in_addr_t daddr;
	int reqs;
	int rate;
	int n_clients;
} SendThreadArg;

typedef struct {
	uint8_t li_vn_mode;
	uint8_t stratum;
	uint8_t poll;
	uint8_t precision;
	uint32_t root_delay;
	uint32_t root_dispersion;
	uint32_t ref_id;
	uint32_t ref_timestamp;
	uint32_t orig_timestamp;
	uint32_t recv_timestamp;
	uint32_t trans_timestamp;
} ntp_packet;

/**
 * @brief Creates a socket to be used as a virtual NTP client
 *
 * @param[in] daddr Destination server address
 * @param[out] sockfd File descriptor for the client socket
 *
 */
int create_client(const in_addr_t daddr, int* sockfd);

/**
 * @brief Resolves FQDN to an IP address
 *
 * @param[in] fqdn FQDN to resolve
 * @param[put] daddr Resolved IP address
 */
int resolve_fqdn(char* fqdn, in_addr_t* daddr);

#endif
