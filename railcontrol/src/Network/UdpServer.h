/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2023 Dominik (Teddy) Mahrer - www.railcontrol.org

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
#include <thread>
#include <vector>

#include <sys/socket.h>

#include "Logger/Logger.h"

namespace Network
{
	class UdpClient;

	class UdpServer
	{
		public:
			UdpServer() = delete;
			UdpServer(const UdpServer&) = delete;
			UdpServer& operator=(const UdpServer&) = delete;

		protected:
			UdpServer(const std::string& address,
				const unsigned short port,
				const std::string& threadName);

			virtual ~UdpServer();

			void StartUdpServer();
			void TerminateUdpServer();

			virtual UdpClient* UdpClientFactory(const int serverSocket,
				const struct sockaddr_storage* clientAddress) = 0;

		protected:
			std::vector<UdpClient*> clients;

		private:
			void SocketCreateBindListen(int family, struct sockaddr* address);

			void Worker();

			UdpClient* GetClient(const struct sockaddr_storage* clientAddress);

			void CleanUpClients();

			Logger::Logger* logger;
			volatile bool run;
			std::vector<std::thread> serverThreads;
			std::string error;
			const std::string& address;
			const unsigned short port;
			int serverSocket;
			const std::string threadName;
	};
}
