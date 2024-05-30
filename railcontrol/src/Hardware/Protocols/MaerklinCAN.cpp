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

#include <deque>

#include "Hardware/Protocols/MaerklinCAN.h"
#include "Hardware/ZLib.h"

using std::deque;
using std::string;
using std::vector;

namespace Hardware
{
	namespace Protocols
	{
		void MaerklinCAN::Init()
		{
			receiverThread = std::thread(&MaerklinCAN::Receiver, this);
			pingThread = std::thread(&MaerklinCAN::PingSender, this);
		}

		MaerklinCAN::~MaerklinCAN()
		{
			run = false;
			receiverThread.join();
			pingThread.join();
			if (canFileData != nullptr)
			{
				free(canFileData);
				canFileData = nullptr;
			}
		}

		void MaerklinCAN::Wait(const unsigned int duration) const
		{
			unsigned int wait = duration;
			while (run && !hasCs2Master && wait)
			{
				Utils::Utils::SleepForSeconds(1);
				--wait;
			}
		}

		void MaerklinCAN::PingSender()
		{
			Utils::Utils::SetThreadName("Maerklin CAN Ping");
			logger->Info(Languages::TextPingSenderStarted);

			Wait(1);

			while (run && !hasCs2Master)
			{
				Ping();
				Wait(10);
			}
			if (run && hasCs2Master)
			{
				RequestLoks();
			}
			logger->Info(Languages::TextTerminatingPingSender);
		}

		void MaerklinCAN::CreateCommandHeader(unsigned char* const buffer, const CanCommand command,
			const CanResponse response, const CanLength length)
		{
			const CanPrio prio = 0;
			buffer[0] = (prio << 1) | (command >> 7);
			buffer[1] = (command << 1) | (response & 0x01);
			Utils::Integer::ShortToDataBigEndian(hash, buffer + 2);
			buffer[4] = length;
			buffer[5] = 0;
			buffer[6] = 0;
			buffer[7] = 0;
			buffer[8] = 0;
			buffer[9] = 0;
			buffer[10] = 0;
			buffer[11] = 0;
			buffer[12] = 0;
		}

		void MaerklinCAN::ParseAddressProtocol(const Address input,
			Address& address,
			Protocol& protocol,
			LocoType& type)
		{
			address = input;
			Address maskedAddress = address & 0xFC00;
			type = LocoTypeLoco;

			if ((maskedAddress == 0x0000)     // MM Loco
				|| (maskedAddress == 0x1000)  // MM Loco (unused by CS2)
				|| (maskedAddress == 0x2000)  // MM Accessory (unused by CS2)
				|| (maskedAddress == 0x3000)) // MM Accessory
			{
				protocol = ProtocolMM;
				address &= 0x03FF;
				return;
			}

			// 0x0800 is SX1 Loco (unused by CS2)
			// 0x2800 is SX1 Accessory (unused by CS2)

			if (maskedAddress == 0x2C00) // Multiple Unit
			{
				protocol = ProtocolMultipleUnit;
				type = LocoTypeMultipleUnit;
				address &= 0x03FF;
				return;
			}

			if ((maskedAddress == 0x3800)     // DCC Accessory
				|| (maskedAddress == 0x3C00)) // DCC Accessory
			{
				protocol = ProtocolDCC;
				address &= 0x03FF;
				return;
			}

			maskedAddress = address & 0xC000;
			address &= 0x3FFF;
			if (maskedAddress == 0x4000) // MFX Loco
			{
				protocol = ProtocolMFX;
				return;
			}
			if (maskedAddress == 0xC000) // DCC Loco
			{
				protocol = ProtocolDCC;
				return;
			}

			protocol = ProtocolNone;
			address = 0;
		}

		MaerklinCAN::CanHash MaerklinCAN::CalcHash(const CanUid uid)
		{
			CanHash calc = (uid >> 16) ^ (uid & 0xFFFF);
			CanHash hash = ((calc << 3) | 0x0300) & 0xFF00;
			hash |= (calc & 0x007F);
			return hash;
		}

		void MaerklinCAN::GenerateUidHash()
		{
			uid = rand();
			string uidString = Utils::Integer::IntegerToHex(uid);
			// FIXME: params->SetArg5(uidString);
			hash = CalcHash(uid);
			logger->Info(Languages::TextMyUidHash, uidString, Utils::Integer::IntegerToHex(hash));
		}

		void MaerklinCAN::CreateLocalIDLoco(unsigned char* buffer,
			const Protocol protocol,
			const Address address)
		{
			uint32_t localID = address;
			if (protocol == ProtocolDCC)
			{
				localID |= 0xC000;
			}
			else if (protocol == ProtocolMFX)
			{
				localID |= 0x4000;
			}
			else if (protocol == ProtocolMultipleUnit)
			{
				localID |= 0x2000;
			}
			// else expect PROTOCOL_MM2: do nothing
			Utils::Integer::IntToDataBigEndian(localID, buffer + 5);
		}

		void MaerklinCAN::CreateLocalIDAccessory(unsigned char* buffer,
			const Protocol protocol,
			const Address address)
		{
			uint32_t localID = address - 1; // GUI-address is 1-based, protocol-address is 0-based
			if (protocol == ProtocolDCC)
			{
				localID |= 0x3800;
			}
			else
			{
				localID |= 0x3000;
			}
			Utils::Integer::IntToDataBigEndian(localID, buffer + 5);
		}

