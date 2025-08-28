#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>

void resolve_service_name(const char *service_name) {
    struct servent *service;

    service = getservbyname(service_name, "tcp");
    if (service == NULL) {
        service = getservbyname(service_name, "udp");
    }

    if (service == NULL) {
        fprintf(stderr, "Error: Could not resolve service name '%s'\n", service_name);
        return;
    }

    printf("\nOfficial Service Name: %s\n", service->s_name);

    
    if (service->s_aliases[0] != NULL) {
        printf("Aliases:\n");
        for (int i = 0; service->s_aliases[i] != NULL; i++) {
            printf("  - %s\n", service->s_aliases[i]);
        }
    } else {
        printf("Aliases: None\n");
    }

    printf("Port Number: %d\n", ntohs(service->s_port));
    printf("Protocol Used: %s\n", service->s_proto);
}

void resolve_port_number(int port) {
    struct servent *service;

    int network_port = htons(port);

    service = getservbyport(network_port, "tcp");
    if (service == NULL) {
        service = getservbyport(network_port, "udp");
    }

    if (service == NULL) {
        fprintf(stderr, "Error: Could not resolve port number '%d'\n", port);
        return;
    }

    printf("\nOfficial Service Name: %s\n", service->s_name);

    if (service->s_aliases[0] != NULL) {
        printf("Aliases:\n");
        for (int i = 0; service->s_aliases[i] != NULL; i++) {
            printf("  - %s\n", service->s_aliases[i]);
        }
    } else {
        printf("Aliases: None\n");
    }

    printf("Port Number: %d\n", ntohs(service->s_port));
    printf("Protocol Used: %s\n", service->s_proto);
}

int main() {
    char input[256];
    printf("Enter service name or port number: ");
    scanf("%255s", input);

    int is_number = 1;
    for (int i = 0; input[i] != '\0'; i++) {
        if (!isdigit((unsigned char)input[i])) {
            is_number = 0;
            break;
        }
    }

    if (is_number) {
        int port = atoi(input);
        resolve_port_number(port);
    } else {
        resolve_service_name(input);
    }

    return 0;
}
