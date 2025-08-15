#pragma once

#define WIN32_LEAN_AND_MEAN

// Windows API or otherwise Windows specific headers
//
// NOTE: If you wish to use <windows.h> in the same
// program that's using <winsock2.h>, you must define
// #define WIN32_LEAN_AND_MEAN
// prior to the inclusion of <windows.h>. 
//
// NOTE: If you wish to use <lphlpapi.h> in the same
// program that's using <winsock2.h>, you must include
// include <winsock2.h> before <lphppapi.h>.
#if _WIN32
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>

// POSIX-compliant API or otherwise POSIX-compliant headers
#elif  defined(__linux__) || defined(__APPLE__)
	#include <socket.h>

#endif

// C Standard Library headers or otherwise UCRT64-compatible headers
#include <stdio.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <process.h>