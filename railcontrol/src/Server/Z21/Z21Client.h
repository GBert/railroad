/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2024 by Teddy / Dominik Mahrer - www.railcontrol.org

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

#include <ctime>

#include "Hardware/Protocols/Z21DataTypes.h"
#include "Network/UdpClient.h"
#include "Utils/Integer.h"

namespace Z21Enums = Hardware::Protocols::Z21Enums;

namespace Server { namespace Z21
{
	class Z21Server;

	class Z21Client : protected Network::UdpClient
	{
		public:
			Z21Client() = delete;
			Z21Client(const Z21Client&) = delete;
			Z21Client& operator=(const Z21Client&) = delete;

			Z21Client(const unsigned int id,
				Manager& manager,
				const int serverSocket,
				const struct sockaddr_storage* clientAddress);

			~Z21Client();

			void Work(const unsigned char* buffer, const size_t size) override;

			size_t ParseData(const unsigned char* buffer, const size_t bufferLength);

			void ParseXHeader(const unsigned char* buffer);

			void ParseDB0(const unsigned char* buffer);

			void ParseLocoDrive(const unsigned char* buffer);

			inline uint16_t ParseLocoAddress(const unsigned char* buffer)
			{
				return ((buffer[0] & 0x3F) << 8) + buffer[1];
			}

			inline uint16_t ParseAccessoryAddress(const unsigned char* buffer)
			{
				return ((buffer[0] << 8) + buffer[1]) + 1; // + 1 because Z21 protocol is 0 based
			}

			inline void SendPowerOff()
			{
				const unsigned char sendBuffer[7] = { 0x07, 0x00, 0x40, 0x00, 0x61, 0x00, 0x61 };
				Send(sendBuffer, sizeof(sendBuffer));
			}

			inline void SendPowerOn()
			{
				const unsigned char sendBuffer[7] = { 0x07, 0x00, 0x40, 0x00, 0x61, 0x01, 0x60 };
				Send(sendBuffer, sizeof(sendBuffer));
			}

			void SendLocoInfo(const DataModel::LocoBase* const locoBase);

			void SendTurnoutInfo(const DataModel::AccessoryBase* const accessoryBase);

		private:
			inline void SendSerialNumber()
			{
				const unsigned char sendBuffer[8] = { 0x08, 0x00, 0x10, 0x00, 0x12, 0x34, 0x56, 0x78 };
				Send(sendBuffer, sizeof(sendBuffer));
			}

			inline void SendCode()
			{
				const unsigned char sendBuffer[5] = { 0x05, 0x00, 0x18, 0x00, 0x00 };
				Send(sendBuffer, sizeof(sendBuffer));
			}

			inline void SendHardwareInfo()
			{
				const unsigned char sendBuffer[12] = { 0x0C, 0x00, 0x1A, 0x00, 0x01, 0x02, 0x00, 0x00, 0x42, 0x01, 0x00, 0x00 };
				Send(sendBuffer, sizeof(sendBuffer));
			}

			inline void SendSystemStatusChanged()
			{
				const BoosterState booster = manager.Booster();
				const unsigned char centralState = (!booster) << 1;
				const unsigned char sendBuffer[20] = { 0x14, 0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x20, 0x4E, 0x50, 0x46, centralState, 0x00, 0x00, 0x71 };
				Send(sendBuffer, sizeof(sendBuffer));
			}

			inline void SendBcStopped()
			{
				const unsigned char sendBuffer[7] = { 0x07, 0x00, 0x40, 0x00, 0x81, 0x00, 0x81 };
				Send(sendBuffer, sizeof(sendBuffer));
			}

			inline void SendFirmwareVersion()
			{
				const unsigned char sendBuffer[9] = { 0x09, 0x00, 0x40, 0x00, 0xF3, 0x0A, 0x01, 0x43, 0xBB };
				Send(sendBuffer, sizeof(sendBuffer));
			}

			inline void SendUnknownCommand()
			{
				const unsigned char sendBuffer[7] = { 0x07, 0x00, 0x40, 0x00, 0x61, 0x82, 0xE3 };
				Send(sendBuffer, sizeof(sendBuffer));
			}

			inline void SendStatusChanged()
			{
				const BoosterState booster = manager.Booster();
				const unsigned char centralState = (!booster) << 1;
				unsigned char sendBuffer[8] = { 0x08, 0x00, 0x40, 0x00, 0x62, 0x22, centralState };
				sendBuffer[sizeof(sendBuffer) - 1] = Utils::Utils::CalcXORCheckSum(sendBuffer + 4, sizeof(sendBuffer) - 5);
				Send(sendBuffer, sizeof(sendBuffer));
			}

			inline void SendVersion()
			{
				unsigned char sendBuffer[9] = { 0x09, 0x00, 0x40, 0x00, 0x63, 0x21, 0x40, 0x12 };
				sendBuffer[sizeof(sendBuffer) - 1] = Utils::Utils::CalcXORCheckSum(sendBuffer + 4, sizeof(sendBuffer) - 5);
				Send(sendBuffer, sizeof(sendBuffer));
			}

			inline void SendBroadcastFlags()
			{
				unsigned char sendBuffer[8] = { 0x08, 0x00, 0x51, 0x00 };
				Utils::Integer::IntToDataLittleEndian(broadCastFlags, sendBuffer + 4);
				Send(sendBuffer, sizeof(sendBuffer));
			}

			inline void SendLocoMode(const uint16_t address)
			{
				unsigned char sendBuffer[7] = { 0x07, 0x00, 0x60, 0x00 };
				Utils::Integer::ShortToDataBigEndian(address, sendBuffer + 4);
				sendBuffer[6] = 0x00; // we always use DCC
				Send(sendBuffer, sizeof(sendBuffer));
			}

			inline void SendTurnoutMode(const uint16_t address)
			{
				unsigned char sendBuffer[7] = { 0x07, 0x00, 0x70, 0x00 };
				Utils::Integer::ShortToDataBigEndian(address, sendBuffer + 4);
				sendBuffer[6] = 0x00; // we always use DCC
				Send(sendBuffer, sizeof(sendBuffer));
			}

			inline void Send(const unsigned char* buffer, const size_t size)
			{
				logger->Hex(buffer, size);
				UdpClient::Send(buffer, size);
			}

			Logger::Logger* logger;

			Manager& manager;
			Z21Enums::BroadCastFlags broadCastFlags;
	};
}} // namespace Server::Z21

