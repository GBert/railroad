/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2025 by Teddy / Dominik Mahrer - www.railcontrol.org

RailControl is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

RailControl is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RailControl; see the file LICENCE. If not see
<http://www.gnu.org/licenses/>.
*/
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>

#include "Network/Select.h"
#include "Network/TcpClient.h"

namespace Network
{
	TcpConnection TcpClient::GetTcpClientConnection(Logger::Logger* logger, const std::string& host, const unsigned short port)
	{
	    struct sockaddr_storage address;
	    struct sockaddr_in* addressPointer = reinterpret_cast<struct sockaddr_in*>(&address);
	    addressPointer->sin_family = AF_INET;
	    addressPointer->sin_port = htons(port);
	    int ok = inet_pton(AF_INET, host.c_str(), &(addressPointer->sin_addr));
	    if (ok <= 0)
	    {
			logger->Error(Languages::TextUnableToResolveAddress, host);
	        return TcpConnection(0);
	    }

	    int sock = socket(AF_INET, SOCK_STREAM, 0);
	    if (sock < 0)
	    {
			logger->Error(Languages::TextUnableToCreateTcpSocket, host, port);
	        return TcpConnection(0);
	    }

	    ok = ConnectWithTimeout(sock, reinterpret_cast<struct sockaddr*>(addressPointer), sizeof(address));
	    if (ok < 0)
	    {
	    	Languages::TextSelector text;
	    	switch (errno)
	    	{
	    		case ECONNREFUSED:
	    			text = Languages::TextConnectionRefused;
	    			break;

	    		case ENETUNREACH:
	    			text = Languages::TextNetworkUnreachable;
	    			break;

	    		default:
	    			text = Languages::TextConnectionFailed;
		    }
			logger->Error(text, host, port);
	        close(sock);
	        return TcpConnection(0);
	    }

		return TcpConnection(sock, &address);
	}

	int TcpClient::ConnectWithTimeout(int sock, struct sockaddr *addr, socklen_t length)
	{
		// Set non-blocking
		long arg = fcntl(sock, F_GETFL, nullptr);
		if (arg < 0)
		{
			return -1;
		}
		arg |= O_NONBLOCK;
		if (fcntl(sock, F_SETFL, arg) < 0)
		{
			return -1;
		}
		// Trying to connect with timeout
		int ret = connect(sock, addr, length);
		if (ret < 0)
		{
			if (errno != EINPROGRESS)
			{
				return -1;
			}
			while (true)
			{
				fd_set myset;
				int valopt;
				socklen_t lon;
				struct timeval tv;
				tv.tv_sec = 3;
				tv.tv_usec = 0;
				FD_ZERO(&myset);
				FD_SET(sock, &myset);
				ret = TEMP_FAILURE_RETRY(select(sock + 1, nullptr, &myset, nullptr, &tv));
				if (ret < 0 && errno == EINTR)
				{
					continue;
				}

				if (ret <= 0)
				{
					return -1;
				}

				// Socket selected for write
				lon = sizeof(int);
				if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*) (&valopt), &lon) < 0)
				{
					return -1;
				}
				// Check the value returned...
				if (valopt)
				{
					return -1;
				}
				break;
			}
		}
		// Set to blocking mode again...
		if ((arg = fcntl(sock, F_GETFL, nullptr)) < 0)
		{
			return -1;
		}
		arg &= (~O_NONBLOCK);
		if (fcntl(sock, F_SETFL, arg) < 0)
		{
			return -1;
		}
		return ret;
	}
}
