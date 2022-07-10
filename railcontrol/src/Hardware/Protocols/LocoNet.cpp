/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2022 Dominik (Teddy) Mahrer - www.railcontrol.org

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

#include <string>
#include <thread>
#include "DataModel/LocoFunctions.h"
#include "Hardware/Protocols/LocoNet.h"
#include "Utils/Utils.h"

namespace Hardware
{
	namespace Protocols
	{
		LocoNet::LocoNet(const HardwareParams* params,
			const std::string& controlName,
			const unsigned int dataSpeed)
		:	HardwareInterface(params->GetManager(),
			   params->GetControlID(),
				controlName + " / " + params->GetName() + " at serial port " + params->GetArg1(),
			   params->GetName()),
			run(true),
			serialLine(logger, params->GetArg1(), dataSpeed, 8, 'N', 1)
		{
			receiverThread = std::thread(&Hardware::Protocols::LocoNet::Receiver, this);
		}

		LocoNet::~LocoNet()
		{
			run = false;
			receiverThread.join();
			logger->Info(Languages::TextTerminatingSenderSocket);
		}

		void LocoNet::Booster(const BoosterState status)
		{
			if (BoosterStateGo == status)
			{
				Send2ByteCommand(OPC_GPON);
				logger->Info(Languages::TextTurningBoosterOn);
			}
			else
			{
				Send2ByteCommand(OPC_GPOFF);
				logger->Info(Languages::TextTurningBoosterOff);
			}
		}

		void LocoNet::LocoSpeed(__attribute__((unused)) const Protocol protocol,
			const Address address,
			const Speed speed)
		{
			unsigned char slot = locoCache.GetSlotOfAddress(address);
			if (0 == slot)
			{
				SendLocoAddress(address);
				return;
			}
			SendLocoSpeed(slot, speed);
		}

		void LocoNet::LocoOrientation(__attribute__((unused)) const Protocol protocol,
			const Address address,
			const Orientation orientation)
		{
			unsigned char slot = locoCache.GetSlotOfAddress(address);
			if (0 == slot)
			{
				SendLocoAddress(address);
				return;
			}
			const unsigned char orientationF0F4 = SetOrientationF0F4Bit(slot, orientation, 5);
			SendLocoOrientationF0F4(slot, orientationF0F4);
		}

		void LocoNet::LocoFunction(__attribute__((unused)) const Protocol protocol,
			const Address address,
			const DataModel::LocoFunctionNr function,
			const DataModel::LocoFunctionState on)
		{
			unsigned char slot = locoCache.GetSlotOfAddress(address);
			if (0 == slot)
			{
				SendLocoAddress(address);
				return;
			}
			switch (function)
			{
				case 0:
				{
					const unsigned char shift = 4;
					const unsigned char orientationF0F4 = SetOrientationF0F4Bit(slot, on, shift);
					SendLocoOrientationF0F4(slot, orientationF0F4);
					break;
				}

				case 1:
				case 2:
				case 3:
				case 4:
				{
					const unsigned char shift = function - 1;
					const unsigned char orientationF0F4 = SetOrientationF0F4Bit(slot, on, shift);
					SendLocoOrientationF0F4(slot, orientationF0F4);
					break;
				}

				default:
					return;
			}
		}

		void LocoNet::AccessoryOnOrOff(__attribute__((unused)) const Protocol protocol,
			const Address address,
			const DataModel::AccessoryState state,
			const bool on)
		{
			logger->Info(Languages::TextSettingAccessoryOnOff, address, Languages::GetGreenRed(state), Languages::GetOnOff(on));
			const Address addressLocoNet = address - 1;
			unsigned char addressLow = static_cast<unsigned char>(addressLocoNet & 0x007F);
			unsigned char addressHigh = static_cast<unsigned char>(((addressLocoNet >> 7) & 0x000F) | ((state & 0x01) << 5) | ((on & 0x01) << 4));
			Send4ByteCommand(OPC_SW_REQ, addressLow, addressHigh);
		}

