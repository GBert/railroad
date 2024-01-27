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

#include <cstring>		//memset

#include "DataModel/LocoBase.h"
#include "DataModel/LocoFunctions.h"
#include "Manager.h"
#include "Server/Z21/Z21Client.h"
#include "Server/Z21/Z21Server.h"
#include "Utils/Network.h"

using DataModel::Loco;
using DataModel::LocoBase;
using DataModel::LocoFunctionNr;
using DataModel::LocoFunctionState;

//namespace Z21Enums = Hardware::Protocols::Z21Enums;

namespace Network
{
	class UdpClient;
}

namespace Server { namespace Z21
{
	Z21Server::Z21Server(Manager& manager,
		const unsigned short port)
	:	ControlInterface(ControlTypeZ21Server),
		Network::UdpServer("0.0.0.0", port, "Z21Server"),
		logger(Logger::Logger::GetLogger("Z21Server")),
		manager(manager),
		lastClientID(0)
	{
	}

	Z21Server::~Z21Server()
	{
		logger->Info(Languages::TextZ21ServerStopped);
	}

	void Z21Server::Start()
	{
		StartUdpServer();
		logger->Info(Languages::TextZ21ServerStarted);
	}

	void Z21Server::Stop()
	{
		TerminateUdpServer();
	}

	Network::UdpClient* Z21Server::UdpClientFactory(const int serverSocket,
		const struct sockaddr_storage* clientAddress)
	{
		return reinterpret_cast<Network::UdpClient*>(new Z21Client(++lastClientID, manager, serverSocket, clientAddress));
	}

	void Z21Server::Booster(__attribute__((unused)) const ControlType controlType,
		const BoosterState status)
	{
		if (status == BoosterStateGo)
		{
			for (auto client : clients)
			{
				reinterpret_cast<Z21Client*>(client)->SendPowerOn();
			}
		}
		else
		{
			for (auto client : clients)
			{
				reinterpret_cast<Z21Client*>(client)->SendPowerOff();
			}
		}
	}

	void Z21Server::LocoBaseSpeed(__attribute__((unused)) const ControlType controlType,
		const LocoBase* locoBase,
		__attribute__((unused)) const Speed speed)
	{
		const Loco* loco = dynamic_cast<const Loco*>(locoBase);
		if (nullptr == loco)
		{
			return;
		}
		for (auto client : clients)
		{
			reinterpret_cast<Z21Client*>(client)->SendLocoInfo(loco);
		}
	}

	void Z21Server::LocoBaseOrientation(__attribute__((unused)) const ControlType controlType,
		const LocoBase* locoBase,
		__attribute__((unused)) const Orientation orientation)
	{
		const Loco* loco = dynamic_cast<const Loco*>(locoBase);
		if (nullptr == loco)
		{
			return;
		}
		for (auto client : clients)
		{
			reinterpret_cast<Z21Client*>(client)->SendLocoInfo(loco);
		}
	}

	void Z21Server::LocoBaseFunction(__attribute__((unused)) const ControlType controlType,
		const LocoBase* locoBase,
		__attribute__((unused)) const LocoFunctionNr function,
		__attribute__((unused)) const LocoFunctionState state)
	{
		const Loco* loco = dynamic_cast<const Loco*>(locoBase);
		if (nullptr == loco)
		{
			return;
		}
		for (auto client : clients)
		{
			reinterpret_cast<Z21Client*>(client)->SendLocoInfo(loco);
		}
	}

	void Z21Server::AccessoryState(__attribute__((unused)) const ControlType controlType,
		__attribute__((unused)) const DataModel::Accessory* accessory)
	{
	}

	void Z21Server::SwitchState(__attribute__((unused)) const ControlType controlType,
		__attribute__((unused)) const DataModel::Switch* mySwitch)
	{
	}

	void Z21Server::SignalState(__attribute__((unused)) const ControlType controlType,
		__attribute__((unused)) const DataModel::Signal* signal)
	{
	}
}} // namespace Server::Z21
