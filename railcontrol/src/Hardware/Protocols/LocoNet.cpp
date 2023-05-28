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
			serialLine(logger, params->GetArg1(), dataSpeed, 8, 'N', 1),
			lastCv(0),
			isProgramming(false)
		{
			receiverThread = std::thread(&Hardware::Protocols::LocoNet::Receiver, this);
			senderThread = std::thread(&Hardware::Protocols::LocoNet::Sender, this);
		}

		LocoNet::~LocoNet()
		{
			run = false;
			sendingQueue.Enqueue(SendingQueueEntry());
			senderThread.join();
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
					return;
				}

				case 1:
				case 2:
				case 3:
				case 4:
				{
					const unsigned char shift = function - 1;
					const unsigned char orientationF0F4 = SetOrientationF0F4Bit(slot, on, shift);
					SendLocoOrientationF0F4(slot, orientationF0F4);
					return;
				}

				case 5:
				case 6:
				case 7:
				case 8:
				{
					const unsigned char shift = function - 5;
					const unsigned char f5F8 = SetF5F8Bit(slot, on, shift);
					SendLocoF5F8(slot, f5F8);
					return;
				}

				case 9:
				case 10:
				case 11:
				case 12:
				{
					const unsigned char shift = function - 9;
					const unsigned char f9F12 = SetF9F12Bit(slot, on, shift);
					SendLocoF9F12(slot, f9F12);
					return;
				}

				default:
					break;
			}
			// else function > 12
			// Sending function numbers bigger than 12 is an Uhlenbrock Intellibox II extension

			if (function > 28)
			{
				// even Uhlenbrock Intellibox does not interpret functions > 28 correct
				return;
			}

			const unsigned char shift = function - 13;
			const uint32_t data = SetF13F44Bit(slot, on, shift);
			if (function < 20)
			{
				const uint8_t data4 = static_cast<uint8_t>(data & 0x7F);
				Send6ByteCommand(OPC_EXP_CMD, 0x20, 0x01, 0x08, data4);
				return;
			}
			if (function == 20 || function == 28)
			{
				const uint8_t data4 = static_cast<uint8_t>(((data >> 2) & 0x20) | ((data >> 9) & 0x40));
				Send6ByteCommand(OPC_EXP_CMD, 0x20, 0x01, 0x05, data4);
				return;
			}

			// else function > 21 && function < 28
			const uint8_t data4 = static_cast<uint8_t>((data >> 8) & 0x7F);
			Send6ByteCommand(OPC_EXP_CMD, 0x20, 0x01, 0x09, data4);
			return;

			// else function > 28
			// Actually Uhlenbrock Intellibox II does not interpret and store these values.
			// Therefore we don't send the values.
			// const uint8_t data1 = static_cast<uint8_t>((data >> (function - 17	)) & 0x10);
			// const uint8_t data4 = static_cast<uint8_t>(function - 29);
			// Send6ByteCommand(OPC_LOCO_FUNC2, data1, 0x01, 0x1D, data4);
			// return;
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

		void LocoNet::ProgramWrite(const ProgramMode mode,
			__attribute__((unused)) const Address address,
			const CvNumber cv,
			const CvValue value)
		{
			switch(mode)
			{
				case ProgramModeMm:
				case ProgramModeDccRegister:
				case ProgramModeDccPage:
				case ProgramModeDccDirect:
					ProgramStart();
					ProgramPT(true, mode, cv, value);
					lastCv = cv;
					break;

				/**
				 * actually not used
				case ProgramModeDccPomLoco:
					ProgramMain(address, cv, value);
					lastCv = cv;
					break;
				 */

				default:
					break;
			}
		}

		void LocoNet::ProgramRead(const ProgramMode mode,
			__attribute__((unused)) const Address address,
			const CvNumber cv)
		{
			switch(mode)
			{
				case ProgramModeDccRegister:
				case ProgramModeDccPage:
				case ProgramModeDccDirect:
					ProgramStart();
					ProgramPT(false, mode, cv);
					lastCv = cv;
					break;

				default:
					break;
			}
		}

		void LocoNet::ProgramStart()
		{
			unsigned char data[0x07];
			data[0] = 0xE5;
			data[1] = 0x07;
			data[2] = 0x01;
			data[3] = 0x49;
			data[4] = 0x42;
			data[5] = 0x41;
			// data[6] is set in SendXByteCommand

			SendXByteCommand(data, sizeof(data));
			isProgramming = true;
		}

		void LocoNet::ProgramEnd()
		{
			unsigned char data[0x07];
			data[0] = 0xE5;
			data[1] = 0x07;
			data[2] = 0x01;
			data[3] = 0x49;
			data[4] = 0x42;
			data[5] = 0x40;
			// data[6] is set in SendXByteCommand

			SendXByteCommand(data, sizeof(data));
			isProgramming = false;
		}

		/**
 	 	 * Reverse Engineered with a KM1 System control 7, but it does not work properly.
		void LocoNet::ProgramMain(const Address address,
			const CvNumber cv,
			const CvValue value)
		{
			unsigned char data[0x1F]; // 31 bytes
			data[0] = OPC_PROGRAM;
			data[1] = 0x1F;
			data[2] = 0x01;
			data[3] = 0x49;
			data[4] = 0x42;
			data[5] = 0x71 | ((cv >> 4) & 0x08) | ((address >> 6) & 0x02);
			data[6] = 0x5E;
			data[7] = (address & 0x7F);
			data[8] = ((address >> 1) & 0x7F);
			data[9] = (cv & 0x7F);
			data[10] = 0x00;
			data[11] = ((cv >> 8) & 0x7F);
			data[12] = (value & 0x7F);
			data[13] = 0x00;
			data[14] = 0x00;
			data[15] = 0x10;
			data[16] = 0x00;
			data[17] = 0x00;
			data[18] = 0x00;
			data[19] = 0x00;
			data[20] = 0x00;
			data[21] = 0x00;
			data[22] = 0x00;
			data[23] = 0x00;
			data[24] = 0x00;
			data[25] = 0x00;
			data[26] = 0x00;
			data[27] = 0x00;
			data[28] = 0x00;
			data[29] = 0x00;
			data[30] = 0x00;
			//data[31] is set in SendXByteCommand

	        SendXByteCommand(data, sizeof(data));
		}
		*/

		void LocoNet::ProgramPT(const bool write,
			const ProgramMode mode,
			const CvNumber cv,
			const CvValue value)
		{
			unsigned char data[0x1F]; // 31 bytes
			switch(mode)
			{
				case ProgramModeMm:
					data[6] = 0x62;
					break;

				case ProgramModeDccRegister:
					data[6] = 0x6C;
					break;

				case ProgramModeDccPage:
					data[6] = 0x6E;
					break;

				case ProgramModeDccDirect:
					data[6] = 0x70;
					break;

				default:
					return;
			}
			data[0] = OPC_PROGRAM;
			data[1] = 0x1F;
			data[2] = 0x01;
			data[3] = 0x49;
			data[4] = 0x42;
			data[5] = 0x71 | ((value >> 4) & 0x08) | ((cv >> 6) & 0x02);
			data[6] |= static_cast<unsigned char>(write);
			data[7] = (cv & 0x7F);
			data[8] = (cv >> 8);
			data[9] = (value & 0x7F);
			data[10] = 0x00;
			data[11] = 0x00;
			data[12] = 0x00;
			data[13] = 0x00;
			data[14] = 0x00;
			data[15] = 0x10;
			data[16] = 0x00;
			data[17] = 0x00;
			data[18] = 0x00;
			data[19] = 0x00;
			data[20] = 0x00;
			data[21] = 0x00;
			data[22] = 0x00;
			data[23] = 0x00;
			data[24] = 0x00;
			data[25] = 0x00;
			data[26] = 0x00;
			data[27] = 0x00;
			data[28] = 0x00;
			data[29] = 0x00;
			data[30] = 0x00;
			//data[31] is set in SendXByteCommand

	        SendXByteCommand(data, sizeof(data));
		}

		/*
		 * This code is written according the Digitrax LocoNet Persional Use Edition 1.0
		 * It does not work with Uhlenbrock controls...

		void LocoNet::Program(const bool write,
			const ProgramMode mode,
			const Address address,
			const CvNumber cv,
			const CvValue value)
		{
			unsigned char data[0x0E]; // 14 bytes
			data[0] = OPC_WR_SL_DATA;
			// data[1] is set in SendXByteCommand with sizeof(data)
			data[2] = ProgrammingSlot; // slot 124
			// data[3]
			switch(mode)
			{
				case ProgramModeDccRegister:
					data[3] = 0x33; // reserved D0 and D1 must be set to 1
					data[5] = 0x00;
					data[6] = 0x00;
					break;

				case ProgramModeDccPage:
					data[3] = 0x23; // reserved D0 and D1 must be set to 1
					data[5] = 0x00;
					data[6] = 0x00;

				case ProgramModeDccDirect:
					data[3] = 0x2B; // reserved D0 and D1 must be set to 1
					data[5] = 0x00;
					data[6] = 0x00;
					break;

				case ProgramModeDccPomLoco:
					data[3] = 0x27;
					data[5] = static_cast<unsigned char>((address >> 7) & 0x7F);
					data[6] = static_cast<unsigned char>(address & 0x7F);
					break;

				default:
					return;
			}
			data[3] |= (static_cast<unsigned char>(write) << 6);

			data[4] = 0x00;
			// TRK
			data[7] = 0x00;
			// CVH
			data[8] = ((cv >> 4) & 0x30) | ((cv >> 7) & 0x01) | ((value >> 6) & 0x02);
			// CVL
			data[9] = static_cast<unsigned char>(cv & 0x7F);
			// DATA7
			data[10] = static_cast<unsigned char>(value & 0x7F);
			data[11] = 0x00;
			data[12] = 0x00;
			// data[13] is checksum and is calculated in SendXByteCommand

			SendXByteCommand(data, sizeof(data));
		}
		*/

		void LocoNet::Sender()
		{
			Utils::Utils::SetThreadName("LocoNet Sender");
			logger->Info(Languages::TextSenderThreadStarted);
			while (run)
			{
				{
					std::unique_lock<std::mutex> lock(entryToVerifyMutex);
					while (entryToVerify.GetSize() > 0)
					{
						// we wait 1s for the response from LocoNet
						if (entryToVerifyCV.wait_for(lock, std::chrono::seconds(1)) == std::cv_status::timeout)
						{
							break;
						}
					}
				}

				SendingQueueEntry temp = sendingQueue.Dequeue();
				{
					std::unique_lock<std::mutex> lock(entryToVerifyMutex);
					entryToVerify = temp;
				}

				const unsigned char size = temp.GetSize();
				if (!size)
				{
					continue;
				}

				const unsigned char* data = temp.GetData();

				logger->Hex(data, size);
				serialLine.Send(data, size);
			}
			logger->Info(Languages::TextTerminatingSenderThread);
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
						commandLength = buffer[1];
						if (commandLength < 3)
						{
							continue;
						}
						unsigned char dataToRead = commandLength - 2;
						dataLength = serialLine.ReceiveExact(buffer + 2, dataToRead);
						if (dataLength != dataToRead)
						{
							continue;
						}
						break;
					}

					default:
						continue;
				}
				CalcCheckSum(buffer, commandLength - 1, &checkSumCalculated);
				if (checkSumCalculated != buffer[commandLength - 1])
				{
					continue;
				}

				{
					std::unique_lock<std::mutex> lock(entryToVerifyMutex);
					const unsigned char size = entryToVerify.GetSize();
					if (size && memcmp(buffer, entryToVerify.GetData(), size) == 0)
					{
						// response of sent command received
						entryToVerify.Reset();
						entryToVerifyCV.notify_all();
						continue;
					}
				}

				if (run == false)
				{
					break;
				}

				logger->Hex(buffer, commandLength);
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
				case OPC_GPON: // 0x83
					logger->Info(Languages::TextTurningBoosterOn);
					manager->Booster(ControlTypeHardware, BoosterStateGo);
					return;

				case OPC_GPOFF: // 0x82
					logger->Info(Languages::TextTurningBoosterOff);
					manager->Booster(ControlTypeHardware, BoosterStateStop);
					return;

				case OPC_LOCO_SPD: // 0xA0
				{
					const Address address = CheckSlot(data[1]);
					if (!address)
					{
						return;
					}
					ParseSpeed(address, data[2]);
					return;
				}

				case OPC_LOCO_DIRF: // 0xA1
				{
					const unsigned char slot = data[1];
					const Address address = CheckSlot(slot);
					if (!address)
					{
						return;
					}
					ParseOrientationF0F4(slot, address, data[2]);
					return;
				}

				case OPC_LOCO_SND: // 0xA2
				{
					const unsigned char slot = data[1];
					const Address address = CheckSlot(slot);
					if (!address)
					{
						return;
					}
					ParseF5F8(slot, address, data[2]);
					return;
				}

				case OPC_LOCO_FUNC: // 0xA3
				{
					const unsigned char slot = data[1];
					const Address address = CheckSlot(slot);
					if (!address)
					{
						return;
					}
					ParseF9F12(slot, address, data[2]);
					return;
				}

				case OPC_SW_REQ: // 0xB0
				{
					const bool on = static_cast<bool>((data[2] & 0x10) >> 4);
					if (!on)
					{
						return;
					}
					const DataModel::AccessoryState state = static_cast<DataModel::AccessoryState>((data[2] & 0x20) >> 5);
					const Address address = (static_cast<Address>(data[1] & 0x7F) | (static_cast<Address>(data[2] & 0x0F) << 7)) + 1;
					logger->Info(Languages::TextSettingAccessory, address, Languages::GetGreenRed(state));
					manager->AccessoryState(ControlTypeHardware, controlID, ProtocolServer, address, state);
					return;
				}

				case OPC_INPUT_REP: // 0xB2
				{
					ParseSensorData(data);
					return;
				}

				case OPC_LONG_ACK: // 0xB4
					// Long ACK is not parsed
					return;

				case OPC_EXP_CMD: // 0xD4
				{
					// This is an Uhlenbrock Intellibox II extension
					const unsigned char slot = data[2];
					const Address address = CheckSlot(slot);
					if (!address)
					{
						return;
					}
					ParseF13F44(slot, address, data);
					return;
				}

				case OPC_PEER_XFER: // 0xE5
					// Peer Xfer is not parsed
					return;

				case OPC_SL_RD_DATA: // 0xE7
					ParseSlotReadData(data);
					return;

				case OPC_PROGRAM: // 0xED
					ParseProgram(data);
					return;

				default:
					logger->Debug(Languages::TextCommandUnknown, Utils::Utils::IntegerToHex(data[0]));
					return;
			}
		}

		Address LocoNet::CheckSlot(const unsigned char slot)
		{
			const Address address = locoCache.GetAddressOfSlot(slot);
			if (0 == address)
			{
				SendRequestLocoData(slot);
				return 0;
			}
			return address;
		}

		void LocoNet::ParseSlotReadData(const unsigned char* data)
		{
			const unsigned char dataLength = data[1];
			if (dataLength != 0x0E)
			{
				return;
			}
			const unsigned char slot = data[2];
			switch (slot)
			{
				case SlotHardwareType:
					ParseSlotHardwareType(data);
					return;

				case SlotProgramming:
					ParseSlotProgramming(data);
					return;

				default:
					ParseSlotLocoData(data);
					return;
			}
		}

		void LocoNet::ParseSlotHardwareType(const unsigned char* data)
		{
			const unsigned char ib[] = { 0x00, 0x00, 0x02, 0x00, 0x07, 0x00, 0x00, 0x00, 0x49, 0x42, 0x18 };
			if (memcmp(ib, data + 3, 11) == 0)
			{
				logger->Info(Languages::TextConnectedTo, "Intellibox / TwinCenter");
				return;
			}

			const unsigned char ib2[] = { 0x02, 0x42, 0x03, 0x00, 0x07, 0x00, 0x00, 0x15, 0x49, 0x42, 0x4C };
			if (memcmp(ib2, data + 3, 11) == 0)
			{
				logger->Info(Languages::TextConnectedTo, "Intellibox II / IB-Basic / IB-Com");
				return;
			}

			const unsigned char sc7[] = { 0x02, 0x42, 0x03, 0x00, 0x06, 0x00, 0x00, 0x15, 0x49, 0x42, 0x4D };
			if (memcmp(sc7, data + 3, 11) == 0)
			{
				logger->Info(Languages::TextConnectedTo, "System Control 7");
				return;
			}

			const unsigned char daisy[] = { 0x00, 0x44, 0x02, 0x00, 0x07, 0x00, 0x59, 0x01, 0x49, 0x42, 0x04 };
			if (memcmp(daisy, data + 3, 11) == 0)
			{
				logger->Info(Languages::TextConnectedTo, "Daisy");
				return;
			}

			const unsigned char adapter63820[] = { 0x00, 0x4C, 0x01, 0x00, 0x07, 0x00, 0x49, 0x02, 0x49, 0x42, 0x1C };
			if (memcmp(adapter63820, data + 3, 11) == 0)
			{
				logger->Info(Languages::TextConnectedTo, "Adapter 63820");
				return;
			}

			const unsigned char digitraxChief[] = { 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11 };
			if (memcmp(digitraxChief, data + 3, 11) == 0)
			{
				logger->Info(Languages::TextConnectedTo, "Digitrax Chief");
				return;
			}

			logger->Info(Languages::TextUnknownHardware);
		}

		void LocoNet::ParseSlotProgramming(const unsigned char* data)
		{
			if (data[4] != 0)
			{
				return;
			}
			CvNumber cv = (static_cast<CvNumber>(data[9]) & 0x7F) | ((static_cast<CvNumber>(data[8]) & 0x01) << 7) | ((static_cast<CvNumber>(data[8]) & 0x30) << 5);
			if (cv == 0)
			{
				cv = lastCv;
			}

			const CvValue value =  (static_cast<CvValue>(data[10]) & 0x7F) | ((static_cast<CvValue>(data[8]) & 0x02) << 6);
			logger->Info(Languages::TextProgramReadValue, cv, value);
			manager->ProgramValue(cv, value);

			if (isProgramming)
			{
				ProgramEnd();
			}
		}

		void LocoNet::ParseSlotLocoData(const unsigned char* data)
		{
			const unsigned char slot = data[2];
			const Address address = ParseLocoAddress(data[4], data[9]);
			logger->Debug(Languages::TextSlotHasAddress, slot, address);

			locoCache.SetAddress(slot, address);
			ParseSpeed(address, data[5]);
			ParseOrientationF0F4(slot, address, data[6]);
			ParseF5F8(slot, address, data[10]);
		}

		void LocoNet::ParseProgram(const unsigned char* data)
		{
			if (data[1] != 0x1F)
			{
				return;
			}
			switch (data[6])
			{
				/*
				 * actually not used because it can't be verified
				case 0x5E:
					// PoM
					lastCv = static_cast<CvNumber>(data[9] & 0x7F) | static_cast<CvNumber>((data[5] & 0x08) << 4) | static_cast<CvNumber>((data[11] & 0x7F) << 8);
					break;
				 */

				case 0x6C: // register read
				case 0x6D: // register write
				case 0x6E: // page read
				case 0x6F: // page write
				case 0x70: // direct read
				case 0x71: // direct write
					// Program track
					lastCv = static_cast<CvNumber>(data[7] & 0x7F) | static_cast<CvNumber>((data[5] & 0x08) << 4) | static_cast<CvNumber>((data[8] & 0x7F) << 8);
					break;

				default:
					// unknown programming method
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

		void LocoNet::ParseOrientationF0F4(const unsigned char slot, const Address address, const uint8_t data)
		{
			const uint8_t oldData = locoCache.GetOrientationF0F4(slot);
			const uint8_t dataDiff = data ^ oldData;
			locoCache.SetOrientationF0F4(slot, data);
			if (dataDiff & 0x20)
			{
				Orientation orientation = static_cast<Orientation>((data >> 5) & 0x01);
				logger->Info(Languages::TextSettingOrientation, address, orientation);
				manager->LocoOrientation(ControlTypeHardware, controlID, ProtocolServer, address, orientation);
			}
			if (dataDiff & 0x10)
			{
				ParseFunction(address, data, 0, 4);
			}
			if (dataDiff & 0x01)
			{
				ParseFunction(address, data, 1, 0);
			}
			if (dataDiff & 0x02)
			{
				ParseFunction(address, data, 2, 1);
			}
			if (dataDiff & 0x04)
			{
				ParseFunction(address, data, 3, 2);
			}
			if (dataDiff & 0x08)
			{
				ParseFunction(address, data, 4, 3);
			}
		}

		void LocoNet::ParseF5F8(const unsigned char slot, const Address address, const uint8_t data)
		{
			const uint8_t oldData = locoCache.GetF5F8(slot);
			const uint8_t dataDiff = data ^ oldData;
			locoCache.SetF5F8(slot, data);
			if (dataDiff & 0x01)
			{
				ParseFunction(address, data, 5, 0);
			}
			if (dataDiff & 0x02)
			{
				ParseFunction(address, data, 6, 1);
			}
			if (dataDiff & 0x04)
			{
				ParseFunction(address, data, 7, 2);
			}
			if (dataDiff & 0x08)
			{
				ParseFunction(address, data, 8, 3);
			}
		}

		void LocoNet::ParseF9F12(const unsigned char slot, const Address address, const uint8_t data)
		{
			const uint8_t oldData = locoCache.GetF9F12(slot);
			const uint8_t dataDiff = data ^ oldData;
			locoCache.SetF9F12(slot, data);
			if (dataDiff & 0x01)
			{
				ParseFunction(address, data, 9, 0);
			}
			if (dataDiff & 0x02)
			{
				ParseFunction(address, data, 10, 1);
			}
			if (dataDiff & 0x04)
			{
				ParseFunction(address, data, 11, 2);
			}
			if (dataDiff & 0x08)
			{
				ParseFunction(address, data, 12, 3);
			}
		}

		void LocoNet::ParseFunction(const Address address,
			const uint32_t data,
			const DataModel::LocoFunctionNr nr,
			const uint8_t shift)
		{
			DataModel::LocoFunctionState state = static_cast<DataModel::LocoFunctionState>((data >> shift) & 0x01);
			logger->Info(Languages::TextSettingFunction, nr, address, state);
			manager->LocoFunctionState(ControlTypeHardware, controlID, ProtocolServer, address, nr, state);
		}

		void LocoNet::ParseF13F44(const unsigned char slot, const Address address, const unsigned char* data)
		{
			// This is an Uhlenbrock Intellibox II extension
			const uint32_t oldData = locoCache.GetF13F44(slot);
			uint32_t newData = oldData;
			if (data[1] == 0x20)
			{
				if (data[3] == 0x08)
				{
					newData &= 0xFFFFFF80;
					newData |= (static_cast<uint32_t>(data[4]) & 0x7F);
				}
				else if (data[3] == 0x09)
				{
					newData &= 0xFFFF80FF;
					newData |= ((static_cast<uint32_t>(data[4]) << 8) & 0x7F00);
				}
				else if (data[3] == 0x05)
				{
					newData &= 0xFFFFFF7F;
					newData |= ((static_cast<uint32_t>(data[4]) << 2) & 0x80);
					newData &= 0xFFFF7FFF;
					newData |= ((static_cast<uint32_t>(data[4]) << 9) & 0x8000);
				}
			}
			else if ((data[1] & 0xEF) == 0)
			{
				const uint8_t nr = data[3];
				if (nr < 29 ||  nr > 44)
				{
					return;
				}
				newData &= ~(1 << (nr - 13));
				newData |= (static_cast<uint32_t>(data[1]) << (nr - 17));
			}
			const uint32_t dataDiff = newData ^ oldData;
			locoCache.SetF13F44(slot, newData);
			for (int i = 0; i < 32; ++i)
			{
				if ((dataDiff >> i) & 0x01)
				{
					ParseFunction(address, newData, i + 13, i);
				}
			}
		}

		void LocoNet::ParseSensorData(const unsigned char* data)
		{
			if (!(data[2] & 0x40))
			{
				// control bit not set
				return;
			}
			const FeedbackPin pin = (((static_cast<FeedbackPin>(data[2]) >> 5) & 0x0001)
				| ((static_cast<FeedbackPin>(data[1]) <<1) & 0x00FE)
				| ((static_cast<FeedbackPin>(data[2]) << 8) & 0x7F))
				+ 1; // LocoNet is 0-based, RailControl is 1-based
			DataModel::Feedback::FeedbackState state = static_cast<DataModel::Feedback::FeedbackState>((data[2] >> 4) & 0x01);
			logger->Info(Languages::TextFeedbackChange, pin & 0x000F, pin >> 4, Languages::GetText(state ? Languages::TextOn : Languages::TextOff));
			manager->FeedbackState(controlID, pin, state);
		}

		void LocoNet::Send2ByteCommand(const unsigned char data)
		{
			unsigned char buffer[2];
			buffer[0] = data;
			CalcCheckSum(buffer, 1, buffer + 1);
			sendingQueue.Enqueue(SendingQueueEntry(sizeof(buffer), buffer));
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
			sendingQueue.Enqueue(SendingQueueEntry(sizeof(buffer), buffer));
		}

		void LocoNet::Send6ByteCommand(const unsigned char data0,
			const unsigned char data1,
			const unsigned char data2,
			const unsigned char data3,
			const unsigned char data4)
		{
			unsigned char buffer[6];
			buffer[0] = data0;
			buffer[1] = data1;
			buffer[2] = data2;
			buffer[3] = data3;
			buffer[4] = data4;
			CalcCheckSum(buffer, 5, buffer + 5);
			sendingQueue.Enqueue(SendingQueueEntry(sizeof(buffer), buffer));
		}

		void LocoNet::SendXByteCommand(unsigned char* data, unsigned char dataLength)
		{
			data[1] = dataLength;
			CalcCheckSum(data, dataLength - 1, data + dataLength - 1);
			sendingQueue.Enqueue(SendingQueueEntry(dataLength, data));
		}

		uint8_t LocoNet::SetOrientationF0F4Bit(const unsigned char slot, const bool on, const unsigned char shift)
		{
			uint8_t data = locoCache.GetOrientationF0F4(slot);
			data &= (~(0x01u << shift));
			data |= (on << shift);
			locoCache.SetOrientationF0F4(slot, data);
			return data;
		}

		uint8_t LocoNet::SetF5F8Bit(const unsigned char slot, const bool on, const unsigned char shift)
		{
			uint8_t data = locoCache.GetF5F8(slot);
			data &= (~(0x01u << shift));
			data |= (on << shift);
			locoCache.SetF5F8(slot, data);
			return data;
		}

		uint8_t LocoNet::SetF9F12Bit(const unsigned char slot, const bool on, const unsigned char shift)
		{
			uint8_t data = locoCache.GetF9F12(slot);
			data &= (~(0x01u << shift));
			data |= (on << shift);
			locoCache.SetF9F12(slot, data);
			return data;
		}

		uint32_t LocoNet::SetF13F44Bit(const unsigned char slot, const bool on, const unsigned char shift)
		{
			uint32_t data = locoCache.GetF13F44(slot);
			data &= (~(0x00000001u << shift));
			data |= (on << shift);
			locoCache.SetF13F44(slot, data);
			return data;
		}
	} // namespace
} // namespace
