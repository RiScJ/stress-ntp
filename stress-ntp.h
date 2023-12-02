#ifndef STRESS_NTP_H
#define STRESS_NTP_H

#include <arpa/nameser.h>
#include <resolv.h>

/**
 * @brief Resolves FQDN to an IP address
 *
 * @param[in] fqdn FQDN to resolve
 * @param[put] daddr Resolved IP address
 */
int resolve_fqdn(char* fqdn, in_addr_t* daddr);

#endif
