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
#include <errno.h>
#include <netinet/in.h>
#include <string.h>



// Change port number if desired (HAS TO BE CHAR*)
#define PORT "8080"
// For use in sending/recieving data
#define BUFFER_SIZE 1024
// Program exit signal
atomic_bool EXIT = false;
// Lazy way of ID'ing a socket
atomic_int  SOCKET_ID = 0;


void hang()
{
	int temp;

	printf("Press ENTER to quit: ");

	while ((temp = getchar()) != '\n'
		&& temp != EOF)
	{
		;
	}

	return;
}


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
	SOCKET connection_socket;
	int    int_result;
	int    int_send_result;
	int    recv_timeout;
	char   recv_buffer[BUFFER_SIZE];
	int    recv_buffer_length;
	char*  body;
	char   header[4096];
	int    header_length;
	int    socket_id;
	int    sent_bytes_body;
	int    sent_bytes_header;

	connection_socket  = (SOCKET)(UINT_PTR)parameter;
	recv_timeout       = 500; // 500ms aka 0.5s
	recv_buffer_length = BUFFER_SIZE;
	socket_id          = SOCKET_ID++;


	setsockopt(
		connection_socket,
		SOL_SOCKET,
		SO_RCVTIMEO,
		(const char*)&recv_timeout,
		sizeof(recv_timeout));

	do
	{
		int_result = recv(
			connection_socket,
			recv_buffer,
			recv_buffer_length,
			0);

		if (int_result > 0)
		{
			sent_bytes_header = 0;
			sent_bytes_body   = 0;

			printf("Socket %d || Bytes recieved: %d\n", socket_id, int_result);

			body = 
			"<!doctype html><meta charset=\"utf-8\">"
			"<title>Hello Teto</title>"
			"<div><h1>Hello Teto</h1></div>";

			header_length = _snprintf_s(
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
				printf("Socket %d || Truncation occured while formatting header.\n", socket_id);
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
					printf("Socket %d || Failed to send header. Code: %d\n", socket_id, WSAGetLastError());
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
					printf("Socket %d || Failed to send body. Code %d\n", socket_id, WSAGetLastError());
					break;
				}

				sent_bytes_body += int_send_result;
			}

			printf("Socket %d || Sent %d bytes\n", socket_id, sent_bytes_body + sent_bytes_header);
			break;
		}
		else if (int_result == 0)
		{
			printf("Socket %d || Connection closing.\n", socket_id);
			break;
		}
		else
		{
			int error = WSAGetLastError();
			if (error == WSAETIMEDOUT)
			{
				printf("Socket %d || recv timed out.\n", socket_id);
				break;
			}
			printf("Socket %d || recv failed. Code: %d\n", socket_id, WSAGetLastError());
			break;
		}

	} while (int_result > 0);

	int_result = shutdown(connection_socket, SD_SEND);
	if (int_result == SOCKET_ERROR)
	{
		printf("Socket %d || Shutdown failed. Code: %d\n", socket_id, WSAGetLastError());
	}

	else
		printf("Socket %d || Successfully shutdown.\n", socket_id);


	hang();
	closesocket(connection_socket);
	return;

}


