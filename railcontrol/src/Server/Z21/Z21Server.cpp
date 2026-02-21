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

#include "DataModel/LocoBase.h"
#include "DataModel/LocoConfig.h"
#include "DataModel/LocoFunctions.h"
#include "Manager.h"
#include "Server/Z21/Z21Client.h"
#include "Server/Z21/Z21Server.h"
#include "Utils/Network.h"

using DataModel::Loco;
using DataModel::LocoBase;
using DataModel::LocoConfig;
using DataModel::LocoFunctionNr;
using DataModel::LocoFunctionState;
using Hardware::Protocols::Z21LocoCacheEntry;

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
		return reinterpret_cast<Network::UdpClient*>(new Z21Client(++lastClientID, manager, *this, serverSocket, clientAddress));
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
		__attribute__((unused)) const ControlID controlID,
		__attribute__((unused)) const LocoID locoID,
		__attribute__((unused)) const LocoType locoType,
		__attribute__((unused)) const Protocol protocol,
		__attribute__((unused)) const Address address,
		const Address serverAddress,
		__attribute__((unused)) const std::string& name,
		const Speed speed)
	{
		locoCache.SetSpeed(serverAddress, speed);
		SendLocoInfo(locoCache.GetData(serverAddress), serverAddress);
	}

	void Z21Server::LocoBaseOrientation(__attribute__((unused)) const ControlType controlType,
		__attribute__((unused)) const ControlID controlID,
		__attribute__((unused)) const LocoID locoID,
		__attribute__((unused)) const LocoType locoType,
		__attribute__((unused)) const Protocol protocol,
		__attribute__((unused)) const Address address,
		const Address serverAddress,
		__attribute__((unused)) const std::string& name,
		const Orientation orientation)
	{
		locoCache.SetOrientation(serverAddress, orientation);
		SendLocoInfo(locoCache.GetData(serverAddress), serverAddress);
	}

	void Z21Server::LocoBaseFunctionState(__attribute__((unused)) const ControlType controlType,
		__attribute__((unused)) const ControlID controlID,
		__attribute__((unused)) const LocoID locoID,
		__attribute__((unused)) const LocoType locoType,
		__attribute__((unused)) const Protocol protocol,
		__attribute__((unused)) const Address address,
		const Address serverAddress,
		__attribute__((unused)) const std::string& name,
		const DataModel::LocoFunctionNr function,
		const DataModel::LocoFunctionState state)
	{
		locoCache.SetFunction(serverAddress, function, state != DataModel::LocoFunctionStateOff);
		SendLocoInfo(locoCache.GetData(serverAddress), serverAddress);
	}

	void Z21Server::LocoBaseSpeedOrientationFunctionStates(__attribute__((unused)) const ControlID controlID,
		__attribute__((unused)) const LocoID locoID,
		__attribute__((unused)) const LocoType locoType,
		__attribute__((unused)) const Protocol protocol,
		__attribute__((unused)) const Address address,
		const Address serverAddress,
		__attribute__((unused)) const std::string& name,
		const Speed speed,
		const Orientation orientation,
		const std::vector<DataModel::LocoFunctionEntry>& functions)
	{
		locoCache.SetSpeed(serverAddress, speed);
		locoCache.SetOrientation(serverAddress, orientation);

		for (auto& function : functions)
		{
			locoCache.SetFunction(serverAddress, function.nr, function.state != DataModel::LocoFunctionStateOff);
		}
		SendLocoInfo(locoCache.GetData(serverAddress), serverAddress);
	}


	void Z21Server::SendLocoInfo(const Z21LocoCacheEntry& locoCache,
		const Address serverAddress)
	{
		for (auto client : clients)
		{
			reinterpret_cast<Z21Client*>(client)->SendLocoInfo(locoCache, serverAddress);
		}
	}

	void Z21Server::AccessoryState(__attribute__((unused)) const ControlType controlType,
		const DataModel::Accessory* accessory)
	{
		AccessoryBaseState(accessory);
	}

	void Z21Server::SwitchState(__attribute__((unused)) const ControlType controlType,
		const DataModel::Switch* mySwitch)
	{
		AccessoryBaseState(mySwitch);
	}

	void Z21Server::SignalState(__attribute__((unused)) const ControlType controlType,
		const DataModel::Signal* signal)
	{
		AccessoryBaseState(signal);
	}

	void Z21Server::AccessoryBaseState(const DataModel::AccessoryBase* accessoryBase)
	{
		if (!accessoryBase)
		{
			return;
		}
		for (auto client : clients)
		{
			reinterpret_cast<Z21Client*>(client)->SendTurnoutInfo(accessoryBase);
		}
	}
}} // namespace Server::Z21