		void MaerklinCAN::Booster(const BoosterState status)
		{
			unsigned char buffer[CANCommandBufferLength];
			logger->Info(status ? Languages::TextTurningBoosterOn : Languages::TextTurningBoosterOff);
			CreateCommandHeader(buffer, CanCommandSystem, CanResponseCommand, 5);
			buffer[9] = status;
			SendInternal(buffer);
		}

		void MaerklinCAN::LocoSpeed(const Protocol protocol, const Address address, const Speed speed)
		{
			unsigned char buffer[CANCommandBufferLength];
			logger->Info(Languages::TextSettingSpeedWithProtocol, Utils::Utils::ProtocolToString(protocol), address, speed);
			CreateCommandHeader(buffer, CanCommandLocoSpeed, CanResponseCommand, 6);
			CreateLocalIDLoco(buffer, protocol, address);
			Utils::Integer::ShortToDataBigEndian(speed, buffer + 9);
			SendInternal(buffer);
		}

		void MaerklinCAN::LocoOrientation(const Protocol protocol, const Address address,
			const Orientation orientation)
		{
			unsigned char buffer[CANCommandBufferLength];
			logger->Info(Languages::TextSettingDirectionOfTravelWithProtocol, Utils::Utils::ProtocolToString(protocol), address, Languages::GetLeftRight(orientation));
			CreateCommandHeader(buffer, CanCommandLocoDirection, CanResponseCommand, 5);
			CreateLocalIDLoco(buffer, protocol, address);
			buffer[9] = (orientation ? 1 : 2);
			SendInternal(buffer);
		}

		void MaerklinCAN::LocoFunction(const Protocol protocol,
			const Address address,
			const DataModel::LocoFunctionNr function,
			const DataModel::LocoFunctionState on)
		{
			unsigned char buffer[CANCommandBufferLength];
			logger->Info(Languages::TextSettingFunctionWithProtocol, static_cast<int>(function), Utils::Utils::ProtocolToString(protocol), address, Languages::GetOnOff(on));
			CreateCommandHeader(buffer, CanCommandLocoFunction, CanResponseCommand, 6);
			CreateLocalIDLoco(buffer, protocol, address);
			buffer[9] = function;
			buffer[10] = (on == DataModel::LocoFunctionStateOn);
			SendInternal(buffer);
		}

		void MaerklinCAN::AccessoryOnOrOff(const Protocol protocol, const Address address,
			const DataModel::AccessoryState state, const bool on)
		{
			unsigned char buffer[CANCommandBufferLength];
			logger->Info(Languages::TextSettingAccessoryWithProtocol, Utils::Utils::ProtocolToString(protocol), address, Languages::GetGreenRed(state), Languages::GetOnOff(on));
			CreateCommandHeader(buffer, CanCommandAccessory, CanResponseCommand, 6);
			CreateLocalIDAccessory(buffer, protocol, address);
			buffer[9] = state & 0x03;
			buffer[10] = static_cast<unsigned char>(on);
			SendInternal(buffer);
		}

		void MaerklinCAN::ProgramRead(const ProgramMode mode, const Address address, const CvNumber cv)
		{
			Address addressInternal = address;
			Protocol protocol = ProtocolNone;
			switch (mode)
			{
				case ProgramModeDccDirect:
					logger->Info(Languages::TextProgramDccDirectRead, cv);
					protocol = ProtocolDCC;
					break;

				case ProgramModeMfx:
					logger->Info(Languages::TextProgramMfxRead, address, cv);
					protocol = ProtocolMFX;
					break;

				default:
					return;
			}
			unsigned char buffer[CANCommandBufferLength];
			CreateCommandHeader(buffer, CanCommandReadConfig, CanResponseCommand, 7);
			CreateLocalIDLoco(buffer, protocol, addressInternal);
			Utils::Integer::ShortToDataBigEndian(cv, buffer + 9);
			buffer[11] = 1;
			SendInternal(buffer);
		}

		void MaerklinCAN::ProgramWrite(const ProgramMode mode, const Address address, const CvNumber cv,
			const CvValue value)
		{
			Address addressInternal = address;
			Protocol protocol = ProtocolNone;
			unsigned char controlFlags = 0;
			switch (mode)
			{
				case ProgramModeMm:
					logger->Info(Languages::TextProgramMm, cv, value);
					protocol = ProtocolMM;
					addressInternal = 80;
					break;

				case ProgramModeMmPom:
					logger->Info(Languages::TextProgramMmPom, address, cv, value);
					protocol = ProtocolMM;
					controlFlags = 1 << 7;
					break;

				case ProgramModeDccDirect:
					logger->Info(Languages::TextProgramDccDirectWrite, cv, value);
					protocol = ProtocolDCC;
					addressInternal = 0;
					break;

				case ProgramModeDccPomLoco:
					logger->Info(Languages::TextProgramDccPomLocoWrite, address, cv, value);
					protocol = ProtocolDCC;
					controlFlags = 1 << 7;
					break;

				case ProgramModeMfx:
					logger->Info(Languages::TextProgramMfxWrite, address, cv, value);
					protocol = ProtocolMFX;
					controlFlags = 1 << 7;
					break;

				default:
					return;
			}
			unsigned char buffer[CANCommandBufferLength];
			CreateCommandHeader(buffer, CanCommandWriteConfig, CanResponseCommand, 8);
			CreateLocalIDLoco(buffer, protocol, addressInternal);
			Utils::Integer::ShortToDataBigEndian(cv, buffer + 9);
			buffer[11] = value;
			buffer[12] = controlFlags;
			SendInternal(buffer);
		}

