/*
Computer Network Project 1

by eopXD, b04705001 

Project Request: 
	Implement ping, including to implement client ping and host a server that responds.

	Compile:
		gcc client.c -o client

	Command:
		./client [-n number] [-t timeout] [host:port ..]

	Option:
		-n: pack number send to server. If 0, then send message until closing program. (default: 0)
		-t: maximum millisecond client need to wait. (default: 1000)
	Output:
		recv from [server_ip], RTT = [delay] msec
		timeout when connect to [server_ip]
		
Implementation:
	Open multiple sockets for each host specified, then send message to it and calculate the RTT.
	Sent message is in the form: "This is eopXD connecting [hostname] for the [some_number] time"
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
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#define MAX_HOST 128
#define PACKET_SIZE 128
#define PING_SLEEP 1000000

struct host_info {
	struct sockaddr_in addr;
	char host[512];
	unsigned short port_num;
	char msg[512];
	int sock_fd, msg_sent_cnt, msg_recv_cnt;
	int bad_connection;
};

struct hostent *hname;

struct host_info hosts[MAX_HOST];
fd_set fds;
int max_fd = -1;

int host_t = 0;

int total_packet = -1;								// total packet send
int max_time_out = -1;								// time out per packet
int ping_loop = 0;									// =1, infinite ping loop

void int_Handler () {
	ping_loop = total_packet = 0; 
	fprintf (stderr, "recieved SIGINT, no more packet sent\n");
	fprintf (stderr, "send EOF package to terminate connection\n");
}
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

// wrong argument lah~~QAQ
#define WRONG_ARG_LAH do {\
	fprintf (stderr, "Bad command lah ~~~ QAQ\n");\
    fprintf (stderr, "Usage: eopXD_client [-n number] [-t timeout] host\n");\
	fprintf (stderr, "-n: pack number send to server. If 0, then send message until closing program. (default: 0)\n");\
	fprintf (stderr, "-t: maximum millisecond client need to wait. (default: 1000)\n\n");\
	fprintf (stderr, "Host is specified after options, need at least 1 host\n");\
    exit ( 1 );\
} while ( 0 )
	
	if ( argc == 1 ) {
		WRONG_ARG_LAH;
	}
// get possible options
	int last_option = 1;
	for ( int i=1; i<argc; ++i ) {
		if ( !strcmp ( argv[i], "-n" ) ) {
			if ( ++i == argc || total_packet != -1 ) WRONG_ARG_LAH;
			if ( (total_packet = to_number ( argv[i] )) == -1 ) WRONG_ARG_LAH;
			last_option = i + 1;
		}
		else if ( !strcmp ( argv[i], "-t" ) ) {
			if ( ++i == argc || max_time_out != -1 ) WRONG_ARG_LAH;
			if ( (max_time_out = to_number ( argv[i] )) == -1 ) WRONG_ARG_LAH;
			last_option = i + 1;
		}
	}
	if ( total_packet == -1 || total_packet == 0 ) ping_loop = 1, total_packet = 0;
	if ( max_time_out == -1 ) max_time_out = 1000;
// get possible hosts
	for ( int i=last_option; i<argc; i++ ) {
		int len = strlen ( argv[i] );
		int meet_column = 0;
		int host_len[2]={};
		char hostname[2][512]={};
		for ( int j=0; j<len; j++ ) {
			if ( argv[i][j] == ':' ) meet_column = 1;
			else hostname[meet_column][host_len[meet_column]++] = argv[i][j];
		}
		hostname[0][host_len[0]++] = hostname[1][host_len[1]++] = 0;
		if ( meet_column != 1 ) {
			fprintf (stderr, "Please specify port lah ~\n\n");
			WRONG_ARG_LAH;
		}
		if ( to_number ( hostname[1] ) >= 0 ) {
			strcpy(hosts[host_t].host, hostname[0]);
			hosts[host_t].port_num = to_number ( hostname[1] );
			hosts[host_t].msg_sent_cnt = hosts[host_t].msg_recv_cnt = 0;
			host_t++;
		}
		else {
			fprintf (stderr, "Error: port number of some host is not an natural number\n\n");
			WRONG_ARG_LAH;
		}
	}
	if ( host_t == 0 ) {
		fprintf (stderr, "Error: no host recieved OAO\n\n");
		WRONG_ARG_LAH;
	}
	fprintf(stderr, "Going to connect following host:\n");
	for ( int i=0; i<host_t; i++ ) fprintf(stderr, "%s, %hu\n", hosts[i].host, hosts[i].port_num);

	FD_ZERO ( &fds );
// initialize: dns lookup, and distribute socket fd, connect
	for ( int i=0; i<host_t; i++ ) {
	// DNS lookup
		if ( (hname = gethostbyname ( hosts[i].host )) == NULL ) {
			fprintf (stderr, "%s: fail DNS lookup ~~ OAO\n",hosts[i].host);
			exit ( 1 );
		}
		hosts[i].addr.sin_family = hname -> h_addrtype;
		hosts[i].addr.sin_port = htons( hosts[i].port_num );
		hosts[i].addr.sin_addr.s_addr = *(long*) hname -> h_addr;
	// get socket fd
	/*	if ( (hosts[i].sock_fd = socket ( AF_INET, SOCK_STREAM, 0)) < 0 ) {
			fprintf (stderr, "%s: socket fd acquire fail ~~ why lah OAO\n",hosts[i].host);
			exit ( 1 );
		}
		if ( hosts[i].sock_fd > max_fd ) max_fd = hosts[i].sock_fd;
		FD_SET ( hosts[i].sock_fd, &fds );*/
	// connect to host
	/*	if ( connect ( hosts[i].sock_fd, (struct sockaddr *) &hosts[i].addr, sizeof(struct sockaddr) ) != 0 ) {
			fprintf (stderr, "%s: connect fail\n",hosts[i].host);
			exit ( 1 );
		}
		else {
			fprintf (stderr, "connect %s with socket fd %d\n",hosts[i].host, hosts[i].sock_fd);
		}*/
	}
	fprintf (stderr, "Initial connection complete\n");
	struct sockaddr_in recv_addr;					// receiving address
	int addr_len = sizeof ( recv_addr );
	
	char msg[PACKET_SIZE]; 							// send message
	char buffer[PACKET_SIZE];						// buffer for recieving
	int msg_len, read_len;
	
	struct timespec send_time[MAX_HOST];			// per ping
	struct timespec recv_time[MAX_HOST];			// per ping
	int recieved[MAX_HOST];
	int all_ok;
	
	char str1[30] = "This is eopXD connecting ";	// msg for packet
	char str2[30] = " for the ";					// msg for packet
	char str4[30] = " time";						// msg for packet
	
	int ttl = 64;									// time to live for routers
	
	double rtt;										// round trip time
	time_t batch_start;
	time_t batch_end;
	
