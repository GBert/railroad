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

#pragma once

#include <string>
#include <vector>

#include <sys/socket.h>

#include "Logger/Logger.h"
#include "Utils/Network.h"

namespace Network
{
	class UdpClient
	{
		public:
			UdpClient() = delete;
			UdpClient(const UdpClient&) = delete;
			UdpClient& operator=(const UdpClient&) = delete;

			inline UdpClient(const int serverSocket,
				const struct sockaddr_storage* clientAddress,
				const time_t timeout = 60)
			:	run(true),
				lastUsed(time(nullptr)),
				timeout(timeout),
				serverSocket(serverSocket),
				clientAddress(*clientAddress)
			{
			}

			virtual ~UdpClient()
			{
			}

			inline bool IsClient(const struct sockaddr_storage* address)
			{
				return Utils::Network::CompareAddresses(&clientAddress, address);
			}

			inline void Update()
			{
				lastUsed = time(nullptr);
			}

			inline bool Terminated()
			{
				return !run;
			}

			inline bool TimedOut()
			{
				return lastUsed + timeout < time(nullptr);
			}

			virtual void Work(const unsigned char* buffer, const size_t size) = 0;


		protected:
			inline void Send(const unsigned char* buffer, const size_t size)
			{
				sendto(serverSocket, buffer, size, 0, reinterpret_cast<const struct sockaddr*>(&clientAddress), sizeof(struct sockaddr_storage));
			}

			inline void Terminate()
			{
				run = false;
			}

			inline std::string AddressAsString()
			{
				return Utils::Network::AddressToString(&clientAddress);
			}

		private:
			volatile bool run;
			time_t lastUsed;
			time_t timeout;
			const int serverSocket;
			const struct sockaddr_storage clientAddress;
	};
}
