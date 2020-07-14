/*
Computer Network Project 1

by eopXD, b04705001 

Project Request: 
	Implement ping, including to implement client ping and host a server that responds.

	Compile:
		gcc server.c -o server

	Command:
		./server [listen_port]

	Output:
		recv from [client_ip:client_port]
		
Implementation:
	Maintain a file descriptor table and also IP and ports of it.
	Recieve everything and directly echo it back.
*/

#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <resolv.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#define MAX_CLIENT 128
#define PACKET_SIZE 128


int to_number ( char* s ) {
	int res = 0;
	for ( int i=0; i<strlen(s); i++ ) {
		if ( !('9'>=s[i] && s[i]>='0') ) return -1;
		else res = res*10 + s[i] - '0';
	}
	return res;
}
int main ( int argc, char **argv ) 
{
	fclose(stderr);
	
// Step 0: parse argument
// Wrong argument lah~~QAQ
#define WRONG_ARG_LAH do {\
	fprintf (stderr, "Bad command lah ~~~ QAQ\n");\
    fprintf (stderr, "Usage: eopXD_server [listen_port]\n");\
	exit ( 1 );\
} while ( 0 )
	if ( argc != 2 ) WRONG_ARG_LAH;
	if ( to_number ( argv[1] ) < 0 ) WRONG_ARG_LAH;

	int socket_fd, conn_fd;						// master socket, new client socket
	struct sockaddr_in addr;					// master address
	struct sockaddr_in client_addr;				// new client address
	
	char hostnames[MAX_CLIENT][PACKET_SIZE];	// hostname table
	int ports[MAX_CLIENT];						// port_num table

	char buffer[PACKET_SIZE];					// buffer for reading
	int addr_len, read_len;						// length
	int tmp = 1;
	
	fd_set active_fds, read_fds;				// file descriptor set
// step 1: setup socket
	if ( (socket_fd = socket ( AF_INET, SOCK_STREAM, 0)) < 0 ) {
		fprintf (stderr, "Create socket fail ~~~ QAQ\n");
    	exit ( 1 );
	}
	else fprintf (stderr, "Now have master socket descriptor %d\n",socket_fd);
	if ( setsockopt ( socket_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0 ) {
    	fprintf (stderr, "Set socket option failed ~~~ (‘⊙д-) \n");
		exit ( 1 );
    }
    else fprintf (stderr, "set socket option success\n");

// step 2: bind
    bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons ( to_number( argv[1]) );

    if ( bind ( socket_fd, (struct sockaddr *)&addr, sizeof(addr) ) < 0 )    {   
    	fprintf (stderr, "Bind failed ~~~ (‘⊙д-) \n");
		exit ( 1 );
    } 
// step3: listen
    if ( listen ( socket_fd, MAX_CLIENT ) < 0 ) {   
        fprintf ( stderr, "Listen failed ~~~ OAO\n");
        exit ( 1 );   
    } 
   	// add into file descriptor set
    FD_ZERO ( &active_fds );
  	FD_SET ( socket_fd, &active_fds);
// list out all interface
    fprintf (stderr, "List of interface: \n");
    struct ifaddrs *addrs, *tmp1;
    getifaddrs(&addrs);
	tmp1 = addrs;
	while (tmp1) {
	    if (tmp1->ifa_addr && tmp1->ifa_addr->sa_family == AF_INET) {
	        struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp1->ifa_addr;
	        fprintf (stderr, "%s: %s\n", tmp1->ifa_name, inet_ntoa(pAddr->sin_addr));
	    }
	    tmp1 = tmp1->ifa_next;
	}
	freeifaddrs(addrs);
// step4: accept
	fprintf (stderr, "\nI'm a good guy with a good heart, and I have good intentions.\n");
	fprintf (stderr, "Now listening\n\n");
	addr_len = sizeof ( client_addr );
	while ( 1 ) {
		read_fds = active_fds;
		if ( select ( MAX_CLIENT, &read_fds, NULL, NULL, NULL) < 0 ) {
        	fprintf(stderr, "error when select ~~~ OAO\n");
          	exit ( 1 );
        }
        for ( int i=0; i <MAX_CLIENT; i++ ) {
	        if ( FD_ISSET (i, &read_fds ) ) {
	            if ( i == socket_fd ) {			// connection request on master socket
	                if ( (conn_fd=accept ( socket_fd, (struct sockaddr*)&client_addr, (socklen_t*)&addr_len)) < 0 ) {
		   				fprintf(stderr, "error when accepting ~~~ OAO\n");
            			exit ( 1 );
					}
					fprintf (stderr, "connection from host %s, port %d.\n", inet_ntoa (client_addr.sin_addr),
	                	ntohs (client_addr.sin_port));
	                
	                strcpy ( hostnames[conn_fd], inet_ntoa (client_addr.sin_addr) );
	                ports[conn_fd] = ntohs ( client_addr.sin_port );
	            	FD_SET ( conn_fd, &active_fds );
	            }
	            else {							// data from some client socket
	            	read_len = read ( i, buffer, sizeof(buffer) );
	            	if ( read_len < 0 ) {
	            		fprintf(stderr, "fail reading ~~~ OAO\n");
	            		exit ( 1 );
					}
					else if ( read_len == 0 || !strcmp(buffer,"") ) {
						fprintf( stderr, "EOF\n");
						close ( i );
						FD_CLR ( i, &active_fds );		
					}
					else {
						printf ("recv from %s:%d\n", hostnames[i],ports[i]);
						fprintf (stderr, "buffer read: %s\n",buffer);
						fprintf (stderr, "======echo it back======\n");
						send ( i, buffer, sizeof(buffer), 0 );
					}
				}
			}
		}
	}
	exit ( 0 );
}