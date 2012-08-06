#ifndef __NET_H
#define __NET_H

int set_non_block(int fd);

int tcp_create_listener(char *hostname, char *port, unsigned backlog);

#endif
