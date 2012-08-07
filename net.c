#include "net.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <string.h>

int set_non_block(int fd) {
    int flags;
    flags = fcntl(fd, F_GETFL, 0);

    if (!(flags & O_NONBLOCK)) {
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    return 0;
}


int tcp_create_listener(char *hostname, char *port, unsigned backlog) {
    int status, fd, reuse_addr;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if(!hostname)
        hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(hostname, port, &hints, &servinfo);
    if(status) {
        /* TODO: Check the error code and set errno appropriately */
        return -1;
    }

    fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(fd == -1)
        return -1;

    /* Make the socket available for reuse immediately after it's closed */
    reuse_addr = 1;
   status = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));

    if(status == -1)
        return -1;

    /* Bind the socket to the address */
    status = bind(fd, servinfo->ai_addr, servinfo->ai_addrlen);
    if(status == -1)
        return -1;

    /* Listen for incoming connections */
    status = listen(fd, backlog);
    if(status == -1)
        return -1;

    return fd;
}
