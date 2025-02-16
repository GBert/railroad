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

#include <deque>
#include <cstring>

#include "DataModel/AccessoryBase.h"
#include "Hardware/HardwareInterface.h"
#include "Hardware/HardwareParams.h"
#include "Hardware/Protocols/EsuCAN.h"
#include "Utils/Integer.h"

using std::deque;
using std::string;
using std::strlen;
using std::to_string;

namespace Hardware
{
	namespace Protocols
	{
		const char* const EsuCAN::CommandActivateBoosterUpdates = "request(1,view)\n";
		const char* const EsuCAN::CommandQueryLocos = "queryObjects(10,name)\n";
		const char* const EsuCAN::CommandQueryAccessories = "queryObjects(11,name1,name2,name3)\n";
		const char* const EsuCAN::CommandQueryFeedbacks = "queryObjects(26,ports)\n";

		EsuCAN::EsuCAN(const HardwareParams* params, const std::string& controlName)
		:	HardwareInterface(params->GetManager(),
			params->GetControlID(),
				controlName + " / " + params->GetName() + " at IP " + params->GetArg1(),
				params->GetName()),
			run(false),
			tcp(Network::TcpClient::GetTcpClientConnection(logger, params->GetArg1(), EsuCANPort)),
			readBufferLength(0),
			readBufferPosition(0),
			locoCache(params->GetControlID(), params->GetManager()),
			accessoryCache(params->GetControlID(), params->GetManager()),
			feedbackCache(params->GetControlID(), params->GetManager())
		{
			logger->Info(Languages::TextStarting, GetFullName());
			std::memset(feedbackMemory, 0, MaxFeedbackModules);
			receiverThread = std::thread(&Hardware::Protocols::EsuCAN::Receiver, this);
			if (!tcp.IsConnected())
			{
				return;
			}
			SendActivateBoosterUpdates();
			SendQueryLocos();
			SendQueryAccessories();
			SendQueryFeedbacks();
		}

		EsuCAN::~EsuCAN()
		{
			run = false;
			receiverThread.join();
			logger->Info(Languages::TextTerminatingSenderSocket);
		}

		void EsuCAN::LocoSpeed(__attribute__((unused)) const Protocol protocol,
			const Address address,
			const Speed speed)
		{
			const unsigned int locoId = address + OffsetLocoAddress;
			SendGetHandle(locoId);
			const string command = "set(" + to_string(locoId) + ",speed[" + to_string(speed >> 3) + "])\n";
			Send(command.c_str());
		}

		void EsuCAN::LocoOrientation(__attribute__((unused)) const Protocol protocol,
			const Address address,
		    const Orientation orientation)
		{
			const unsigned int locoId = address + OffsetLocoAddress;
			SendGetHandle(locoId);
			const string command = "set(" + to_string(locoId) + ",dir[" + (orientation == OrientationRight ? "0" : "1") + "])\n";
			Send(command.c_str());
		}

		void EsuCAN::LocoFunction(__attribute__((unused)) const Protocol protocol,
		    const Address address,
		    const DataModel::LocoFunctionNr function,
		    const DataModel::LocoFunctionState on)
		{
			const unsigned int locoId = address + OffsetLocoAddress;
			SendGetHandle(locoId);
			const string command = "set(" + to_string(locoId) + ",func[" + to_string(function) + "," + (on == DataModel::LocoFunctionStateOn ? "1" : "0") + "])\n";
			Send(command.c_str());
		}

		void EsuCAN::Accessory(__attribute__((unused))  const Protocol protocol,
			const Address address,
			const DataModel::AccessoryState state,
			const bool on,
			__attribute__((unused)) const DataModel::AccessoryPulseDuration duration)
		{
			if (!on)
			{
				return;
			}
			const unsigned int accessoryId = address + OffsetAccessoryAddress;
			SendGetHandle(accessoryId);
			const string command = "set(" + to_string(accessoryId) + ",state[" + (state ? "0" : "1") + "])\n";
			Send(command.c_str());
		}

		void EsuCAN::Send(const char* data)
		{
			int ret = tcp.Send(data);
			if (ret < 0)
			{
				logger->Error(Languages::TextUnableToSendDataToControl);
			}
		}

		void EsuCAN::Receiver()
		{
			Utils::Utils::SetThreadName("EsuCAN");
			if (!tcp.IsConnected())
			{
				return;
			}
			logger->Info(Languages::TextReceiverThreadStarted);

			run = true;
			while (run)
			{
				ReadLine();

				if (run == false)
				{
					break;
				}

				if (readBufferLength == 0)
				{
					if (errno == ETIMEDOUT)
					{
						continue;
					}

					logger->Error(Languages::TextUnableToReceiveData);
					break;
				}

				Parser();
			}
			tcp.Terminate();
			logger->Info(Languages::TextTerminatingReceiverThread);
		}