		void MaerklinCAN::Ping()
		{
			unsigned char buffer[CANCommandBufferLength];
			CreateCommandHeader(buffer, CanCommandPing, CanResponseCommand, 0);
			SendInternal(buffer);
		}

		void MaerklinCAN::RequestLoks()
		{
			unsigned char buffer[CANCommandBufferLength];
			CreateCommandHeader(buffer, CanCommandRequestConfigData, CanResponseCommand, 8);
			buffer[5] = 'l';
			buffer[6] = 'o';
			buffer[7] = 'k';
			buffer[8] = 's';
			buffer[9] = 0;
			buffer[10] = 0;
			buffer[11] = 0;
			buffer[12] = 0;
			SendInternal(buffer);
		}

		void MaerklinCAN::Parse(const unsigned char* buffer)
		{
			CanResponse response = ParseResponse(buffer);
			CanCommand command = ParseCommand(buffer);
			CanLength length = ParseLength(buffer);
			logger->Hex(buffer, 5 + length);
			const CanHash receivedHash = ParseHash(buffer);
			if (receivedHash == hash)
			{
				uint16_t deviceType = Utils::Integer::DataBigEndianToShort(buffer + 11);
				if (command == CanCommandPing && response == true)
				{
					if (deviceType == CanDeviceCs2Master)
					{
						hasCs2Master = true;
						logger->Info(Languages::TextCs2MasterFound);
					}
				}
				else if (command != CanCommandConfigData)
				{
					GenerateUidHash();
				}
			}

			if (response)
			{
				switch (command)
				{
					case CanCommandS88Event:
						ParseResponseS88Event(buffer);
						return;

					case CanCommandReadConfig:
						ParseResponseReadConfig(buffer);
						return;

					case CanCommandPing:
						ParseResponsePing(buffer);
						return;

					case CanCommandLocoSpeed:
						ParseResponseLocoSpeed(buffer);
						return;

					case CanCommandLocoDirection:
						ParseResponseLocoDirection(buffer);
						return;

					case CanCommandLocoFunction:
						ParseResponseLocoFunction(buffer);
						return;

					case CanCommandAccessory:
						ParseResponseAccessory(buffer);
						return;

					default:
						return;
				}
			}

			switch (command)
			{
				case CanCommandSystem:
					ParseCommandSystem(buffer);
					return;

				case CanCommandConfigData:
					ParseCommandConfigData(buffer);
					return;

				case CanCommandPing:
					ParseCommandPing(buffer);
					return;

				default:
					return;
			}
		}

		void MaerklinCAN::ParseCommandSystem(const unsigned char* const buffer)
		{
			if (ParseLength(buffer) != 5)
			{
				return;
			}
			CanSubCommand subcmd = ParseSubCommand(buffer);
			switch (subcmd)
			{
				case CanSubCommandStop:
					// system stop
					manager->Booster(ControlTypeHardware, BoosterStateStop);
					return;

				case CanSubCommandGo:
					// system go
					manager->Booster(ControlTypeHardware, BoosterStateGo);
					return;
			}
		}

		void MaerklinCAN::ParseResponseLocoSpeed(const unsigned char* const buffer)
		{
			if (ParseLength(buffer) != 6)
			{
				return;
			}
			Address address;
			Protocol protocol;
			LocoType type;
			ParseAddressProtocol(buffer, address, protocol, type);
			Speed speed = Utils::Integer::DataBigEndianToShort(buffer + 9);
			logger->Info(Languages::TextReceivedSpeedCommand, Utils::Utils::ProtocolToString(protocol), address, speed);
			manager->LocoSpeed(ControlTypeHardware, controlID, protocol, address, speed);
		}

		void MaerklinCAN::ParseResponseLocoDirection(const unsigned char* const buffer)
		{
			if (ParseLength(buffer) != 5)
			{
				return;
			}
			Address address;
			Protocol protocol;
			LocoType type;
			ParseAddressProtocol(buffer, address, protocol, type);
			Orientation orientation = (buffer[9] == 1 ? OrientationRight : OrientationLeft);
			logger->Info(Languages::TextReceivedDirectionCommand, Utils::Utils::ProtocolToString(protocol), address, orientation);
			// changing direction implies speed = 0
			manager->LocoSpeed(ControlTypeHardware, controlID, protocol, address, MinSpeed);
			manager->LocoOrientation(ControlTypeHardware, controlID, protocol, address, orientation);
		}

