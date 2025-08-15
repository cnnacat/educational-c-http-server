/*
This template is written for anyone wanting to use, or learn, the
very basics of how to initalize a HTTP server using C. 


It mainly serves as an educationa reference to see how to use C to create a 
HTTP server, or how to use winsock2.h (Windows) or socket.h (POSIX)


With a little bit of work, you could probably make it a fully functional and
proper HTML server. But as of now, it only serves HTML.


If you think there should be other foundational features that are not
in this template, feel free to submit a pull request. 
*/


/*
Any questions you might have with the code, check out the Q&A section
of the repository. I might have an answer there.
*/


#include "server.h"
#include <winerror.h>
#include <winsock2.h>



// Change port number if desired (HAS TO BE CHAR*)
#define PORT "888"
// For use in sending/recieving data
#define BUFFER_SIZE 1024
// Program exit signal
atomic_bool EXIT = false;



#if _WIN32

BOOL WINAPI ctrl_handler(DWORD event)
{
	switch (event)
	{
	case CTRL_C_EVENT:
		{
			EXIT = true;
			return true;
		}
	case CTRL_BREAK_EVENT:
		{
			EXIT = true;
			return true;
		}
	case CTRL_CLOSE_EVENT:
		{
			EXIT = true;
			return true;
		}
	default:
		return FALSE;
	}
}


void CALLBACK worker_thread(
	PTP_CALLBACK_INSTANCE instance, 
	PVOID                 parameter,
	PTP_WORK              work)
{
	//todo;
	SOCKET connection_socket = (SOCKET)(UINT_PTR)parameter;
	int int_result;
	int int_send_result;

	int recv_timeout = 500; // 500ms / 0.5s
	setsockopt(
		connection_socket,
		SOL_SOCKET,
		SO_RCVTIMEO,
		(const char*)&recv_timeout,
		sizeof(recv_timeout));

	char recv_buffer[BUFFER_SIZE];
	int  recv_buffer_length = BUFFER_SIZE;

	do
	{
		printf("THREAD\n");

		int_result = recv(
			connection_socket,
			recv_buffer,
			recv_buffer_length,
			0);

		if (int_result > 0)
		{
			int sent_bytes_header = 0;
			int sent_bytes_body   = 0;

			printf("Bytes recieved: %d\n", int_result);

			char* body = 
			"<!doctype html><meta charset=\"utf-8\">"
			"<title>Hello Teto</title>"
			"<div><h1>Hello Teto</h1></div>";

			char header[4096];
			int header_length = _snprintf_s(
				header, 
				sizeof(header),
				_TRUNCATE,
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: text/html; charset=utf-8\r\n"
				"Content-Length: %d\r\n"
				"Connection: close\r\n"
				"\r\n",
				(int)strlen(body)
				);
			if (header_length < 0)
			{
				printf("Truncation occured while formatting header.\n");
				break;
			}
			while (sent_bytes_header < header_length)
			{
				int_send_result = send(
					connection_socket,
					header        + sent_bytes_header, // Advance to the next batch of data to send
					header_length - sent_bytes_header,   // Take in account already sent data
					0);
				if (int_send_result == SOCKET_ERROR)
				{
					printf("Failed to send header. Code: %d\n", WSAGetLastError());
					break;
				}
				sent_bytes_header += int_send_result;
			}

			while (sent_bytes_body < strlen(body))
			{
				int_send_result = send(
					connection_socket,
					body         + sent_bytes_body,
					strlen(body) - sent_bytes_body,
					0);
				if (int_send_result == SOCKET_ERROR)
				{
					printf("Failed to send body. Code %d\n", WSAGetLastError());
					break;
				}
				sent_bytes_body += int_send_result;
			}

			printf("Sent %d bytes\n", sent_bytes_body + sent_bytes_header);
			putchar('\n');
			break;
		}
		else if (int_result == 0)
		{
			printf("Connection closing.\n");
			break;
		}
		else
		{
			int error = WSAGetLastError();
			if (error == WSAETIMEDOUT)
			{
				printf("recv timed out\n");
				break;
			}
			printf("recv failed: %d\n", WSAGetLastError());
			break;
		}

	} while (int_result > 0);

	int_result = shutdown(connection_socket, SD_SEND);
	if (int_result == SOCKET_ERROR)
	{
		printf("Shutdown failed. Code: %d\n", WSAGetLastError());
	}

	closesocket(connection_socket);
	return;

}


