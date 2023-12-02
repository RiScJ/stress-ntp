#include "stress-ntp.h"

int resolve_fqdn(char* fqdn, in_addr_t* daddr) {
    struct hostend* h;
    h = gethostbyname(fqdn);
    if (h == NULL) return ERR_NP;
    struct in_addr* target;
    target = (struct in_addr*) (h->h_addr);
    *daddr = target->s_addr;
    return 0;
}

int main(int argc, char** argv) {

    return 0;
}
