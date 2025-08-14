/*
This template is written for anyone wanting to use, or learn, the
very basics of how to initalize a HTTP server using C. 

It can be used as the foundation for a larger program, or as an 
educational reference to see how to use C to create a HTTP server,
or how to use winsock2.h (Windows) or socket.h (POSIX)


If you think there should be other foundational features that are not
in this template, feel free to submit a pull request. 
*/


/*
Any questions you might have with the code, check out the Q&A section
of the repository. I might have an answer there.
*/


#include "server.h"
#include <psdk_inc/_socket_types.h>
#include <winsock.h>
#include <winsock2.h>



// Change port number if desired (HAS TO BE CHAR*)
#define PORT "888"

#define BUFFER_SIZE 1024


#if _WIN32


int main(void)
{

	// NOTE: After initialization of WSADATA, you must free with WSACleanup().
	WSADATA wsa_data;
	int     int_result;
	int     int_send_result;

	// NOTE: result needs to be freed with freeaddrinfo() after initialization
	struct addrinfo* result = NULL;
	struct addrinfo  hints;
	struct addrinfo* index;

	char recv_buffer[BUFFER_SIZE];
	int  recv_buffer_length = BUFFER_SIZE;

	SOCKET listening_socket; // Socket that the server listens to
	SOCKET connection_socket;    // Socket that the server accepts connections from clients



	// MAKEWORD(2, 2) corresponds to version 2.2 (latest version as of August 2025)
	int_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (int_result != 0)
	{
		printf("WSAStartup() failure: %d\n", int_result);
		exit(-1);
	}


	// SecureZeroMemory(destination, length) == memset(destination, 0, length). !!!
	SecureZeroMemory(&hints, sizeof(hints));

	// Set the parameters of the struct
	hints.ai_family   = AF_INET;           // Specifies the IPv4 address family (AF_INET6 for IPv6)
	hints.ai_socktype = SOCK_STREAM;       // Specifies a stream socket
	hints.ai_protocol = IPPROTO_TCP;       // Specifies the TCP protocol
	hints.ai_flags    = 0;        // Specifies that the caller (this program) intends to
	                                       // use the returned socket information to bind itself to.


	// Get the local address and port and load into the "result" structure
	// (port is manually set, see top of script)
	//
	// NOTE: The first parameter is set to NULL because hints.ai_flags is set to AI_PASSIVE. What this means is that
	// the server will listen to EVERY IPv4 interface that the computer has (ex. Ethernet/Wifi, Virtual machines, Link-local addresses). 
	// If this isn't your intent and you want to specify a specific IP address, set hints.ai_flags to 0 and replace NULL with your IP address.
	//
	// NOTE: To those new to all this IP stuff, your computer only ever has access to its PRIVATE IP address. If you want
	// to recieve connections from devices that aren't connected to your LAN/WLAN (Ethernet, Wifi), you have to PORT FORWARD
	// a port so your router can NAT translate incoming requests from your public IP and forward it to your computer.
	// Port forwarding is required to recieve external requests regardless of whether you're using a wildcard IPv4 address
	// or a specific one.
	int_result = GetAddrInfo("127.0.0.1", PORT, &hints, &result);
	if (int_result != 0)
	{
		printf("GetAddrInfo() failure: %d\n", int_result);
		WSACleanup();
		exit(-1);
	}


	// Attempt to create and bind to a socket
	for (index = result; index != NULL; index = index->ai_next)	
	{
		// Create listen socket
		listening_socket = socket(
			index->ai_family,
			index->ai_socktype,
			index->ai_protocol);

		if (listening_socket == INVALID_SOCKET)
		{
			printf("Error: Socket. %d\n", WSAGetLastError());
			continue;
		}


		BOOL exclusive = TRUE;

		// Set exclusive
		int_result = setsockopt(
			listening_socket,
			SOL_SOCKET,
			SO_EXCLUSIVEADDRUSE,
			(const char*)&exclusive,
			sizeof(exclusive));

		if (int_result == -1)
		{
			printf("Error: setsockopt(). %d\n", WSAGetLastError());
			continue;
		}

		// Set up listening socket
		int_result = bind(
			listening_socket,
			result->ai_addr,
			(int)result->ai_addrlen);
		
		if (int_result == 0)
			break;
		else
		{
			printf("Error: Bind failed. %d\n", WSAGetLastError());
			closesocket(listening_socket);
		}
	}

	if (listening_socket == INVALID_SOCKET)
	{
		printf("Couldn't bind to any address with port: %d\n", atoi(PORT));
		FreeAddrInfo(result);
		closesocket(listening_socket);
		WSACleanup();
		exit(-1);
	}

	// Start listening to the binded socket
	//
	// NOTE: SOMAXCONN is just a special constant for the parameter "backlog", which just
// states the maximum length of the queue of pending connections for the socket to accept.
	int_result = listen(listening_socket, SOMAXCONN);
	if (int_result == SOCKET_ERROR) 
	{
		printf("Failed to listen. Error: %d\n", WSAGetLastError());
		closesocket(listening_socket);
		FreeAddrInfo(result);
		WSACleanup();
		exit(-1);
	}


	for (;;)
	{
		// TODO: Implement multi-threading
		// Start accepting connections, ignore the client's address information
		connection_socket = accept(
			listening_socket,
			NULL,
			NULL);

		do
		{
			int_result = recv(
				connection_socket,
				recv_buffer,
				recv_buffer_length,
				0);

			if (int_result > 0)
			{
				printf("Bytes recieved: %d\n", int_result);
				putchar('\n');

				char* body = 
				"<!doctype html><meta charset=\"utf-8\">"
				"<title>Hello Teto</title>"
				"<div><h1>Hello Teto</h1></div>";

				char header[4096];
				int header_length = snprintf(
					header, 
					sizeof(header),
					"HTTP/1.1 200 OK\r\n"
					"Content-Type: text/html; charset-utf-8\r\n"
					"Content-Length: %d\r\n"
					"Connection: close\r\n"
					"\r\n",
					(int)strlen(body)
					);

				int_send_result = send(
					connection_socket,
					header,
					header_length,
					0);
				if (int_send_result == SOCKET_ERROR)
				{
					printf("Failed to send: %d\n", WSAGetLastError());
					closesocket(connection_socket);
					continue;
				}

				int_send_result = send(
					connection_socket,
					body,
					strlen(body),
					0);
				if (int_send_result == SOCKET_ERROR)
				{
					printf("Failed to send body: %d\n", WSAGetLastError());
					closesocket(connection_socket);
					continue;
				}

				printf("Bytes sent: %d\n", int_send_result);
			}
			else if (int_result == 0)
			{
				printf("Connection closing.\n");
			}
			else
			{
				printf("recv failed: %d\n", WSAGetLastError());
				closesocket(connection_socket);
				continue;
			}

		} while (int_result > 0);
	}

	int_result = shutdown(connection_socket, SD_SEND);
	if (int_result > 0)
	{
		printf("Shutdown failed. %d\n", WSAGetLastError());
		closesocket(connection_socket);
		closesocket(listening_socket);
		WSACleanup();
		exit(-1);
	}

	// Clean up
	FreeAddrInfo(result); 
	closesocket(listening_socket);
	closesocket(connection_socket);
	WSACleanup();
	return 0;
}

#elif defined(__linux__) || defined(__APPLE__)
//todo

#endif