		void MaerklinCAN::ParseResponseLocoFunction(const unsigned char* const buffer)
		{
			if (ParseLength(buffer) != 6)
			{
				return;
			}
			Address address;
			Protocol protocol;
			LocoType type;
			ParseAddressProtocol(buffer, address, protocol, type);
			DataModel::LocoFunctionNr function = buffer[9];
			DataModel::LocoFunctionState on = (buffer[10] != 0 ? DataModel::LocoFunctionStateOn : DataModel::LocoFunctionStateOff);
			logger->Info(Languages::TextReceivedFunctionCommand, Utils::Utils::ProtocolToString(protocol), address, function, on);
			manager->LocoFunctionState(ControlTypeHardware, controlID, protocol, address, function, on);
		}

		void MaerklinCAN::ParseResponseAccessory(const unsigned char* const buffer)
		{
			if (ParseLength(buffer) != 6 || buffer[10] != 1)
			{
				return;
			}
			Address address;
			Protocol protocol;
			LocoType type;
			ParseAddressProtocol(buffer, address, protocol, type);
			DataModel::AccessoryState state = (buffer[9] ? DataModel::AccessoryStateOn : DataModel::AccessoryStateOff);
			// GUI-address is 1-based, protocol-address is 0-based
			++address;
			logger->Info(Languages::TextReceivedAccessoryCommand, Utils::Utils::ProtocolToString(protocol), address, state);
			manager->AccessoryBaseState(ControlTypeHardware, controlID, protocol, address, state);
		}

		void MaerklinCAN::ParseCommandPing(const unsigned char* const buffer)
		{
			if (buffer[4] == 8 && Utils::Integer::DataBigEndianToInt(buffer + 5) != uid)
			{
				return;
			}
			unsigned char sendBuffer[CANCommandBufferLength];
			CreateCommandHeader(sendBuffer, CanCommandPing, CanResponseResponse, 8);
			Utils::Integer::IntToDataBigEndian(uid, sendBuffer + 5);
			// version 3.8
			sendBuffer[9] = 3;
			sendBuffer[10] = 8;
			// device type CS2 Slave
			sendBuffer[11] = 0xff;
			sendBuffer[12] = 0xf0;
			SendInternal(sendBuffer);
		}

		void MaerklinCAN::ParseCommandConfigData(const unsigned char* const buffer)
		{
			CanLength length = ParseLength(buffer);
			switch (length)
			{
				case 6:
				case 7:
					ParseCommandConfigDataFirst(buffer);
					return;

				case 8:
					ParseCommandConfigDataNext(buffer);
					return;

				default:
					return;
			}
			return;
		}

		void MaerklinCAN::ParseCommandConfigDataFirst(const unsigned char* const buffer)
		{
			if (canFileData != nullptr)
			{
				free(canFileData);
			}
			canFileDataSize = Utils::Integer::DataBigEndianToInt(buffer + 5);
			canFileCrc = Utils::Integer::DataBigEndianToShort(buffer + 9);
			canFileCrcSize = (canFileDataSize + 8) & 0xFFFFFFF8;
			canFileData = reinterpret_cast<unsigned char*>(malloc(canFileCrcSize));
			canFileDataPointer = canFileData;
		}

		void MaerklinCAN::ParseCommandConfigDataNext(const unsigned char* const buffer)
		{
			if (canFileData == nullptr)
			{
				return;
			}

			Utils::Utils::Copy8Bytes(buffer + 5, canFileDataPointer);
			canFileDataPointer += 8;
			if (canFileDataSize > static_cast<size_t>(canFileDataPointer - canFileData))
			{
				return;
			}

			const CanFileCrc calculatedCrc = CalcCrc(canFileData, canFileCrcSize);
			if (canFileCrc != calculatedCrc)
			{
				logger->Info(Languages::TextCrcMissmatch, Utils::Integer::IntegerToHex(canFileCrc), Utils::Integer::IntegerToHex(calculatedCrc));
				CleanUpCanFileData();
				return;
			}

			size_t canFileUncompressedSize = Utils::Integer::DataBigEndianToInt(canFileData);
			logger->Info(Languages::TextConfigFileReceivedWithSize, canFileUncompressedSize);
			string file = ZLib::UnCompress(reinterpret_cast<char*>(canFileData + 4), canFileDataSize, canFileUncompressedSize);
			deque<string> lines;
			Utils::Utils::SplitString(file, "\n", lines);
			for (std::string& line : lines)
			{
				logger->Debug(line);
			}

			ParseCs2File(lines);
			CleanUpCanFileData();
		}

		void MaerklinCAN::ParseResponseS88Event(const unsigned char* const buffer)
		{
			if (ParseLength(buffer) != 8)
			{
				return;
			}
			const char* onOff;
			DataModel::Feedback::FeedbackState state;
			if (buffer[10])
			{
				onOff = Languages::GetText(Languages::TextOn);
				state = DataModel::Feedback::FeedbackStateOccupied;
			}
			else
			{
				onOff = Languages::GetText(Languages::TextOff);
				state = DataModel::Feedback::FeedbackStateFree;
			}
			FeedbackPin pin = ParseFeedbackPin(buffer);
			logger->Info(Languages::TextFeedbackChange, pin & 0x000F, pin >> 4, onOff);
			manager->FeedbackState(controlID, pin, state);
		}

