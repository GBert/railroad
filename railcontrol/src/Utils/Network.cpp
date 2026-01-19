/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2026 by Teddy / Dominik Mahrer - www.railcontrol.org

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
#include <cstring>
#include <netdb.h>

#include "Utils/Network.h"

using std::string;

namespace Utils
{
	string Network::AddressToString(const struct sockaddr_storage *sockaddr)
	{
		char buffer[40] = { 0 };
		unsigned short port = 0;
	    switch(sockaddr->ss_family) {
	        case AF_INET:
	        {
	        	const struct sockaddr_in* sa4 = reinterpret_cast<const struct sockaddr_in*>(sockaddr);
	            inet_ntop(AF_INET, &(sa4->sin_addr), buffer, sizeof(buffer));
	            port = ntohs(sa4->sin_port);
	            break;
	        }

	        case AF_INET6:
	        {
	        	const struct sockaddr_in6* sa6 = reinterpret_cast<const struct sockaddr_in6*>(sockaddr);
	            inet_ntop(AF_INET6, &(sa6->sin6_addr), buffer, sizeof(buffer));
	            port = ntohs(sa6->sin6_port);
	            break;
	        }

	        default:
	            break;
	    }
	    return string("[") + string(buffer) + string("]:") + std::to_string(port);
	}

	bool Network::CompareAddresses(const struct sockaddr_storage *address1, const struct sockaddr_storage *address2)
	{
		const unsigned short family = (reinterpret_cast<const struct sockaddr*>(address1))->sa_family;
		if (family != (reinterpret_cast<const struct sockaddr*>(address2))->sa_family)
		{
			return false;
		}

		switch (family)
		{
			case AF_INET:
			{
				const struct sockaddr_in* inet1 = reinterpret_cast<const struct sockaddr_in*>(address1);
				const struct sockaddr_in* inet2 = reinterpret_cast<const struct sockaddr_in*>(address2);
				if (inet1->sin_port != inet2->sin_port)
				{
					return false;
				}
				return (0 == memcmp(&(inet1->sin_addr), &(inet2->sin_addr), sizeof(inet1->sin_addr)));
			}

			case AF_INET6:
			{
				const struct sockaddr_in6* inet1 = reinterpret_cast<const struct sockaddr_in6*>(address1);
				const struct sockaddr_in6* inet2 = reinterpret_cast<const struct sockaddr_in6*>(address2);
				if (inet1->sin6_port != inet2->sin6_port)
				{
					return false;
				}
				return (0 == memcmp(&(inet1->sin6_addr), &(inet2->sin6_addr), sizeof(inet1->sin6_addr)));
			}

			default:
				return false;
		}
	}

	bool Network::HostResolves(const string& host)
	{
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = PF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags |= AI_CANONNAME;

		struct addrinfo* res;
		struct addrinfo* result;
		int errcode = getaddrinfo(host.c_str(), NULL, &hints, &result);
		if (errcode != 0)
		{
			return false;
		}

		res = result;

		while (res)
		{
			char addrstr[100];
			inet_ntop(res->ai_family, res->ai_addr->sa_data, addrstr, sizeof(addrstr));

			switch (res->ai_family)
			{
				case AF_INET:
				case AF_INET6:
					freeaddrinfo(result);
					return true;

				default:
					res = res->ai_next;
					break;
			}
		}

		freeaddrinfo(result);
		return false;
	}
} // namespace Utils
