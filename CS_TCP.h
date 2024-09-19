// Header file for CS_TCP functions - Windows and Linux versions

#ifndef CS_TCP_H_INCLUDED
#define CS_TCP_H_INCLUDED

#define SUCCESS  0      // value for functions to return on success
#define FAILURE -1      // value for functions to return on failure

#define U 127           // UPLOAD byte
#define D 63            // DOWNLOAD byte

#ifdef SHUT_WR			// if compiling the Linux version
typedef int SOCKET;		// for compatibility with Windows version
#define SOCKET_ERROR -1	// value returned by Linux functions on error
#endif

/*  Function to create a socket for use with TCP and IPv4.
    The Windows version starts the WSA system if necessary.  */
SOCKET TCPcreateSocket (void);

/*  Function to set up a server for use with TCP and IPv4.
    Arguments are an existing socket to use, and a port number.
    It first defines a service, in a struct of type sockaddr_in.
    Then it binds the socket to this service.   */
int TCPserverSetup (SOCKET lSocket, int port);

/*  Function to use in a server, to wait for a connection request.
    It will then accept the request, making a connection with the client.
    It reports the IP address and port number used by the client.
    Argument is an existing socket, bound to the required port.
    Return value is the socket used for the connection.  */
SOCKET TCPserverConnect (SOCKET lSocket);

/*  Function to use in a client, to request a connection to a server.
    The IP address is given as a string, in dotted decimal form.
    The port number is given as an integer, and converted to
    network form in this function. */
int TCPclientConnect (
            SOCKET cSocket,     // socket to use to make the connection
            char * IPaddrStr,   // IP address of the server as a string
            int port);          // port number for connection

/*  Function to close a socket.  The Windows version tidies up
    the WSA system after closing the last socket. */
void TCPcloseSocket (SOCKET socket2close);

/*  Function to find an IP address from the name of a server.  If the
    server has more than one address, only the first is returned.
    On Windows, the WSA system must be initialised already,
    so call TCPcreateSocket() before calling this function.
    Arguments:  pointer to a string containing the name of the server (host);
    pointer to an in_addr struct to hold the IP address (or NULL if not needed);
    pointer to a string to hold the IP address in dotted decimal form.
    Return value is 0 for success, negative for error.  */
int getIPaddress(char * hostName, struct in_addr * IPaddr, char * IPaddrStr);

/* Function to print messages when something goes wrong...  */
void printProblem(void);

#endif // CS_TCP_H_INCLUDED