		void MaerklinCAN::ParseResponseReadConfig(const unsigned char* const buffer)
		{
			if (ParseLength(buffer) != 7)
			{
				return;
			}
			CvNumber cv = Utils::Integer::DataBigEndianToShort(buffer + 9);
			CvValue value = buffer[11];
			logger->Info(Languages::TextProgramReadValue, cv, value);
			manager->ProgramValue(cv, value);
		}

		void MaerklinCAN::ParseResponsePing(const unsigned char* const buffer)
		{
			const uint16_t deviceType = Utils::Integer::DataBigEndianToShort(buffer + 11);
			char* deviceString = nullptr;
			switch (deviceType)
			{
				case CanDeviceGfp:
					deviceString = const_cast<char*>("Gleisformat Prozessor");
					break;

				case CanDeviceGleisbox:
					case CanDeviceGleisbox_2:
					deviceString = const_cast<char*>("Gleisbox");
					break;

				case CanDeviceConnect6021:
					deviceString = const_cast<char*>("Connect 6021");
					break;

				case CanDeviceMs2:
					case CanDeviceMs2_2:
					case CanDeviceMs2_3:
					case CanDeviceMs2_4:
					deviceString = const_cast<char*>("MS2");
					break;

				case CanDeviceWireless:
					deviceString = const_cast<char*>("Wireless");
					break;

				case CanDeviceCs2Master:
					deviceString = const_cast<char*>("CS2 Master");
					hasCs2Master = true;
					break;

				case CanDeviceCs2Slave:
					case CanDeviceCs2Slave_2:
					deviceString = const_cast<char*>("CS2 Slave");
					break;

				case CanDeviceLinkS88:
					deviceString = const_cast<char*>("Link S88");
					hasCs2Master = true;
					break;

				default:
					deviceString = const_cast<char*>("unknown");
					break;
			}
			const string hash = Utils::Integer::IntegerToHex(Utils::Integer::DataBigEndianToShort(buffer + 2));
			const unsigned char deviceId = buffer[8];
			const unsigned char majorVersion = buffer[9];
			const unsigned char minorVersion = buffer[10];
			logger->Debug(Languages::TextDeviceOnCanBus, deviceString, hash, deviceId, majorVersion, minorVersion);
		}

		bool MaerklinCAN::ParseCs2FileKeyValue(const string& line, string& key, string& value)
		{
			if (line.length() < 4 || line[0] != ' ' || line[1] != '.')
			{
				return false;
			}
			const string stripedLine = line.substr(2);
			Utils::Utils::SplitString(stripedLine, "=", key, value);
			return (key.compare(stripedLine) != 0);
		}

		bool MaerklinCAN::ParseCs2FileSubkeyValue(const string& line, string& key, string& value)
		{
			if (line.length() < 5 || line[0] != ' ' || line[1] != '.' || line[2] != '.')
			{
				return false;
			}
			const string stripedLine = line.substr(3);
			Utils::Utils::SplitString(stripedLine, "=", key, value);
			return (key.compare(stripedLine) != 0);
		}

		void MaerklinCAN::ParseCs2FileLocomotiveFunction(deque<string>& lines, LocoCacheEntry& cacheEntry)
		{
			lines.pop_front();
			DataModel::LocoFunctionNr nr = 0;
			DataModel::LocoFunctionType type = DataModel::LocoFunctionTypeNone;
			DataModel::LocoFunctionIcon icon = DataModel::LocoFunctionIconNone;
			DataModel::LocoFunctionTimer timer = 0;
			while (lines.size())
			{
				string& line = lines.front();
				string key;
				string value;
				bool ok = ParseCs2FileSubkeyValue(line, key, value);
				if (ok == false)
				{
					break;
				}
				if (key.compare("nr") == 0)
				{
					nr = Utils::Integer::StringToInteger(value);
				}
				else if (key.compare("typ") == 0 || key.compare("typ2") == 0)
				{
					uint8_t valueInt = Utils::Integer::StringToInteger(value);
					icon = MapLocoFunctionCs2ToRailControl(static_cast<LocoFunctionCs2Icon>(valueInt & 0x7F));
					type = static_cast<DataModel::LocoFunctionType>((valueInt >> 7) + 1); // CS2: 1 = permanent, 2 = once
				}
				else if (key.compare("dauer") == 0 || key.compare("dauer2") == 0)
				{
					type = DataModel::LocoFunctionTypeTimer;
					timer = Utils::Integer::StringToInteger(value);
				}
				lines.pop_front();
			}
			if (type == DataModel::LocoFunctionTypeNone)
			{
				icon = DataModel::LocoFunctionIconNone;
				timer = 0;
				cacheEntry.ClearFunction(nr);
				return;
			}
			cacheEntry.SetFunction(nr, type, icon, timer);
			if (type == DataModel::LocoFunctionTypeTimer)
			{
				logger->Info(Languages::TextCs2MasterLocoFunctionIconTypeTimer, nr, icon, timer);
			}
			else
			{
				logger->Info(Languages::TextCs2MasterLocoFunctionIconType, nr, icon, type);
			}
		}

