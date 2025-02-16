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

#pragma once

#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#include <sys/socket.h>

#include "ControlInterface.h"
#include "Logger/Logger.h"
#include "Manager.h"
#include "Network/UdpServer.h"

namespace Server { namespace CS2
{
	class CS2Client;

	class CS2Server : public ControlInterface, private Network::TcpServer
	{
		public:
			static const unsigned short CS2ReceiverPort = 15731;
			static const unsigned short CS2SenderPort = 15730;

			CS2Server() = delete;
			CS2Server(const CS2Server&) = delete;
			CS2Server& operator=(const CS2Server&) = delete;

			CS2Server(Manager& manager);
			~CS2Server();

			void Start() override;

			void Stop() override;

			void Work(Network::TcpConnection* connection) override;

			inline const std::string& GetName() const override
			{
				static const std::string CS2Name("CS2Server");
				return CS2Name;
			}

			void Booster(const ControlType controlType, const BoosterState status) override;

			void LocoBaseOrientation(const ControlType controlType,
				const DataModel::LocoBase* loco,
				const Orientation direction) override;

			void LocoBaseFunction(const ControlType controlType,
				const DataModel::LocoBase* loco,
				const DataModel::LocoFunctionNr function,
				const DataModel::LocoFunctionState on) override;

			void LocoBaseSpeed(const ControlType controlType,
				const DataModel::LocoBase* loco,
				const Speed speed) override;

			void AccessoryState(const ControlType controlType, const DataModel::Accessory* accessory) override;

			void SwitchState(const ControlType controlType, const DataModel::Switch* mySwitch) override;

			void SignalState(const ControlType controlType, const DataModel::Signal* signal) override;

		private:
			Logger::Logger* logger;
			Manager& manager;
			std::vector<CS2Client*> clients;
			unsigned int lastClientID;

			bool runUdp;
			std::thread udpServerThread;
			int udpServerSocket;

			void AccessoryBaseState(const DataModel::AccessoryBase* accessoryBase);

			void CleanUpClients();

			void StartUdpServer();

			void TerminateUdpServer();

			void UdpSocketCreateBindListen(int family, struct sockaddr* address);

			void UdpWorker();
	};
}} // namespace Server::CS2
