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

#include "DataModel/ObjectIdentifier.h"
#include "Hardware/Protocols/Z21.h"
#include "Manager.h"
#include "Server/Z21/Z21Client.h"

using DataModel::ObjectIdentifier;

namespace Z21Enums = Hardware::Protocols::Z21Enums;

namespace Server { namespace Z21
{
	Z21Client::Z21Client(const unsigned int id,
		Manager& manager,
		const int serverSocket,
		const struct sockaddr_storage* clientAddress)
	:	Network::UdpClient(serverSocket, clientAddress, 60),
		logger(Logger::Logger::GetLogger("Z21Client # " + std::to_string(id))),
		manager(manager),
		broadCastFlags(Z21Enums::BroadCastFlagNone)
	{
		logger->Debug(Languages::TextUdpConnectionEstablished, AddressAsString());
		SendSystemStatusChanged();
	}

	Z21Client::~Z21Client()
	{
		logger->Debug(Languages::TextUdpConnectionClosed, AddressAsString());
	}

	void Z21Client::Work(const unsigned char* buffer, const size_t dataLength)
	{
		logger->Hex(buffer, dataLength);

		size_t dataRead = 0;
		while (dataRead < dataLength)
		{
			size_t ret = ParseData(buffer + dataRead, dataLength - dataRead);
			if (ret == 0)
			{
				break;
			}
			dataRead += ret;
		}
	}

	size_t Z21Client::ParseData(const unsigned char* buffer, const size_t bufferLength)
	{
		unsigned short dataLength = Utils::Utils::DataLittleEndianToShort(buffer);
		if (dataLength < 4 || dataLength > bufferLength)
		{
			return 1472; // 1472 = Max UDP data size
		}

		const uint16_t header = Utils::Utils::DataLittleEndianToShort(buffer + 2);
		switch (header)
		{
			case Z21Enums::HeaderSerialNumber:
				SendSerialNumber();
				break;

			case Z21Enums::HeaderGetCode:
				SendCode();
				break;

			case Z21Enums::HeaderGetHardwareInfo:
				SendHardwareInfo();
				break;

			case Z21Enums::HeaderLogOff:
				Terminate();
				break;

			case Z21Enums::HeaderSeeXHeader:
				ParseXHeader(buffer);
				break;

			case Z21Enums::HeaderSetBroadcastFlags:
			{
				broadCastFlags = static_cast<Z21Enums::BroadCastFlags>(Utils::Utils::DataLittleEndianToInt(buffer + 4));
				break;
			}

			case Z21Enums::HeaderGetBroadcastFlags:
				SendBroadcastFlags();
				break;

			case Z21Enums::HeaderGetLocoMode:
			{
				const uint16_t address = *(reinterpret_cast<const uint16_t*>(buffer + 4));
				SendLocoMode(address);
				break;
			}

			case Z21Enums::HeaderSetLocoMode:
				// ignore loco mode, we always use DCC
				break;

			case Z21Enums::HeaderGetTurnoutMode:
			{
				const uint16_t address = *(reinterpret_cast<const uint16_t*>(buffer + 4));
				SendTurnoutMode(address);
				break;
			}

			case Z21Enums::HeaderSetTurnoutMode:
				// ignore turnout mode, we always use DCC
				break;

			case Z21Enums::HeaderGetSystemState:
				SendSystemStatusChanged();
				break;

			default:
				SendUnknownCommand();
				break;
		}
		return dataLength;
	}

	void Z21Client::ParseXHeader(const unsigned char* buffer)
	{
		Z21Enums::XHeader xHeader = static_cast<Z21Enums::XHeader>(buffer[4]);
		switch (xHeader)
		{
			case Z21Enums::XHeaderSeeDB0_1:
				ParseDB0(buffer);
				return;

			case Z21Enums::XHeaderSetStop:
				logger->Debug(Languages::TextBoosterIsTurnedOff);
				manager.Booster(ControlTypeZ21Server, BoosterStateStop);
				SendBcStopped();
				return;

			case Z21Enums::XHeaderGetLocoInfo:
				{
					const DataModel::Loco* const loco = manager.GetLoco(ParseLocoAddress(buffer + 6));
					if (nullptr == loco)
					{
						return;
					}
					SendLocoInfo(loco);
				}
				return;

			case Z21Enums::XHeaderLocoDrive:
				switch(buffer[5])
				{
					case Z21Enums::DB0SetLocoDrive14:
					case Z21Enums::DB0SetLocoDrive28:
					case Z21Enums::DB0SetLocoDrive128:
					case Z21Enums::DB0LocoFunction:
						ParseLocoDrive(buffer);
						return;

					default:
						return;
				}

			case Z21Enums::XHeaderSetLocoBinaryState:
				// we do not care
				return;

			case Z21Enums::XHeaderGetFirmwareVersion:
				SendFirmwareVersion();
				return;

			default:
				SendUnknownCommand();
				return;
		}
	}

