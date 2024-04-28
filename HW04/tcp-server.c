/* tcp-server.c */

/* To test this server, you can use the following
   command-line netcat tool:

   bash$ netcat linux00.cs.rpi.edu ~
         ^^^^^^
      in this case, netcat will act as a client to
       this TCP server....

   The hostname (e.g., linux00.cs.rpi.edu) cannot be
   localhost (127.0.0.1); and the port number must match what
   the server reports.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// 
// Compare the behavior here with UDP
//
#if 1
#define MAXBUFFER 8192
#else
#define MAXBUFFER 16
#endif

int main()
{
  //
  // We need to create a socket to wait for communication requests. 
  // A socket requires a host address, and it requires a port number.
  // The following code creates a UDP socket on the host (... with the IP
  // address of this host), and then "names" or "binds" it to a specific 
  // port.
  // 

  // vvvv Same as UDP, almost (SOCK_STREAM instead of SOCK_DGRAM) vvvv
  int sd = socket(AF_INET, SOCK_STREAM, 0); // Create a socket at our IP address.
  if (sd == -1)
  {
    perror("socket() failed: ");
    return EXIT_FAILURE;
  }
  printf("Socket: %d\n", sd);

  struct sockaddr_in server;
  socklen_t length = sizeof(server);
  
  /* htons() is host-to-network short for data marshalling */
  /* Internet is big endian; Intel is little endian */
  server.sin_family = AF_INET;
  server.sin_port = htons(8123);
  server.sin_addr.s_addr = htonl( INADDR_ANY );

  if (bind(sd, (struct sockaddr *) &server, sizeof(server)) < 0)
  {
    perror("bind() failed: ");
    return EXIT_FAILURE;
  }

  printf("Before getsockname() port %d\n", ntohs(server.sin_port));
  if (getsockname(sd, (struct sockaddr *) &server, &length) < 0)
  {
    perror("getsockname() failed: ");
    return EXIT_FAILURE;
  }
  printf("Bound to port %d\n", ntohs(server.sin_port));

  // ^^^^ Same as UDP, almost (SOCK_STREAM instead of SOCK_DGRAM) ^^^^

  int n;
  char buffer[MAXBUFFER + 1];
  struct sockaddr_in client;
  
  //
  // Here is a difference. We will listen for connection 
  // requests on this socket
  //
  if (listen(sd, 5) < 0)
  {
    perror("listen() failed: ");
    return EXIT_FAILURE;
  }

  while ( 1 )
  {
    //
    // Now accept connection requests and create new 
    // connections via a new socket (file) descriptor
    //
    int newsd = accept(sd, (struct sockaddr *) &client, &length);
    if (newsd < 0)
    {
      perror("accept() failed: ");
      return EXIT_FAILURE;
    }
    printf( "SERVER: Accepted new client connection on newsd %d\n", newsd );

    /* we have just established a TCP connection between server and a
        remote client; below implements the application-layer protocol */

    /* the sd variable is listener socket descriptor that we use to accept
        new incoming client connections (port 8123) */

    /* the newsd variable is the means of communicating via recv()/send()
        or read()/write() with the connected client */

    do
        {
          printf( "Blocked on recv()\n" );

          /* recv() call will block until we receive data (n > 0)
              or there's an error (n == -1)
               or the client closed its socket (n == 0) */
          n = recv( newsd, buffer, MAXBUFFER, 0 );   /* or read() */

          if ( n == -1 )
          {
            perror( "recv() failed" );
            return EXIT_FAILURE;
          }
          else if ( n == 0 )
          {
            printf( "Rcvd 0 from recv(); closing socket...\n" );
          }
          else /* n > 0 */
          {
            buffer[n] = '\0';   /* assume this is text data */
            // inet_ntoa() takes an IP address and returns it as a string
            // for printing.
            printf( "Rcvd message from %s: %s\n",
                     inet_ntoa( (struct in_addr)client.sin_addr ),
                     buffer );

            /* send ACK message back to client */
            n = send( newsd, "ACK\n", 4, 0 );

            if ( n != 4 )
            {
              perror( "send() failed" );
              return EXIT_FAILURE;}
            }
        }
        while ( n > 0 );

        close( newsd );

  }

  return EXIT_SUCCESS;
}