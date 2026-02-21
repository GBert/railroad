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

#include <map>
#include <mutex>
#include <sstream>
#include <vector>

#include "ControlInterface.h"
#include "Hardware/Protocols/Z21LocoCache.h"
#include "Logger/Logger.h"
#include "Manager.h"
#include "Network/UdpServer.h"

namespace Server { namespace Z21
{
	class Z21Client;

	class Z21Server : public ControlInterface, private Network::UdpServer
	{
		public:
			static const unsigned short Z21Port = 21105;

			Z21Server() = delete;
			Z21Server(const Z21Server&) = delete;
			Z21Server& operator=(const Z21Server&) = delete;

			Z21Server(Manager& manager, const unsigned short port);
			~Z21Server();

			void Start() override;

			void Stop() override;

			bool NextUpdate(unsigned int& updateIDClient, std::string& s);

			inline const std::string& GetName() const override
			{
				static const std::string Z21Name("Z21Server");
				return Z21Name;
			}

			void Booster(const ControlType controlType, const BoosterState status) override;

			void LocoBaseSpeed(const ControlType controlType,
				const ControlID controlID,
				const LocoID locoID,
				const LocoType locoType,
				const Protocol protocol,
				const Address address,
				const Address serverAddress,
				const std::string& name,
				const Speed speed) override;

			void LocoBaseOrientation(const ControlType controlType,
				const ControlID controlID,
				const LocoID locoID,
				const LocoType locoType,
				const Protocol protocol,
				const Address address,
				const Address serverAddress,
				const std::string& name,
				const Orientation orientation) override;

			void LocoBaseFunctionState(const ControlType controlType,
				const ControlID controlID,
				const LocoID locoID,
				const LocoType locoType,
				const Protocol protocol,
				const Address address,
				const Address serverAddress,
				const std::string& name,
				const DataModel::LocoFunctionNr function,
				const DataModel::LocoFunctionState state) override;

			void LocoBaseSpeedOrientationFunctionStates(const ControlID controlID,
				const LocoID locoID,
				const LocoType locoType,
				const Protocol protocol,
				const Address address,
				const Address serverAddress,
				const std::string& name,
				const Speed speed,
				const Orientation orientation,
				const std::vector<DataModel::LocoFunctionEntry>& functions) override;

			void AccessoryState(const ControlType controlType, const DataModel::Accessory* accessory) override;

			void SwitchState(const ControlType controlType, const DataModel::Switch* mySwitch) override;

			void SignalState(const ControlType controlType, const DataModel::Signal* signal) override;

			inline Hardware::Protocols::Z21LocoCacheEntry GetLocoCacheEntry(const Address serverAddress)
			{
				return locoCache.GetData(serverAddress);
			}

		protected:
			Network::UdpClient* UdpClientFactory(const int serverSocket,
				const struct sockaddr_storage* clientAddress) override;

		private:
			Logger::Logger* logger;
			Manager& manager;
			unsigned int lastClientID;
			Hardware::Protocols::Z21LocoCache locoCache;

			ssize_t ParseData(const unsigned char* buffer,
				const ssize_t bufferLength,
				const struct sockaddr_storage* clientAddress);

			void SendLocoInfo(const Hardware::Protocols::Z21LocoCacheEntry& locoCache,
				const Address serverAddress);

			void ParseXHeader(const unsigned char* buffer);

			void ParseDB0(const unsigned char* buffer);

			void AccessoryBaseState(const DataModel::AccessoryBase* accessoryBase);
	};
}} // namespace Server::Z21
