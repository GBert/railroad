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
#include <netinet/in.h>

#include "DataModel/LocoBase.h"
#include "DataModel/LocoFunctions.h"
#include "Manager.h"
#include "Network/Select.h"
#include "Network/UdpConnection.h"
#include "Server/CS2/CS2Client.h"
#include "Server/CS2/CS2Server.h"
#include "Utils/Network.h"

using DataModel::Loco;
using DataModel::LocoBase;
using DataModel::LocoFunctionNr;
using DataModel::LocoFunctionState;

namespace Network
{
	class UdpClient;
}

namespace Server { namespace CS2
{
	CS2Server::CS2Server(Manager& manager)
	:	ControlInterface(ControlTypeCS2Server),
		Network::TcpServer("0.0.0.0", CS2ReceiverPort, "CS2Server"),
		logger(Logger::Logger::GetLogger("CS2Server")),
		manager(manager),
		lastClientID(0),
		runUdp(false)
	{
	}

	CS2Server::~CS2Server()
	{
		CleanUpClients();
		logger->Info(Languages::TextCS2ServerStopped);
	}

	void CS2Server::Start()
	{
		StartTcpServer();
		StartUdpServer();
		logger->Info(Languages::TextCS2ServerStarted);
	}

	void CS2Server::Stop()
	{
		TerminateUdpServer();
		TerminateTcpServer();
		// stopping all clients
		for (auto client : clients)
		{
			client->Stop();
		}
	}

	void CS2Server::StartUdpServer()
	{
		runUdp = true;

		// Only IPv4 is used by clients
		struct sockaddr_in serverAddr4;
		memset(reinterpret_cast<char*>(&serverAddr4), 0, sizeof(serverAddr4));
		serverAddr4.sin_family = AF_INET;
		serverAddr4.sin_addr.s_addr = htonl(INADDR_ANY);
		serverAddr4.sin_port = htons(CS2ReceiverPort);
		UdpSocketCreateBindListen(serverAddr4.sin_family, reinterpret_cast<struct sockaddr*>(&serverAddr4));
	}

	void CS2Server::TerminateUdpServer()
	{
		runUdp = false;
		udpServerThread.join();
	}

	void CS2Server::UdpSocketCreateBindListen(int family, struct sockaddr* address)
	{
		udpServerSocket = socket(family, SOCK_DGRAM, 0);
		if (udpServerSocket < 0)
		{
//			error = "Unable to create socket for udp server. Unable to serve clients.";
			return;
		}

		int on = 1;
		int intResult = setsockopt(udpServerSocket, SOL_SOCKET, SO_REUSEADDR, (const void*) &on, sizeof(on));
		if (intResult < 0)
		{
//			error = "Unable to set tcp server socket option SO_REUSEADDR.";
			close(udpServerSocket);
			return;
		}

		intResult = bind(udpServerSocket, address, sizeof(struct sockaddr_in));
		if (intResult < 0)
		{
//			error = "Unable to bind socket for udp server to port. Unable to serve clients.";
			close(udpServerSocket);
			return;
		}

		if (!runUdp)
		{
			close(udpServerSocket);
			return;
		}
		udpServerThread = std::thread(&Server::CS2::CS2Server::UdpWorker, this);
	}