int main(void)
{
	TP_CALLBACK_ENVIRON callback_environment;
	PTP_CLEANUP_GROUP   clean_up_group;
	PTP_POOL            thread_pool;
	PTP_WORK            worker_init;
	
	WSADATA             wsa_data;
	int                 int_result;

	struct addrinfo*    result;
	struct addrinfo     hints;
	struct addrinfo*    index;

	SOCKET              listening_socket;
	SOCKET              connection_socket;
	BOOL                exclusive;
	int                 accept_ready;

	fd_set              socket_status;
	TIMEVAL             timer;


	result         = NULL;
	index          = NULL;
	clean_up_group = NULL;
	clean_up_group = CreateThreadpoolCleanupGroup();
	thread_pool    = NULL;
	thread_pool    = CreateThreadpool(NULL);
	timer          = (struct timeval){1, 0};

	if (thread_pool == NULL)
	{
		printf("Thread pool failed to create. Code: %lu\n", GetLastError());
		hang();
		exit(-1);
	}	

	InitializeThreadpoolEnvironment   (&callback_environment);
	SetThreadpoolCallbackPool         (&callback_environment, thread_pool);	
	SetThreadpoolCallbackCleanupGroup (
		&callback_environment,
		clean_up_group,
		NULL);	
	SetConsoleCtrlHandler             (ctrl_handler, TRUE);

	// program starts here

	// Startup version 2.2 (latest as of August 2025)
	int_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (int_result != 0)
	{
		printf("WSAStartup() failure: %d\n", int_result);
		hang();
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
		hang();
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


		// Set listen socket to be exclusive to the main thread
		exclusive = TRUE;		
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
			index->ai_addr,
			(int)index->ai_addrlen);
		
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
		hang();
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
		hang();
		exit(-1);
	}

	while (!EXIT)
	{
		FD_ZERO(&socket_status);
		FD_SET (listening_socket, &socket_status);

		accept_ready = select(
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

			worker_init = CreateThreadpoolWork(
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

	// Wait for all threads to finish and clean up threads
	CloseThreadpoolCleanupGroupMembers(
		clean_up_group, 
		TRUE,
		NULL);
	CloseThreadpoolCleanupGroup  (clean_up_group);
	DestroyThreadpoolEnvironment (&callback_environment);
	CloseThreadpool              (thread_pool);

	// Clean up
	FreeAddrInfo(result); 
	WSACleanup  ();

	printf("Shutdown successful.\n");
	hang();
	return 0;
}

#elif defined(__linux__) || defined(__APPLE__)

// hard code for now, linked list later
pthread_t thread_list[100];

void ctrl_handler(int signal_number)
{
	if (signal_number == SIGINT)
		EXIT = true;	
	return;
}

void* worker_function(void* argument)
{
	int  connection_socket;
	int    int_result;
	int    int_send_result;
	int    recv_timeout;
	char   recv_buffer[BUFFER_SIZE];
	int    recv_buffer_length;
	char*  body;
	char   header[4096];
	int    header_length;
	int    socket_id;
	int    sent_bytes_body;
	int    sent_bytes_header;

	connection_socket  = (int)(intptr_t)argument;
	recv_timeout       = 500; // 500ms aka 0.5s
	recv_buffer_length = BUFFER_SIZE;
	socket_id          = SOCKET_ID++;


	setsockopt(
		connection_socket,
		SOL_SOCKET,
		SO_RCVTIMEO,
		(const char*)&recv_timeout,
		sizeof(recv_timeout));

	do
	{
		int_result = recv(
			connection_socket,
			recv_buffer,
			recv_buffer_length,
			0);

		if (int_result > 0)
		{
			sent_bytes_header = 0;
			sent_bytes_body   = 0;

			printf("Socket %d || Bytes recieved: %d\n", socket_id, int_result);

			body = 
			"<!doctype html><meta charset=\"utf-8\">"
			"<title>Hello Teto</title>"
			"<div><h1>Hello Teto</h1></div>";

			header_length = snprintf(
				header, 
				sizeof(header),
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: text/html; charset=utf-8\r\n"
				"Content-Length: %d\r\n"
				"Connection: close\r\n"
				"\r\n",
				(int)strlen(body)
				);

			if (header_length < 0)
			{
				printf("Socket %d || Truncation occured while formatting header.\n", socket_id);
				break;
			}

			while (sent_bytes_header < header_length)
			{
				int_send_result = send(
					connection_socket,
					header        + sent_bytes_header, // Advance to the next batch of data to send
					header_length - sent_bytes_header,   // Take in account already sent data
					0);

				if (int_send_result < 0)
				{
					printf("Socket %d || Failed to send header. Code: %d\n", socket_id, errno);
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

				if (int_send_result < 0)
				{
					printf("Socket %d || Failed to send body. Code %d\n", socket_id, errno);
					break;
				}

				sent_bytes_body += int_send_result;
			}

			printf("Socket %d || Sent %d bytes\n", socket_id, sent_bytes_body + sent_bytes_header);
			break;
		}
		else if (int_result == 0)
		{
			printf("Socket %d || Connection closing.\n", socket_id);
			break;
		}
		else
		{
			int error = errno;
			if (error == EAGAIN
				|| error == EWOULDBLOCK)
			{
				printf("Socket %d || recv timed out.", socket_id);
				break;
			}
			printf("Socket %d || recv failed. Code: %d\n", socket_id, errno);
			break;
		}

	} while (int_result > 0);

	int_result = shutdown(connection_socket, SHUT_WR);
	if (int_result < 0)
	{
		printf("Socket %d || Shutdown failed. Code: %d\n", socket_id, errno);
	}

	else
		printf("Socket %d || Successfully shutdown.\n", socket_id);


	close(connection_socket);
	return NULL;
}


int main(void)
{
	struct sigaction signal_action;

	signal_action.sa_handler = ctrl_handler;
	signal_action.sa_flags   = 0;
	sigemptyset(&signal_action.sa_mask);
	sigaction(
		SIGINT,
		&signal_action,
		(void*)NULL
	);

	int int_result;
	struct addrinfo* result;
	struct addrinfo  hints;
	struct addrinfo* index;
	int listening_socket;
	int connection_socket;
	int exclusive;
	fd_set socket_status;
	int accept_ready;
	struct timeval timer;
	int thread_id;


	thread_id = 0;
	timer = (struct timeval){1, 0};


	memset(
		&hints,
		0,
		sizeof(hints));

	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags    = 0;

	int_result = getaddrinfo(
		"127.0.0.1",
		PORT,
		&hints,
		&result);
	if (int_result < 0)
	{
		printf("getaddrinfo failed. Reason: %s\n", gai_strerror(int_result));
		hang();
		exit(-1);
	}

	for (index = result; index != NULL; index = index->ai_next)
	{
		listening_socket = socket(
			index->ai_family,
			index->ai_socktype,
			index->ai_protocol);

		if (listening_socket < 0)
		{
			printf("Socket creation error. Reason: %s || Code: %d\n", strerror(errno), errno);
			continue;
		}

		// this effectively is equiv to so_exclusiveuseaddr in windows
		// due to the user_id restriction implemented in linux's so_reuseaddr
		exclusive = 1;
		int_result = setsockopt(
			listening_socket,
			SOL_SOCKET,
			SO_REUSEADDR,
			&exclusive, 
			sizeof(exclusive));

		if (int_result)
		{
			printf("Failed to set socket options for a socket. Reason: %s\n", strerror(errno));
		}

		int_result = bind(
			listening_socket,
			index->ai_addr,
			(size_t)index->ai_addrlen);

		if (int_result == 0)
			break;
		else
		{
			printf("Failed to bind. Reason: %s\n", strerror(errno));
			close(listening_socket);
			continue;
		}
	}

	if (listening_socket < 0)
	{
		printf("Couldn't bind to any sockets.\n");
		hang();
		exit(-1);
	}

	int_result = listen(listening_socket, SOMAXCONN);
	if (int_result < 0)
	{
		printf("Failed to listen to the binded socket. Reason: %s\n", strerror(errno));
		hang();
		exit(-1);
	}

	while (!EXIT)
	{
		FD_ZERO(&socket_status);
		FD_SET(listening_socket, &socket_status);

		timer.tv_sec = 1;
		timer.tv_usec = 0;

		accept_ready = select(
			listening_socket + 1,
			&socket_status,
			NULL,
			NULL,
			&timer);

		if (accept_ready < 0)
		{
			printf("Error occured with select(). Reason: %s\n", strerror(errno));
			continue;
		}
/*		else if (accept_ready == 0)
		{
			printf("No connections incoming.\n");
			break;
		}*/

		
		if (FD_ISSET(listening_socket, &socket_status))
		{
			connection_socket = accept(
				listening_socket,
				NULL,
				NULL);
			if (connection_socket < 0)
			{
				printf("Error occured while trying to accept a connection. Reason: %s\n", strerror(errno));
				continue;
			}

			pthread_create(
				&thread_list[thread_id++], 
				NULL,
				worker_function,
				(void*)(intptr_t)connection_socket);
		}

	}

	for (int i = 0; i < thread_id; i++)
	{
		pthread_join(thread_list[i], NULL);
	}

	freeaddrinfo(result);
	close(listening_socket);
	printf("Clean shutdown successful.\n");
	hang();
	return 0;
}
#endif