		void MaerklinCAN::ParseCs2FileLocomotiveTraktion(deque<string>& lines, LocoCacheEntry& cacheEntry)
		{
			lines.pop_front();
			Protocol protocol = ProtocolNone;
			Address address = AddressNone;
			string name;
			while (lines.size())
			{
				string& line = lines.front();
				string key;
				string value;
				bool ok = ParseCs2FileSubkeyValue(line, key, value);
				if (ok == false)
				{
					break;
				}
				if (key.compare("lok") == 0)
				{
					Address input = Utils::Integer::HexToInteger(value);
					LocoType type;
					ParseAddressProtocol(input, address, protocol, type);
					logger->Info(Languages::TextCs2MasterLocoSlaveProtocolAddress, Utils::Utils::ProtocolToString(protocol), address);
				}
				else if (key.compare("lokname") == 0)
				{
					name = value;
					logger->Info(Languages::TextCs2MasterLocoSlaveName, name);
				}
				lines.pop_front();
			}
			cacheEntry.AddSlave(protocol, address, name);
		}

		void MaerklinCAN::ParseCs2FileLocomotive(deque<string>& lines)
		{
			lines.pop_front();
			LocoCacheEntry cacheEntry(locoCache.GetControlId());
			std::string name;
			std::string oldName;
			bool remove = false;
			while (lines.size())
			{
				string& line = lines.front();
				if ((line.length() == 0) || (line[0] != ' '))
				{
					break;
				}
				string key;
				string value;
				ParseCs2FileKeyValue(line, key, value);
				if (key.compare("name") == 0)
				{
					name = value;
					cacheEntry.SetName(value);
					cacheEntry.SetMatchKey(value);
					logger->Info(Languages::TextCs2MasterLocoName, value);
				}
				else if (key.compare("vorname") == 0)
				{
					oldName = value;
					logger->Info(Languages::TextCs2MasterLocoOldName, value);
				}
				else if (key.compare("toRemove") == 0)
				{
					remove = true;
				}
				else if (key.compare("uid") == 0)
				{
					Address input = Utils::Integer::HexToInteger(value);
					Address address = AddressNone;
					Protocol protocol = ProtocolNone;
					LocoType type;
					ParseAddressProtocol(input, address, protocol, type);
					cacheEntry.SetAddress(address);
					cacheEntry.SetProtocol(protocol);
					cacheEntry.SetType(type);
					logger->Info(Languages::TextCs2MasterLocoProtocolAddress, Utils::Utils::ProtocolToString(protocol), address);
				}
				else if ((key.compare("funktionen") == 0)
					|| (key.compare("funktionen_2") == 0)
					|| (key.compare("fkt") == 0)
					|| (key.compare("fkt2") == 0))
				{
					ParseCs2FileLocomotiveFunction(lines, cacheEntry);
					continue;
				}
				else if (key.compare("traktion") == 0)
				{
					ParseCs2FileLocomotiveTraktion(lines, cacheEntry);
					continue;
				}
				lines.pop_front();
			}

			if (remove)
			{
				logger->Info(Languages::TextCs2MasterLocoRemove, name);
				LocoID locoId = locoCache.Delete(name);
				manager->LocoDelete(locoId);
			}
			else if (oldName.size() > 0)
			{
				locoCache.Save(cacheEntry, oldName);
			}
			else
			{
				locoCache.Save(cacheEntry);
			}
		}

		void MaerklinCAN::ParseCs2FileLocomotivesSession(deque<string>& lines)
		{
			lines.pop_front();
			while (lines.size())
			{
				string& line = lines.front();
				string key;
				string value;
				bool ok = ParseCs2FileKeyValue(line, key, value);
				if (ok == false)
				{
					return;
				}
				// we do not parse any data in session
				lines.pop_front();
			}
		}

		void MaerklinCAN::ParseCs2FileLocomotivesVersion(deque<string>& lines)
		{
			lines.pop_front();
			while (lines.size())
			{
				string& line = lines.front();
				string key;
				string value;
				bool ok = ParseCs2FileKeyValue(line, key, value);
				if (ok == false)
				{
					return;
				}
				if (key.compare("minor") == 0 && value.compare("3") != 0 && value.compare("4"))
				{
					logger->Warning(Languages::TextCs2MinorVersionIsUnknown);
				}
				lines.pop_front();
			}
		}

		void MaerklinCAN::ParseCs2FileLocomotives(deque<string>& lines)
		{
			lines.pop_front();
			while (lines.size())
			{
				string& line = lines.front();
				if (line.length() == 0)
				{
					return;
				}
				if (line.compare("version") == 0)
				{
					ParseCs2FileLocomotivesVersion(lines);
					continue;
				}
				else if (line.compare("session") == 0)
				{
					ParseCs2FileLocomotivesSession(lines);
					continue;
				}
				else if (line.compare("lokomotive") == 0)
				{
					ParseCs2FileLocomotive(lines);
					continue;
				}
				return;
			}
		}

		void MaerklinCAN::ParseCs2File(deque<string>& lines)
		{
			while (lines.size())
			{
				string& line = lines.front();
				if (line.length() == 0 || line[0] != '[')
				{
					return;
				}
				if (line.compare("[lokomotive]") == 0)
				{
					ParseCs2FileLocomotives(lines);
					locoCache.UpdateSlaves();
					continue;
				}
				return;
			}
		}