	void CS2Server::UdpWorker()
	{
		Utils::Utils::SetThreadName("CS2 UDP Server");
		Logger::Logger* udpLogger = Logger::Logger::GetLogger("CS2 UDP Server");
		fd_set set;
		struct timeval tv;
		struct sockaddr_storage clientAddress;
		socklen_t clientAddressLength = sizeof(clientAddress);
		while (runUdp)
		{
			// wait for data and abort on shutdown
			int ret;
			do
			{
				FD_ZERO(&set);
				FD_SET(udpServerSocket, &set);
				tv.tv_sec = 1;
				tv.tv_usec = 0;
				ret = TEMP_FAILURE_RETRY(select(FD_SETSIZE, &set, NULL, NULL, &tv));

				if (!runUdp)
				{
					return;
				}
			} while (ret == 0);

			if (ret < 0)
			{
				continue;
			}

			static const int CANCommandBufferLength = 13;
			unsigned char buffer[CANCommandBufferLength];
			memset(reinterpret_cast<char*>(&clientAddress), 0, sizeof(clientAddressLength));
			ssize_t size = recvfrom(udpServerSocket, buffer, sizeof(buffer), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), &clientAddressLength);
			udpLogger->HexIn(buffer, size);
			if (size != CANCommandBufferLength)
			{
				continue;
			}

			buffer[0] = 0x00;
			buffer[1] = 0x30;
			buffer[2] = 0x00;
			buffer[3] = 0x00;
			buffer[4] = 0x08;
			buffer[5] = 0x00;
			buffer[6] = 0x00;
			buffer[7] = 0x00;
			buffer[8] = 0x00;
			buffer[9] = 0x03;
			buffer[10] = 0x08;
			buffer[11] = 0xff;
			buffer[12] = 0xff;

			sendto(udpServerSocket, buffer, sizeof(buffer), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), sizeof(clientAddress));
//			sockaddr_in* clientAddress4 = reinterpret_cast<struct sockaddr_in*>(&clientAddress);
//			clientAddress4->sin_port = htons(CS2SenderPort);
//			Network::UdpConnection udpConnection(logger, reinterpret_cast<struct sockaddr*>(clientAddress4));
//			udpConnection.Bind();
//			udpConnection.Send(buffer, sizeof(buffer));
//			udpConnection.Terminate();
		}
	}

	void CS2Server::Booster(__attribute__((unused)) const ControlType controlType,
		const BoosterState status)
	{
		if (status == BoosterStateGo)
		{
//			for (auto client : clients)
//			{
//				reinterpret_cast<CS2Client*>(client)->SendPowerOn();
//			}
		}
		else
		{
//			for (auto client : clients)
//			{
//				reinterpret_cast<CS2Client*>(client)->SendPowerOff();
//			}
		}
	}

	void CS2Server::LocoBaseSpeed(__attribute__((unused)) const ControlType controlType,
		const LocoBase* locoBase,
		__attribute__((unused)) const Speed speed)
	{
		if (nullptr == locoBase)
		{
			return;
		}
//		for (auto client : clients)
//		{
//			reinterpret_cast<CS2Client*>(client)->SendLocoInfo(locoBase);
//		}
	}

	void CS2Server::LocoBaseOrientation(__attribute__((unused)) const ControlType controlType,
		const LocoBase* locoBase,
		__attribute__((unused)) const Orientation orientation)
	{
		if (nullptr == locoBase)
		{
			return;
		}
//		for (auto client : clients)
//		{
//			reinterpret_cast<CS2Client*>(client)->SendLocoInfo(locoBase);
//		}
	}

	void CS2Server::LocoBaseFunction(__attribute__((unused)) const ControlType controlType,
		const LocoBase* locoBase,
		__attribute__((unused)) const LocoFunctionNr function,
		__attribute__((unused)) const LocoFunctionState state)
	{
		if (!locoBase)
		{
			return;
		}
//		for (auto client : clients)
//		{
//			reinterpret_cast<CS2Client*>(client)->SendLocoInfo(locoBase);
//		}
	}

	void CS2Server::AccessoryState(__attribute__((unused)) const ControlType controlType,
		const DataModel::Accessory* accessory)
	{
		AccessoryBaseState(accessory);
	}

	void CS2Server::SwitchState(__attribute__((unused)) const ControlType controlType,
		const DataModel::Switch* mySwitch)
	{
		AccessoryBaseState(mySwitch);
	}

	void CS2Server::SignalState(__attribute__((unused)) const ControlType controlType,
		const DataModel::Signal* signal)
	{
		AccessoryBaseState(signal);
	}

	void CS2Server::AccessoryBaseState(const DataModel::AccessoryBase* accessoryBase)
	{
		if (!accessoryBase)
		{
			return;
		}
//		for (auto client : clients)
//		{
//			reinterpret_cast<CS2Client*>(client)->SendTurnoutInfo(accessoryBase);
//		}

	}

	void CS2Server::Work(Network::TcpConnection* connection)
	{
		clients.push_back(new CS2Client(++lastClientID, connection, manager));
		CleanUpClients();
	}

	void CS2Server::CleanUpClients()
	{
		for (auto iterator = clients.begin(); iterator != clients.end();)
		{
			CS2Client* client = *iterator;
			if (client->IsTerminated())
			{
				iterator = clients.erase(iterator);
				delete client;
			}
			else
			{
				++iterator;
			}
		}
	}
}} // namespace Server::CS2