		void EsuCAN::ReadLine()
		{
			readBufferPosition = 0;
			readBufferLength = -1;
			while (true)
			{
				int dataLength = tcp.Receive(readBuffer + readBufferPosition, 1);
				if (dataLength != 1)
				{
					readBufferLength = 0;
					break;
				}
				if (readBuffer[readBufferPosition] == '\r')
				{
					continue;
				}
				if (readBuffer[readBufferPosition] == '\n')
				{
					readBufferLength = readBufferPosition;
					break;
				}
				++readBufferPosition;
				if (readBufferPosition >= MaxMessageSize)
				{
					break;
				}
			}
			readBufferPosition = 0;
			if (readBufferLength == 0)
			{
				return;
			}
			logger->HexIn(reinterpret_cast<unsigned char*>(readBuffer), readBufferLength);
		}

		void EsuCAN::Parser()
		{
			if (!CheckAndConsumeChar('<'))
			{
				logger->Error(Languages::TextInvalidDataReceived);
				return;
			}
			char type = ReadAndConsumeChar();
			switch (type)
			{
				case 'R':
					ParseReply();
					return;

				case 'E':
					ParseEvent();
					return;

				default:
					logger->Error(Languages::TextInvalidDataReceived);
					return;
			}
		}

		void EsuCAN::ParseReply()
		{
			if (CompareAndConsume("EPLY", 4) == false)
			{
				logger->Error(Languages::TextInvalidDataReceived);
				return;
			}
			SkipWhiteSpace();

			if (Compare(CommandQueryLocos, strlen(CommandQueryLocos) - 1))
			{
				ParseQueryLocos();
				return;
			}

			if (Compare(CommandQueryAccessories, strlen(CommandQueryAccessories) - 1))
			{
				ParseQueryAccessories();
				return;
			}

			if (Compare(CommandQueryFeedbacks, strlen(CommandQueryFeedbacks) - 1))
			{
				ParseQueryFeedbacks();
				return;
			}

			ReadLine();
			while (GetChar() != '<')
			{
				// ParseLine()
				ReadLine();
			}
			ParseEndLine();
		}

		void EsuCAN::ParseQueryLocos()
		{
			ReadLine();
			while (GetChar() != '<')
			{
				ParseLocoData();
				ReadLine();
			}
			ParseEndLine();
		}

		void EsuCAN::ParseQueryAccessories()
		{
			ReadLine();
			while (GetChar() != '<')
			{
				ParseAccessoryData();
				ReadLine();
			}
			ParseEndLine();
		}

		void EsuCAN::ParseQueryFeedbacks()
		{
			ReadLine();
			while (GetChar() != '<')
			{
				ParseFeedbackData();
				ReadLine();
			}
			ParseEndLine();
		}

		void EsuCAN::ParseLocoData()
		{
			const unsigned int locoId = ParseInt();
			const Address address = locoId - OffsetLocoAddress;
			SendActivateUpdates(locoId);

			string name;
			while (true)
			{
				string option;
				string value;
				ParseOption(option, value);
				if (option.size() == 0)
				{
					break;
				}

				if (option.compare("name") == 0)
				{
					name = value;
				}
			}

			LocoCacheEntry cacheEntry(locoCache.GetControlId());
			cacheEntry.SetMatchKey(locoId);
			cacheEntry.SetProtocol(ProtocolServer);
			cacheEntry.SetAddress(address);
			cacheEntry.SetName(name);
			locoCache.Save(cacheEntry);

			logger->Info(Languages::TextFoundLocoInEcosDatabase, address, name);
		}

		void EsuCAN::ParseAccessoryData()
		{
			const unsigned int accessoryId = ParseInt();
			const Address address = accessoryId - OffsetAccessoryAddress;
			SendActivateUpdates(accessoryId);

			string name;
			string name2;
			string name3;
			while (true)
			{
				string option;
				string value;
				ParseOption(option, value);
				if (option.size() == 0)
				{
					break;
				}

				if (option.compare("name1") == 0)
				{
					name = value;
				}
				else if (option.compare("name2") == 0)
				{
					name2 = value;
				}
				else if (option.compare("name3") == 0)
				{
					name3 = value;
				}
			}

			AccessoryCacheEntry cacheEntry(accessoryCache.GetControlId());
			cacheEntry.SetMatchKey(accessoryId);
			cacheEntry.SetProtocol(ProtocolServer);
			cacheEntry.SetAddress(address);

			if (name2.size())
			{
				name += " " + name2;
			}
			if (name3.size())
			{
				name += " " + name3;
			}
			cacheEntry.SetName(name);
			accessoryCache.Save(cacheEntry);

			logger->Info(Languages::TextFoundAccessoryInEcosDatabase, address, name, name2, name3);
		}