// Catch SIGINT signal
	signal ( SIGINT, int_Handler );


// start ping process
	for ( int i = 0; i<total_packet || ping_loop; i++ ) {
		max_fd = -1;
		FD_ZERO ( &fds );
		bzero ( recieved, sizeof(recieved) );
		for ( int k=0; k<host_t; k++ ) {
			hosts[k].bad_connection = 0;
			if ( (hosts[k].sock_fd = socket ( AF_INET, SOCK_STREAM, 0)) < 0 ) {		// maybe need to re-acquire fd
				fprintf (stderr, "%s: socket fd acquire fail ~~ why lah OAO\n",hosts[k].host);
				exit ( 1 );
			}
			if ( hosts[k].sock_fd > max_fd ) max_fd = hosts[k].sock_fd;
			
			FD_SET ( hosts[k].sock_fd, &fds ); // 就是你

			fprintf(stderr, "%s: lets connect\n", hosts[k].host);
			if ( connect ( hosts[k].sock_fd, (struct sockaddr *) &hosts[k].addr, sizeof(struct sockaddr) ) != 0 ) {
				fprintf (stderr, "%s: connect fail\n",hosts[k].host);
				printf ("timeout when connect to %s\n", inet_ntoa ( hosts[k].addr.sin_addr ) );
				hosts[k].bad_connection = 1;
				continue;
			}
			else {
				fprintf (stderr, "connect %s with socket fd %d\n",hosts[k].host, hosts[k].sock_fd);
			}
	// create packet
			bzero ( &msg, sizeof(msg) );
			msg_len = 0;
			for ( int j=0; j<strlen(str1); j++ ) msg[msg_len++] = str1[j];
			for ( int j=0; j<strlen(hosts[k].host); j++ ) msg[msg_len++] = hosts[k].host[j];
			for ( int j=0; j<strlen(str2); j++ ) msg[msg_len++] = str2[j];
			int j=i; do { msg[msg_len++] = (j%10) + '0'; j/=10; } while ( j!=0 );
			for ( int j=0; j<strlen(str4); j++ ) msg[msg_len++] = str4[j];
			msg[msg_len++] = '\0';
			fprintf (stderr, "\nconnecting %s:%d\npacket message: %s\n\n",hosts[k].host, ntohs ( hosts[k].addr.sin_port ),msg);
	// send packet
			if ( send( hosts[k].sock_fd, msg, sizeof(msg), 0 ) < 0 ) {
				fprintf (stderr, "%s: send package fail\n",hosts[k].host);
				exit ( 1 );
			} 
			else  {
				fprintf (stderr, "%s: sent\n",hosts[k].host);
				strcpy ( hosts[k].msg, msg );
				clock_gettime ( CLOCK_MONOTONIC, &send_time[k] );
				++hosts[k].msg_sent_cnt;
			}
		}
//		printf ("%d ISSET: %d\n", hosts[0].sock_fd, FD_ISSET ( hosts[0].sock_fd, &fds) ); 抓到！！！
	// recieve packet
		batch_start = time ( NULL );
		batch_end = batch_start + (double)(max_time_out)/1000.0;
		do {
			all_ok = 1;
			for ( int k=0; k<host_t; k++ ) 
				if ( recieved[k] == 0 ) all_ok = 0;
			if ( all_ok ) break;
			for ( int k=0; k<host_t; k++ ) {
				if ( recieved[k] || hosts[k].bad_connection ) continue;
				
				if ( FD_ISSET ( hosts[k].sock_fd, &fds ) ) {
					read_len = read ( hosts[k].sock_fd, buffer, sizeof(buffer) );
					if ( read_len < 0 ) {
						fprintf( stderr, "fail reading ~~~ QAQ\nyou need to terminate properly lah\n\n");
						exit ( 1 );
					}
					else {
						if ( strcmp ( hosts[k].msg, buffer ) ) {
							fprintf (stderr,"mine: %s, recieved: %s\n",hosts[k].msg, buffer);
							continue;
						}
						recieved[k] = 1;
						++hosts[k].msg_recv_cnt;
						clock_gettime ( CLOCK_MONOTONIC, &recv_time[k] );
						rtt = (recv_time[k].tv_sec - send_time[k].tv_sec) * 1000.0;
						rtt += ((double)( recv_time[k].tv_nsec - send_time[k].tv_nsec )) / 1000000.0;
						printf ("recv from %s, RTT = %.3f msec\n", inet_ntoa ( hosts[k].addr.sin_addr ) ,
							rtt);
					}
				}
			}	
			batch_start = time ( NULL );	
		} while ( batch_start < batch_end );
		bzero ( &msg, sizeof(msg) );
		for ( int k=0; k<host_t; k++ ) 
			if ( !hosts[k].bad_connection ) {	// if connected, send terminating signal
				if ( !recieved[k] ) 
					printf ("timeout when connect to %s\n", inet_ntoa ( hosts[k].addr.sin_addr ) );
				if ( send( hosts[k].sock_fd, msg, sizeof(msg), 0 ) < 0 ) {
					fprintf (stderr, "%s: send package fail at the end\n\n",hosts[k].host);
				}
				close ( hosts[k].sock_fd );		// close opened file descriptor
			}
		if ( i+1 == total_packet ) break;
		usleep ( PING_SLEEP );
	}


// send EOF package to close connection
/*	bzero ( &msg, sizeof(msg) );
	for ( int k=0; k<host_t; k++ ) {
		if ( send( hosts[k].sock_fd, msg, sizeof(msg), 0 ) < 0 ) {
			fprintf (stderr, "%s: send package fail at the end\n\n",hosts[k].host);
		}
	}*/
	exit ( 0 );
}