		uint16_t MaerklinCAN::CalcCrc(const unsigned char *data, const size_t length)
		{
			// Maerklin uses CRC-16-CCITT with seed 0xFFFF and with polynom 0x1021 -> x^16 + x^12 +x^5 + 1
			static const uint16_t CrcTable[256] = {
				0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
				0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
				0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
				0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
				0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
				0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
				0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
				0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
				0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
				0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
				0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
				0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
				0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
				0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
				0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
				0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
				0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
				0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
				0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
				0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
				0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
				0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
				0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
				0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
				0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
				0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
				0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
				0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
				0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
				0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
				0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
				0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
			};

			uint16_t crc = 0xFFFF;
			for (size_t count = 0; count < length; ++count)
			{
				const uint16_t temp = (*data++ ^ (crc >> 8)) & 0xFF;
				crc = CrcTable[temp] ^ (crc << 8);
			}
			return crc;
		}

		const DataModel::LocoFunctionIcon MaerklinCAN::LocoFunctionMapCs2ToRailControl[MaxNrOfCs2FunctionIcons] =
		{
			DataModel::LocoFunctionIconNone,
			DataModel::LocoFunctionIconLight,
			DataModel::LocoFunctionIconInteriorLight1,
			DataModel::LocoFunctionIconBacklightForward,
			DataModel::LocoFunctionIconHeadlightHighBeamForward,
			DataModel::LocoFunctionIconSoundGeneral,
			DataModel::LocoFunctionIconPanto12,
			DataModel::LocoFunctionIconSmokeGenerator,
			DataModel::LocoFunctionIconShuntingMode,
			DataModel::LocoFunctionIconTelex12,
			DataModel::LocoFunctionIconHorn1,
			DataModel::LocoFunctionIconWhistle1,
			DataModel::LocoFunctionIconWhistle2,
			DataModel::LocoFunctionIconBell,
			DataModel::LocoFunctionIconLeftRight,
			DataModel::LocoFunctionIconUpDown1,
			DataModel::LocoFunctionIconTurnLeft,
			DataModel::LocoFunctionIconUpDown2,
			DataModel::LocoFunctionIconInertia,
			DataModel::LocoFunctionIconFan2,
			DataModel::LocoFunctionIconBreak1,
			DataModel::LocoFunctionIconGearBox,
			DataModel::LocoFunctionIconGenerator,
			DataModel::LocoFunctionIconRunning1,
			DataModel::LocoFunctionIconEngine1,
			DataModel::LocoFunctionIconStationAnnouncement1,
			DataModel::LocoFunctionIconShovelCoal,
			DataModel::LocoFunctionIconCloseDoor,
			DataModel::LocoFunctionIconOpenDoor,
			DataModel::LocoFunctionIconFan1,
			DataModel::LocoFunctionIconFan,
			DataModel::LocoFunctionIconFireBox,
			DataModel::LocoFunctionIconInteriorLight2,
			DataModel::LocoFunctionIconTableLight3,
			DataModel::LocoFunctionIconTableLight2,
			DataModel::LocoFunctionIconTableLight1,
			DataModel::LocoFunctionIconShakingRust,
			DataModel::LocoFunctionIconRailJoint,
			DataModel::LocoFunctionIconLocomotiveNumberIndicator,
			DataModel::LocoFunctionIconMusic1,
			DataModel::LocoFunctionIconTrainDestinationIndicator,
			DataModel::LocoFunctionIconCabLight2,
			DataModel::LocoFunctionIconCabLight1,
			DataModel::LocoFunctionIconCoupler,
			DataModel::LocoFunctionIconBufferPush,
			DataModel::LocoFunctionIconStationAnnouncement3,
			DataModel::LocoFunctionIconCraneHook,
			DataModel::LocoFunctionIconBlinkingLight,
			DataModel::LocoFunctionIconCabLight12,
			DataModel::LocoFunctionIconCompressedAir,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconTelex2,
			DataModel::LocoFunctionIconTelex1,
			DataModel::LocoFunctionIconPanto2,
			DataModel::LocoFunctionIconPanto1,
			DataModel::LocoFunctionIconHeadlightLowBeamReverse,
			DataModel::LocoFunctionIconHeadlightLowBeamForward,
			DataModel::LocoFunctionIconUp,
			DataModel::LocoFunctionIconFan3,
			DataModel::LocoFunctionIconEngineLight,
			DataModel::LocoFunctionIconSteamBlowOut,
			DataModel::LocoFunctionIconSteamBlow,
			DataModel::LocoFunctionIconCrane,
			DataModel::LocoFunctionIconUp,
			DataModel::LocoFunctionIconDown,
			DataModel::LocoFunctionIconLeft,
			DataModel::LocoFunctionIconRight,
			DataModel::LocoFunctionIconTurnRight,
			DataModel::LocoFunctionIconMagnet,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconPanto,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconRadio,
			DataModel::LocoFunctionIconStationAnnouncement2,
			DataModel::LocoFunctionIconBacklightReverse,
			DataModel::LocoFunctionIconAirPump,
			DataModel::LocoFunctionIconSpeak,
			DataModel::LocoFunctionIconEngine2,
			DataModel::LocoFunctionIconNoSound,
			DataModel::LocoFunctionIconStairsLight,
			DataModel::LocoFunctionIconFillWater,
			DataModel::LocoFunctionIconBreak2,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault,
			DataModel::LocoFunctionIconDefault
		};