int main(void)
{
	SetConsoleCtrlHandler(ctrl_handler, TRUE);

	TP_CALLBACK_ENVIRON callback_environment;
	PTP_CLEANUP_GROUP clean_up_group = NULL;

	PTP_POOL thread_pool = CreateThreadpool(NULL);
	if (thread_pool == NULL)
	{
		printf("Thread pool failed to create. Code: %lu\n", GetLastError());
		exit(-1);
	}

	InitializeThreadpoolEnvironment(&callback_environment);
	clean_up_group = CreateThreadpoolCleanupGroup();
	SetThreadpoolCallbackPool(&callback_environment, thread_pool);
	SetThreadpoolCallbackCleanupGroup(
		&callback_environment,
		clean_up_group,
		NULL);

	// NOTE: After initialization of WSADATA, you must free with WSACleanup().
	WSADATA wsa_data;
	int     int_result;

	// NOTE: result needs to be freed with freeaddrinfo() after initialization
	struct addrinfo* result = NULL;
	struct addrinfo  hints;
	struct addrinfo* index;


	SOCKET listening_socket;     // Socket that the server listens to
	SOCKET connection_socket;    // Socket that the server accepts connections from clients

	fd_set socket_status;


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
	hints.ai_flags    = 0;                 // Options. 0 (false) if nothing.


	// NOTE: To use a specific IP address, the first paramter of GetAddrInfo() must be an IP address. 127.0.0.1 is a special
	// loopback IPv4 address (aka localhost) that allows a computer to send and recieve data to itself.
	//
	// If you want to use a wildcard IP address (connect to any IPv4 interface currently established on your computer, 
	// set the first parameter of GetAddrInfo() to NULL and hints.ai_flags to AI_PASSIVE.
	//
	// NOTE: If you want to allow external connections, note that your computer only ever has access to its PRIVATE IP address. If you want
	// to recieve connections from devices that aren't connected to your LAN/WLAN (Ethernet, Wifi), you have to PORT FORWARD
	// a port so your router can NAT translate incoming requests from your public IP and forward it to your computer.
	int_result = GetAddrInfo("127.0.0.1", PORT, &hints, &result);
	if (int_result != 0)
	{
		printf("GetAddrInfo() failure: %d\n", int_result);
		WSACleanup();
		exit(-1);
	}

	// Attempt to create and bind to a socket !!!
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

		// Set listen socket to be exclusive to the main thread
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

		// Set up (bind) the listening socket
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

	TIMEVAL timer = {1, 0}; // 1 sec, 0 ms
	while (!EXIT)
	{
		FD_ZERO(&socket_status);
		FD_SET(listening_socket, &socket_status);

		int accept_ready = select(
			0,
			&socket_status, // this is the readfds parameter; Add all sockets to socket_status set if it's ready to be accepted
			NULL,
			NULL,
			&timer);
		if (accept_ready == SOCKET_ERROR)
		{
			printf("Select Error. Code: %d\n", WSAGetLastError());
			continue;
		}
		else if (accept_ready == 0) // 0 sockets ready (no incoming connections)
			continue;

		if (FD_ISSET(listening_socket, &socket_status))
		{
			connection_socket = accept(
				listening_socket,
				NULL,
				NULL);

			PTP_WORK worker_init = CreateThreadpoolWork(
				&worker_thread,
				(PVOID)(UINT_PTR)connection_socket,
				&callback_environment);
			if (worker_init == NULL)
			{
				printf("Failed to initialize worker function. Code: %lu\n", GetLastError());
				closesocket(connection_socket);
				continue;
			}

			SubmitThreadpoolWork(worker_init);
		}
	}
	closesocket(listening_socket);

	CloseThreadpoolCleanupGroupMembers(
		clean_up_group, 
		TRUE,
		NULL);
	CloseThreadpoolCleanupGroup(clean_up_group);
	DestroyThreadpoolEnvironment(&callback_environment);
	CloseThreadpool(thread_pool);

	// Clean up
	FreeAddrInfo(result); 

	WSACleanup();

	printf("Shutdown successful.\n");
	getchar();
	return 0;
}

#elif defined(__linux__) || defined(__APPLE__)
//todo

#endif