		void EsuCAN::ParseFeedbackData()
		{
			const int moduleId = ParseInt();
			const int moduleIdHuman = moduleId - OffsetFeedbackModuleAddress;
			const int moduleIdMachine = moduleId - OffsetFeedbackModuleAddress - 1;
			SendActivateUpdates(moduleId);

			int ports = 0;
			string name;
			while (true)
			{
				string option;
				string value;
				ParseOption(option, value);
				if (option.size() == 0)
				{
					break;
				}

				if (option.compare("ports") == 0)
				{
					ports = Utils::Integer::StringToInteger(value, 16);
					name = "Module " + Utils::Utils::ToStringWithLeadingZeros(moduleIdHuman, 3) + " Pin ";
				}
			}

			FeedbackCacheEntry cacheEntry(feedbackCache.GetControlId());
			for (int pinOfModule = 1; pinOfModule <= ports; ++pinOfModule)
			{
				int pin = (moduleIdMachine * 16) + pinOfModule;
				const string pinAsText = Utils::Utils::ToStringWithLeadingZeros(pinOfModule, 2);
				cacheEntry.SetMatchKey(to_string(moduleId) + "/" + pinAsText);
				cacheEntry.SetFeedbackID(FeedbackNone);
				cacheEntry.SetPin(pin);
				cacheEntry.SetName(name + pinAsText);
				feedbackCache.Save(cacheEntry);
			}

			logger->Info(Languages::TextFoundFeedbackModuleInEcosDatabase, moduleIdHuman);
		}

		void EsuCAN::ParseOption(string& option, string& value)
		{
			SkipWhiteSpace();
			option = ReadUntilChar('[');
			if (!CheckAndConsumeChar('['))
			{
				return;
			}
			if (GetChar() == '"')
			{
				CheckAndConsumeChar('"');
				value = ReadUntilChar('"');
				CheckAndConsumeChar('"');
			}
			else
			{
				value = ReadUntilChar(']');
			}
			CheckAndConsumeChar(']');
		}

		void EsuCAN::ParseOptionInt(string& option, int& value)
		{
			string stringValue;
			ParseOption(option, stringValue);
			value = Utils::Integer::StringToInteger(stringValue, 0);
		}

		void EsuCAN::ParseOptionHex(string& option, int& value)
		{
			string stringValue;
			ParseOption(option, stringValue);
			value = Utils::Integer::HexToInteger(stringValue, 0);
		}

		void EsuCAN::ParseEvent()
		{
			if (CompareAndConsume("VENT", 4) == false)
			{
				logger->Error(Languages::TextInvalidDataReceived);
				return;
			}
			ParseInt();
			if (CheckGraterThenAtLineEnd() == false)
			{
				logger->Error(Languages::TextInvalidDataReceived);
				return;
			}
			while (true)
			{
				ReadLine();
				if (GetChar() == '<')
				{
					ParseEndLine();
					return;
				}
				ParseEventLine();
			}
		}

		void EsuCAN::ParseEventLine()
		{
			int object = ParseInt();
			SkipWhiteSpace();

			if (object >= MinAccessoryId)
			{
				ParseAccessoryEvent(object);
				return;
			}

			if (object >= MinLocoId)
			{
				ParseLocoEvent(object);
				return;
			}

			if (object >= MinFeedbackModuleId)
			{
				ParseFeedbackEvent(object);
				return;
			}

			if (object == 1)
			{
				ParseBoosterEvent();
				return;
			}

			string event = ReadUntilLineEnd();
			logger->HexIn(event);
		}

		void EsuCAN::ParseBoosterEvent()
		{
			if (Compare("status[GO]", 10))
			{
				manager->Booster(ControlTypeHardware, BoosterStateGo);
			}
			else if (Compare("status[STOP]", 12))
			{
				manager->Booster(ControlTypeHardware, BoosterStateStop);
			}
		}