		void LocoNet::Receiver()
		{
			Utils::Utils::SetThreadName("LocoNet Receiver");
			logger->Info(Languages::TextReceiverThreadStarted);

			const unsigned char BufferSize = 128u;
			unsigned char buffer[BufferSize];
			unsigned char checkSumCalculated;
			unsigned char commandLength;
			while (run)
			{
				if (!serialLine.IsConnected())
				{
					logger->Error(Languages::TextUnableToReceiveData);
					return;
				}
				ssize_t dataLength = serialLine.ReceiveExact(buffer, 1);
				if (dataLength != 1)
				{
					continue;
				}
				unsigned char commandType = buffer[0] & 0xE0;
				switch (commandType)
				{
					case 0x80:
						dataLength = serialLine.ReceiveExact(buffer + 1, 1);
						if (dataLength != 1)
						{
							continue;
						}
						commandLength = 2;
						break;

					case 0xA0:
					{
						dataLength = serialLine.ReceiveExact(buffer + 1, 3);
						if (dataLength != 3)
						{
							continue;
						}
						commandLength = 4;
						break;
					}

					case 0xC0:
					{
						dataLength = serialLine.ReceiveExact(buffer + 1, 5);
						if (dataLength != 5)
						{
							continue;
						}
						commandLength = 6;
						break;
					}

					case 0xE0:
					{
						dataLength = serialLine.ReceiveExact(buffer + 1, 1);
						if (dataLength != 1)
						{
							continue;
						}
						unsigned char dataSize = buffer[1];
						dataLength = serialLine.ReceiveExact(buffer + 2, dataSize + 1);
						if (dataLength != dataSize)
						{
							continue;
						}
						commandLength = dataSize + 2;
						break;
					}

					default:
						continue;
				}
				logger->Hex(buffer, commandLength);
				CalcCheckSum(buffer, commandLength - 1, &checkSumCalculated);
				if (checkSumCalculated != buffer[commandLength - 1])
				{
					continue;
				}

				if (run == false)
				{
					break;
				}

				Parse(buffer);
			}
			logger->Info(Languages::TextTerminatingReceiverThread);
		}

		void LocoNet::CalcCheckSum(unsigned char* data, const unsigned char length, unsigned char* checkSum)
		{
			*checkSum = 0xFF;
			for (unsigned char i = 0; i < length; ++i)
			{
				(*checkSum) ^= data[i];
			}
		}

		void LocoNet::Parse(unsigned char* data)
		{
			switch (data[0])
			{
				case OPC_GPON:
					logger->Info(Languages::TextTurningBoosterOn);
					manager->Booster(ControlTypeHardware, BoosterStateGo);
					break;

				case OPC_GPOFF:
					logger->Info(Languages::TextTurningBoosterOff);
					manager->Booster(ControlTypeHardware, BoosterStateStop);
					break;

				case OPC_SW_REQ:
				{
					const bool on = static_cast<bool>((data[2] & 0x10) >> 4);
					if (!on)
					{
						break;
					}
					const DataModel::AccessoryState state = static_cast<DataModel::AccessoryState>((data[2] & 0x20) >> 5);
					const Address address = (static_cast<Address>(data[1] & 0x7F) | (static_cast<Address>(data[2] & 0x0F) << 7)) + 1;
					logger->Info(Languages::TextSettingAccessory, address, Languages::GetGreenRed(state));
					manager->AccessoryState(ControlTypeHardware, controlID, ProtocolServer, address, state);
					break;
				}

				case OPC_SL_RD_DATA:
				{
					if (data[1] != 0x0E)
					{
						break;
					}
					const unsigned char slot = data[2];
					const Address address = static_cast<Address>(data[4] & 0x7F) | (static_cast<Address>(data[9] & 0x3F) << 7);
					logger->Debug("Slot {0} has address {1}", slot, address);
					locoCache.SetAddress(slot, address);
					ParseSpeed(address, data[5]);
					ParseOrientationF0F4(slot, address, data[6]);
					break;
				}

				case OPC_LOCO_SPD:
				{
					const unsigned char slot = data[1];
					const Address address = locoCache.GetAddressOfSlot(slot);
					if (0 == address)
					{
						SendRequestLocoData(slot);
						break;
					}
					ParseSpeed(address, data[2]);
					break;
				}

				case OPC_LOCO_DIRF:
				{
					const unsigned char slot = data[1];
					const Address address = locoCache.GetAddressOfSlot(slot);
					if (0 == address)
					{
						SendRequestLocoData(slot);
						break;
					}
					ParseOrientationF0F4(slot, address, data[2]);
					break;
				}

				default:
					break;
			}
		}