		const MaerklinCAN::LocoFunctionCs2Icon MaerklinCAN::LocoFunctionMapRailControlToCs2[DataModel::MaxLocoFunctionIcons] =
		{
			LocoFunctionCs2IconNone,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconShuntingMode,
			LocoFunctionCs2IconInertia,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconLight,
			LocoFunctionCs2IconLight,
			LocoFunctionCs2IconLight,
			LocoFunctionCs2IconHeadlightHighBeamForward,
			LocoFunctionCs2IconLight,
			LocoFunctionCs2IconHeadlightForward,
			LocoFunctionCs2IconHeadlightReverse,
			LocoFunctionCs2IconBackLightForward,
			LocoFunctionCs2IconBackLightReverse,
			LocoFunctionCs2IconLight,
			LocoFunctionCs2IconBlinkingLight,
			LocoFunctionCs2IconInteriorLight1,
			LocoFunctionCs2IconInteriorLight2,
			LocoFunctionCs2IconTableLight1,
			LocoFunctionCs2IconTableLight2,
			LocoFunctionCs2IconTableLight3,
			LocoFunctionCs2IconCabLight1,
			LocoFunctionCs2IconCabLight2,
			LocoFunctionCs2IconCabLight12,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconTrainDestinationIndicator,
			LocoFunctionCs2IconLocomotiveNumberIndicator,
			LocoFunctionCs2IconEngineLight,
			LocoFunctionCs2IconFireBox,
			LocoFunctionCs2IconStairsLight,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconSmokeGenerator,
			LocoFunctionCs2IconTelex1,
			LocoFunctionCs2IconTelex2,
			LocoFunctionCs2IconTelex12,
			LocoFunctionCs2IconPanto1,
			LocoFunctionCs2IconPanto2,
			LocoFunctionCs2IconPanto12,
			LocoFunctionCs2IconUp1,
			LocoFunctionCs2IconDown,
			LocoFunctionCs2IconUpDown1,
			LocoFunctionCs2IconUpDown2,
			LocoFunctionCs2IconLeft,
			LocoFunctionCs2IconRight,
			LocoFunctionCs2IconLeftRight,
			LocoFunctionCs2IconTurnLeft,
			LocoFunctionCs2IconTurnRight,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconCrane,
			LocoFunctionCs2IconMagnet,
			LocoFunctionCs2IconCraneHook,
			LocoFunctionCs2IconFan,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconDefault,
			LocoFunctionCs2IconNoSound,
			LocoFunctionCs2IconSoundGeneral,
			LocoFunctionCs2IconRunning1,
			LocoFunctionCs2IconSoundGeneral,
			LocoFunctionCs2IconEngine1,
			LocoFunctionCs2IconEngine2,
			LocoFunctionCs2IconBreak1,
			LocoFunctionCs2IconBreak2,
			LocoFunctionCs2IconSoundGeneral,
			LocoFunctionCs2IconHorn1,
			LocoFunctionCs2IconSoundGeneral,
			LocoFunctionCs2IconWhistle1,
			LocoFunctionCs2IconWhistle2,
			LocoFunctionCs2IconBell,
			LocoFunctionCs2IconStationAnnouncement1,
			LocoFunctionCs2IconStationAnnouncement2,
			LocoFunctionCs2IconStationAnnouncement3,
			LocoFunctionCs2IconSpeak,
			LocoFunctionCs2IconRadio,
			LocoFunctionCs2IconMusic1,
			LocoFunctionCs2IconSoundGeneral,
			LocoFunctionCs2IconOpenDoor,
			LocoFunctionCs2IconCloseDoor,
			LocoFunctionCs2IconFan1,
			LocoFunctionCs2IconFan2,
			LocoFunctionCs2IconFan3,
			LocoFunctionCs2IconShovelCoal,
			LocoFunctionCs2IconCompressedAir,
			LocoFunctionCs2IconSoundGeneral,
			LocoFunctionCs2IconSteamBlowOut,
			LocoFunctionCs2IconSteamBlow,
			LocoFunctionCs2IconSoundGeneral,
			LocoFunctionCs2IconShakingRust,
			LocoFunctionCs2IconAirPump,
			LocoFunctionCs2IconSoundGeneral,
			LocoFunctionCs2IconBufferPush,
			LocoFunctionCs2IconGenerator,
			LocoFunctionCs2IconGearBox,
			LocoFunctionCs2IconSoundGeneral,
			LocoFunctionCs2IconSoundGeneral,
			LocoFunctionCs2IconFillWater,
			LocoFunctionCs2IconSoundGeneral,
			LocoFunctionCs2IconSoundGeneral,
			LocoFunctionCs2IconSoundGeneral,
			LocoFunctionCs2IconRailJoint,
			LocoFunctionCs2IconCoupler,
			LocoFunctionCs2IconPanto,
			LocoFunctionCs2IconSoundGeneral,
			LocoFunctionCs2IconSoundGeneral,
			LocoFunctionCs2IconNoSound,
			LocoFunctionCs2IconNoSound
		};
	} // namespace
} // namespace
