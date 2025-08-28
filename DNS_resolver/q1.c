#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void resolve_hostname(const char *hostname) {
    struct hostent *host;
    host = gethostbyname(hostname);

    if (host == NULL) {
        fprintf(stderr, "Error: Could not resolve hostname %s\n", hostname);
        return;
    }

    printf("\nOfficial Hostname: %s\n", host->h_name);

    if (host->h_aliases[0] != NULL) {
        printf("Aliases:\n");
        for (int i = 0; host->h_aliases[i] != NULL; i++) {
            printf("  %s\n", host->h_aliases[i]);
        }
    } else {
        printf("Aliases: None\n");
    }

   
    if (host->h_addrtype == AF_INET)
        printf("Address Type: IPv4\n");
    else if (host->h_addrtype == AF_INET6)
        printf("Address Type: IPv6\n");
    else
        printf("Address Type: Unknown\n");


    printf("Address Length: %d bytes\n", host->h_length);

   
    printf("IP Addresses:\n");
    for (int i = 0; host->h_addr_list[i] != NULL; i++) {
        char ip[INET6_ADDRSTRLEN];
        inet_ntop(host->h_addrtype, host->h_addr_list[i], ip, sizeof(ip));
        printf("  %s\n", ip);
    }
}

void resolve_ip(const char *ip_address) {
    struct in_addr ipv4addr;
    struct hostent *host;

    if (inet_pton(AF_INET, ip_address, &ipv4addr) <= 0) {
        fprintf(stderr, "Error: Invalid IP address format %s\n", ip_address);
        return;
    }

    host = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
    if (host == NULL) {
        fprintf(stderr, "Error: Could not resolve IP %s\n", ip_address);
        return;
    }

    printf("\nOfficial Hostname: %s\n", host->h_name);

    if (host->h_aliases[0] != NULL) {
        printf("Aliases:\n");
        for (int i = 0; host->h_aliases[i] != NULL; i++) {
            printf("  %s\n", host->h_aliases[i]);
        }
    } else {
        printf("Aliases: None\n");
    }

    if (host->h_addrtype == AF_INET)
        printf("Address Type: IPv4\n");
    else if (host->h_addrtype == AF_INET6)
        printf("Address Type: IPv6\n");
    else
        printf("Address Type: Unknown\n");

    printf("Address Length: %d bytes\n", host->h_length);
    
    printf("IP Addresses:\n");
    for (int i = 0; host->h_addr_list[i] != NULL; i++) {
        char ip[INET6_ADDRSTRLEN];
        inet_ntop(host->h_addrtype, host->h_addr_list[i], ip, sizeof(ip));
        printf("  %s\n", ip);
    }
}

int main() {
    char input[256];
    printf("Enter a hostname or IP address: ");
    scanf("%255s", input);
    int is_ip = 1;
    for (int i = 0; i < strlen(input); i++) {
        if (!(input[i] == '.' || (input[i] >= '0' && input[i] <= '9'))) {
            is_ip = 0;
            break;
        }
    }

    if (is_ip) {
        resolve_ip(input);
    } else {
        resolve_hostname(input);
    }

    return 0;
}