		void LocoNet::ParseSpeed(const Address address, const unsigned char data)
		{
			Speed speed = static_cast<Speed>(data);
			if (speed)
			{
				--speed;
			}
			speed <<= 3;
			logger->Info(Languages::TextSettingSpeed, address, speed);
			manager->LocoSpeed(ControlTypeHardware, controlID, ProtocolServer, address, speed);

		}

		unsigned char LocoNet::CalcSpeed(const Speed speed)
		{
			Speed calculatedSpeed = speed;
			calculatedSpeed >>= 3;
			if (calculatedSpeed)
			{
				++calculatedSpeed;
			}
			if (calculatedSpeed > 127)
			{
				calculatedSpeed = 127;
			}
			return static_cast<unsigned char>(calculatedSpeed);
		}

		void LocoNet::ParseOrientationF0F4(const unsigned char slot, const Address address, const unsigned char data)
		{
			const unsigned char oldData = locoCache.GetOrientationF0F4(slot);
			const unsigned char dataDiff = data ^ oldData;
			if (dataDiff | 0x20)
			{
				Orientation orientation = static_cast<Orientation>((data >> 5) & 0x01);
				logger->Info(Languages::TextSettingOrientation, address, orientation);
				manager->LocoOrientation(ControlTypeHardware, controlID, ProtocolServer, address, orientation);
			}
			if (dataDiff | 0x10)
			{
				DataModel::LocoFunctionState f0 = static_cast<DataModel::LocoFunctionState>((data >> 4) & 0x01);
				logger->Info(Languages::TextSettingFunction, 0, address, f0);
				manager->LocoFunctionState(ControlTypeHardware, controlID, ProtocolServer, address, 0, f0);
			}
			if (dataDiff | 0x01)
			{
				DataModel::LocoFunctionState f1 = static_cast<DataModel::LocoFunctionState>((data >> 0) & 0x01);
				logger->Info(Languages::TextSettingFunction, 1, address, f1);
				manager->LocoFunctionState(ControlTypeHardware, controlID, ProtocolServer, address, 1, f1);
			}
			if (dataDiff | 0x02)
			{
				DataModel::LocoFunctionState f2 = static_cast<DataModel::LocoFunctionState>((data >> 1) & 0x01);
				logger->Info(Languages::TextSettingFunction, 2, address, f2);
				manager->LocoFunctionState(ControlTypeHardware, controlID, ProtocolServer, address, 2, f2);
			}
			if (dataDiff | 0x04)
			{
				DataModel::LocoFunctionState f3 = static_cast<DataModel::LocoFunctionState>((data >> 2) & 0x01);
				logger->Info(Languages::TextSettingFunction, 3, address, f3);
				manager->LocoFunctionState(ControlTypeHardware, controlID, ProtocolServer, address, 3, f3);
			}
			if (dataDiff | 0x08)
			{
				DataModel::LocoFunctionState f4 = static_cast<DataModel::LocoFunctionState>((data >> 3) & 0x01);
				logger->Info(Languages::TextSettingFunction, 4, address, f4);
				manager->LocoFunctionState(ControlTypeHardware, controlID, ProtocolServer, address, 4, f4);
			}
		}

		void LocoNet::Send2ByteCommand(const unsigned char data)
		{
			unsigned char buffer[2];
			buffer[0] = data;
			CalcCheckSum(buffer, 1, buffer + 1);
			logger->Hex(buffer, sizeof(buffer));
			serialLine.Send(buffer, sizeof(buffer));
		}

		void LocoNet::Send4ByteCommand(const unsigned char data0,
			const unsigned char data1,
			const unsigned char data2)
		{
			unsigned char buffer[4];
			buffer[0] = data0;
			buffer[1] = data1;
			buffer[2] = data2;
			CalcCheckSum(buffer, 3, buffer + 3);
			logger->Hex(buffer, sizeof(buffer));
			serialLine.Send(buffer, sizeof(buffer));
		}

		unsigned char LocoNet::SetOrientationF0F4Bit(const unsigned char slot, const bool on, const unsigned char shift)
		{
			unsigned char data = locoCache.GetOrientationF0F4(slot);
			data &= (~(0x01 << shift));
			data |= (on << shift);
			locoCache.SetOrientationF0F4(slot, data);
			return data;
		}
	} // namespace
} // namespace