		void EsuCAN::ParseLocoEvent(int loco)
		{
			const Address address = loco - OffsetLocoAddress;
			string option;
			string value;
			ParseOption(option, value);

			if (option.compare("speed") == 0)
			{
				Speed speed = Utils::Integer::StringToInteger(value) << 3;
				manager->LocoSpeed(ControlTypeHardware, controlID, ProtocolServer, address, speed);
				return;
			}

			if (option.compare("dir") == 0)
			{
				Orientation orientation = (Utils::Integer::StringToInteger(value) == 1 ? OrientationLeft : OrientationRight);
				manager->LocoOrientation(ControlTypeHardware, controlID, ProtocolServer, address, orientation);
				return;
			}

			if (option.compare("func") == 0)
			{
				deque < string > valueList;
				Utils::Utils::SplitString(value, ",", valueList);
				if (valueList.size() < 2)
				{
					logger->Error(Languages::TextInvalidDataReceived);
					return;
				}
				DataModel::LocoFunctionNr function = Utils::Integer::StringToInteger(valueList[0], 0);
				DataModel::LocoFunctionState on = Utils::Utils::StringToBool(valueList[1])
					? DataModel::LocoFunctionStateOn : DataModel::LocoFunctionStateOff;
				manager->LocoFunctionState(ControlTypeHardware, controlID, ProtocolServer, address, function, on);
				return;
			}
		}

		void EsuCAN::ParseAccessoryEvent(int accessory)
		{
			const Address address = accessory - OffsetAccessoryAddress;
			string option;
			int value;
			ParseOptionInt(option, value);

			if (option.compare("state") == 0)
			{
				DataModel::AccessoryState state = (
				    value == 0 ? DataModel::AccessoryStateOn : DataModel::AccessoryStateOff);
				manager->AccessoryBaseState(ControlTypeHardware, controlID, ProtocolServer, address, state);
				return;
			}
		}

		void EsuCAN::ParseFeedbackEvent(int feedback)
		{
			string option;
			int value;
			ParseOptionHex(option, value);

			if (option.compare("state") == 0)
			{
				unsigned int module1 = (feedback - MinFeedbackModuleId) << 1;
				unsigned int module2 = module1 + 1;
				uint8_t moduleData1 = value & 0xFF;
				uint8_t moduleData2 = (value >> 8) & 0xFF;
				CheckFeedbackDiff(module1, moduleData1);
				CheckFeedbackDiff(module2, moduleData2);
				return;
			}
		}

		void EsuCAN::CheckFeedbackDiff(unsigned int module, uint8_t data)
		{
			uint8_t oldData = feedbackMemory[module];
			if (oldData == data)
			{
				return;
			}

			for (unsigned int pin = 0; pin < 8; ++pin)
			{
				uint8_t oldPinData = (oldData >> pin) & 0x01;
				uint8_t pinData = (data >> pin) & 0x01;
				if (oldPinData == pinData)
				{
					continue;
				}
				const unsigned int address = (module << 3) + pin + 1;
				const DataModel::Feedback::FeedbackState state =
				    pinData == 1 ? DataModel::Feedback::FeedbackStateOccupied : DataModel::Feedback::FeedbackStateFree;
				manager->FeedbackState(controlID, address, state);
				logger->Info(Languages::TextFeedbackChange, address & 0x000F, address >> 4, state);
			}
			feedbackMemory[module] = data;
		}

		void EsuCAN::ParseEndLine()
		{
			if (CompareAndConsume("<END", 4) == false)
			{
				logger->Error(Languages::TextInvalidDataReceived);
				return;
			}
			int i = ParseInt();
			if (i == 0)
			{
				return;
			}
			SkipWhiteSpace();
			string error = ReadUntilChar('>');
			logger->Error(Languages::TextControlReturnedError, error);
		}

		string EsuCAN::ReadUntilChar(const char c)
		{
			string out;
			while (true)
			{
				const char readChar = GetChar();
				if (readChar == c || readChar == 0)
				{
					return out;
				}
				out.append(1, ReadAndConsumeChar());
			}
		}

		string EsuCAN::ReadUntilLineEnd()
		{
			string out = ReadUntilChar('\n');
			return out;
		}

		bool EsuCAN::Compare(const char* reference, const size_t size) const
		{
			for (size_t index = 0; index < size; ++index)
			{
				if (GetChar(index) != reference[index])
				{
					return false;
				}
			}
			return true;
		}

		bool EsuCAN::CompareAndConsume(const char* reference, const size_t size)
		{
			for (size_t index = 0; index < size; ++index)
			{
				if (ReadAndConsumeChar() != reference[index])
				{
					return false;
				}
			}
			return true;
		}

		bool EsuCAN::SkipOptionalChar(const char charToSkip)
		{
			if (charToSkip == GetChar())
			{
				++readBufferPosition;
				return true;
			}
			return false;
		}

		bool EsuCAN::IsNumber() const
		{
			const char c = GetChar();
			return (c >= '0' && c <= '9');
		}

		int EsuCAN::ParseInt()
		{
			SkipWhiteSpace();
			int out = 0;
			while (IsNumber())
			{
				out *= 10;
				out += ReadAndConsumeChar() - '0';
			}
			return out;
		}
	} // namespace
} // namespace
