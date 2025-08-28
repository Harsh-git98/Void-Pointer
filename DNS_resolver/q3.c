#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

void resolve_name_to_address(const char *hostname) {
    struct addrinfo hints, *res, *p;
    char ipstr[INET6_ADDRSTRLEN];
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     
    hints.ai_socktype = SOCK_STREAM; 

    if ((status = getaddrinfo(hostname, NULL, &hints, &res)) != 0) {
        fprintf(stderr, "Error: %s\n", gai_strerror(status));
        return;
    }

    printf("\nResolved Address(es) for %s:\n", hostname);

    for (p = res; p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("  - %s (%s)\n", ipstr, ipver);
    }

    freeaddrinfo(res); 
}


void resolve_service_to_port(const char *service, const char *protocol) {
    struct addrinfo hints, *res;
    int status;

    memset(&hints, 0, sizeof hints);

    
    if (strcmp(protocol, "tcp") == 0) {
        hints.ai_socktype = SOCK_STREAM;
    } else if (strcmp(protocol, "udp") == 0) {
        hints.ai_socktype = SOCK_DGRAM;
    } else {
        fprintf(stderr, "Error: Unknown protocol '%s'. Use tcp or udp.\n", protocol);
        return;
    }

    if ((status = getaddrinfo(NULL, service, &hints, &res)) != 0) {
        fprintf(stderr, "Error: %s\n", gai_strerror(status));
        return;
    }

    if (res != NULL) {
        int port;
        if (res->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
            port = ntohs(ipv4->sin_port);
        } else {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)res->ai_addr;
            port = ntohs(ipv6->sin6_port);
        }

        printf("\nService Name: %s\n", service);
        printf("Protocol: %s\n", protocol);
        printf("Port Number: %d\n", port);
    }

    freeaddrinfo(res);
}

int main() {
    char input[256];
    printf("Enter hostname/IP or service+protocol (e.g., 'http tcp'):\n> ");

    
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0; 

    char *first = strtok(input, " ");
    char *second = strtok(NULL, " ");

    if (second == NULL) {
      
        resolve_name_to_address(first);
    } else {
       
        resolve_service_to_port(first, second);
    }

    return 0;
}