	void Z21Client::ParseDB0(const unsigned char* buffer)
	{
		switch (buffer[5])
		{
			case Z21Enums::DB0Status:
			{
				SendStatusChanged();
				return;
			}

			case Z21Enums::DB0SetPowerOff:
				logger->Debug(Languages::TextBoosterIsTurnedOff);
				manager.Booster(ControlTypeZ21Server, BoosterStateStop);
				return;

			case Z21Enums::DB0SetPowerOn:
				logger->Debug(Languages::TextBoosterIsTurnedOn);
				manager.Booster(ControlTypeZ21Server, BoosterStateGo);
				return;

			default:
				SendUnknownCommand();
				return;
		}
	}

	void Z21Client::ParseLocoDrive(const unsigned char* buffer)
	{
		// FIXME: multiple units are not runnable
		const ObjectIdentifier locoBaseIdentifier(ObjectTypeLoco, ParseLocoAddress(buffer + 6));
		Speed speed;
		switch(buffer[5])
		{
			case Z21Enums::DB0SetLocoDrive14:
				speed = Hardware::Protocols::Z21::DecodeSpeed14(buffer[8] & 0x7F);
				break;

			case Z21Enums::DB0SetLocoDrive28:
				speed = Hardware::Protocols::Z21::DecodeSpeed28(buffer[8] & 0x7F);
				break;

			case Z21Enums::DB0SetLocoDrive128:
				speed = Hardware::Protocols::Z21::DecodeSpeed128(buffer[8] & 0x7F);
				break;

			case Z21Enums::DB0LocoFunction:
			{
				const DataModel::LocoFunctionNr nr = buffer[8] & 0x3F;
				const DataModel::LocoFunctionState on = static_cast<DataModel::LocoFunctionState>((buffer[8] >> 6) & 0x01);
				manager.LocoBaseFunctionState(ControlTypeZ21Server, locoBaseIdentifier, nr, on);
				return;
			}

			default:
				speed = 0;
				break;
		}
		manager.LocoBaseSpeed(ControlTypeZ21Server, locoBaseIdentifier, speed);
		manager.LocoBaseOrientation(ControlTypeZ21Server, locoBaseIdentifier, static_cast<Orientation>(buffer[8] >> 7));
	}

	void Z21Client::SendLocoInfo(const DataModel::LocoBase* const loco)
	{
		unsigned char sendBuffer[15] = { 0x0F, 0x00, 0x40, 0x00, 0xEF, 0x00, 0x00, 0x04 };
		const Address address = loco->GetID();
		Utils::Utils::ShortToDataBigEndian(address, sendBuffer + 5);
		sendBuffer[8] = (loco->GetOrientation() << 7) | Hardware::Protocols::Z21::EncodeSpeed128(loco->GetSpeed());
		sendBuffer[9] = ((loco->GetFunctionState(0) & 0x01) << 4)
				| ((loco->GetFunctionState(4) & 0x01) << 3)
				| ((loco->GetFunctionState(3) & 0x01) << 2)
				| ((loco->GetFunctionState(2) & 0x01) << 1)
				| (loco->GetFunctionState(1) & 0x01);
		sendBuffer[10] = ((loco->GetFunctionState(12) & 0x01) << 7)
				| ((loco->GetFunctionState(11) & 0x01) << 6)
				| ((loco->GetFunctionState(10) & 0x01) << 5)
				| ((loco->GetFunctionState(9) & 0x01) << 4)
				| ((loco->GetFunctionState(8) & 0x01) << 3)
				| ((loco->GetFunctionState(7) & 0x01) << 2)
				| ((loco->GetFunctionState(6) & 0x01) << 1)
				| (loco->GetFunctionState(5) & 0x01);
		sendBuffer[11] = ((loco->GetFunctionState(20) & 0x01) << 7)
				| ((loco->GetFunctionState(19) & 0x01) << 6)
				| ((loco->GetFunctionState(18) & 0x01) << 5)
				| ((loco->GetFunctionState(17) & 0x01) << 4)
				| ((loco->GetFunctionState(16) & 0x01) << 3)
				| ((loco->GetFunctionState(15) & 0x01) << 2)
				| ((loco->GetFunctionState(14) & 0x01) << 1)
				| (loco->GetFunctionState(12) & 0x01);
		sendBuffer[12] = ((loco->GetFunctionState(28) & 0x01) << 7)
				| ((loco->GetFunctionState(27) & 0x01) << 6)
				| ((loco->GetFunctionState(26) & 0x01) << 5)
				| ((loco->GetFunctionState(25) & 0x01) << 4)
				| ((loco->GetFunctionState(24) & 0x01) << 3)
				| ((loco->GetFunctionState(23) & 0x01) << 2)
				| ((loco->GetFunctionState(22) & 0x01) << 1)
				| (loco->GetFunctionState(21) & 0x01);
		sendBuffer[13] = ((loco->GetFunctionState(31) & 0x01) << 2)
				| ((loco->GetFunctionState(30) & 0x01) << 1)
				| (loco->GetFunctionState(29) & 0x01);
		sendBuffer[sizeof(sendBuffer) - 1] = Utils::Utils::CalcXORCheckSum(sendBuffer, sizeof(sendBuffer) - 1);
		logger->Debug("Sending LocoInfo of address {0}", address);
		Send(sendBuffer, sizeof(sendBuffer));
	}
}} // namespace Server::Z21
