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

#include <algorithm>
#include <ctime>

#include "Hardware/HardwareInterface.h"
#include "Hardware/HardwareParams.h"
#include "Hardware/Protocols/LocoNetLocoCache.h"
#include "Logger/Logger.h"
#include "Network/Serial.h"

// Protocol specification at https://www.digitrax.com/static/apps/cms/media/documents/loconet/loconetpersonaledition.pdf
// Programming specification does not fit for Uhlenbrock Intellibox II

namespace Hardware
{
	namespace Protocols
	{
		class LocoNet: Hardware::HardwareInterface
		{
			public:
				LocoNet() = delete;
				LocoNet(const LocoNet&) = delete;
				LocoNet& operator=(const LocoNet&) = delete;

				LocoNet(const HardwareParams* params,
					const std::string& controlName,
					const unsigned int dataSpeed);

				virtual ~LocoNet();

				inline void Start() override
				{
					SendRequestLocoData(0x7F);
					Utils::Utils::SleepForMilliseconds(25);
					SendRequestLocoData(0);
					Utils::Utils::SleepForMilliseconds(25);
				}

				inline Hardware::Capabilities GetCapabilities() const override
				{
					return Hardware::CapabilityLoco
						| Hardware::CapabilityAccessory
						| Hardware::CapabilityFeedback
						| Hardware::CapabilityProgram
						| Hardware::CapabilityProgramMmWrite
						| Hardware::CapabilityProgramDccRegisterRead
						| Hardware::CapabilityProgramDccRegisterWrite
						| Hardware::CapabilityProgramDccPageRead
						| Hardware::CapabilityProgramDccPageWrite
						| Hardware::CapabilityProgramDccDirectRead
						| Hardware::CapabilityProgramDccDirectWrite;
				}

				void GetLocoProtocols(std::vector<Protocol>& protocols) const override
				{
					protocols.push_back(ProtocolServer);
				}

				bool LocoProtocolSupported(Protocol protocol) const override
				{
					return (protocol == ProtocolServer);
				}

				void GetAccessoryProtocols(std::vector<Protocol>& protocols) const override
				{
					protocols.push_back(ProtocolServer);
				}

				bool AccessoryProtocolSupported(Protocol protocol) const override
				{
					return (protocol == ProtocolServer);
				}

				void Booster(const BoosterState status) override;

				void LocoSpeed(const Protocol protocol,
					const Address address,
					const Speed speed) override;

				void LocoOrientation(const Protocol protocol,
					const Address address,
					const Orientation orientation) override;

				void LocoFunctionState(const Protocol protocol,
					const Address address,
					const DataModel::LocoFunctionNr function,
					const DataModel::LocoFunctionState on) override;

				void Accessory(const Protocol protocol,
					const Address address,
					const DataModel::AccessoryState state,
					const bool on,
					__attribute__((unused)) const DataModel::AccessoryPulseDuration duration) override;

				void ProgramWrite(const ProgramMode mode,
					const Address address,
					const CvNumber cv,
					const CvValue value) override;

				void ProgramRead(const ProgramMode mode,
					const Address address,
					const CvNumber cv) override;

			private:
				// longest known LocoNet-Command is OPC_START_PROGRAM (Uhlenbrock Extension)
				static const unsigned char MaxDataLength = 0x1F;

				enum OpCodes : unsigned char
				{
					OPC_BUSY           = 0x81,
					OPC_GPOFF          = 0x82,
					OPC_GPON           = 0x83,
					OPC_IDLE           = 0x85,
					OPC_LOCO_SPD       = 0xA0,
					OPC_LOCO_DIRF      = 0xA1,
					OPC_LOCO_SND       = 0xA2,
					OPC_LOCO_FUNC      = 0xA3,
					OPC_SW_REQ         = 0xB0,
					OPC_SW_REP         = 0xB1,
					OPC_INPUT_REP      = 0xB2,
					OPC_LONG_ACK       = 0xB4,
					OPC_SLOT_STAT1     = 0xB5,
					OPC_CONSIST_FUNC   = 0xB6,
					OPC_UNLINK_SLOTS   = 0xB8,
					OPC_LINK_SLOTS     = 0xB9,
					OPC_MOVE_SLOTS     = 0xBA,
					OPC_RQ_SL_DATA     = 0xBB,
					OPC_SW_STATE       = 0xBC,
					OPC_SW_ACK         = 0xBD,
					OPC_LOCO_ADR       = 0xBF,
					OPC_PEER_XFER      = 0xE5, // Intellibox-II see below
					OPC_SL_RD_DATA     = 0xE7,
					OPC_IMM_PACKET     = 0xED, // Intellibox-II see below
					OPC_WR_SL_DATA     = 0xEF,
					// Intellibox-II codes
					OPC_LOCO_XADR      = 0xBE,
					OPC_EXP_CMD        = 0xD4,
					OPC_START_PROGRAM  = 0xE5, // not documented
					OPC_SL_RD_DATA_EXT = 0xE6,
					OPC_PROGRAM        = 0xED, // not documented
					OPC_WR_SL_DATA_EXT = 0xEE
				};

				static const unsigned char SlotHardwareType = 0;
				static const unsigned char SlotProgramming = 124;

				class SendingQueueEntry
				{
					public:
						inline SendingQueueEntry()
						:	size(0)
						{
						}

						inline SendingQueueEntry(unsigned char size, const unsigned char* data)
						:	size(std::min(size, static_cast<unsigned char>(MaxDataLength)))
						{
							memcpy(this->data, data, this->size);
						}

						inline SendingQueueEntry(const SendingQueueEntry& rhs) = default;

						inline SendingQueueEntry& operator=(const SendingQueueEntry& rhs) = default;

						inline unsigned char GetSize() const
						{
							return size;
						}

						inline const unsigned char* GetData() const
						{
							return data;
						}

						inline void Reset()
						{
							size = 0;
						}

					private:
						volatile unsigned char size;
						unsigned char data[MaxDataLength];
				};

				void Sender();

				void Receiver();

				static void CalcCheckSum(unsigned char* data, const unsigned char length, unsigned char* checkSum);

				void Parse(unsigned char* data);

				Address CheckSlot(const unsigned char slot);

				void ParseSlotReadData(const unsigned char* data);

				void ParseSlotHardwareType(const unsigned char* data);

				void ParseSlotProgramming(const unsigned char* data);

				void ParseSlotLocoData(const unsigned char* data);

				void ParseProgram(const unsigned char* data);

				inline Address ParseLocoAddress(const unsigned char data1, const unsigned char data2)
				{
					return static_cast<Address>(data1 & 0x7F) | (static_cast<Address>(data2 & 0x3F) << 7);
				}

				void ParseSpeed(const Address address, const unsigned char data);

				static unsigned char CalcSpeed(const Speed speed);

				void ParseOrientationF0F4(const unsigned char slot, const Address address, const uint8_t data);

				void ParseF5F8(const unsigned char slot, const Address address, const uint8_t data);

				void ParseF9F12(const unsigned char slot, const Address address, const uint8_t data);

				void ParseFunction(const Address address,
					const uint32_t data,
					const DataModel::LocoFunctionNr nr,
					const uint8_t shift);

				void ParseF13F44(const unsigned char slot, const Address address, const unsigned char* data);

				void ParseSensorData(const unsigned char* data);

				void Send2ByteCommand(const unsigned char data0);

				void Send4ByteCommand(const unsigned char data0,
					const unsigned char data1,
					const unsigned char data2);

				void Send6ByteCommand(const unsigned char data0,
					const unsigned char data1,
					const unsigned char data2,
					const unsigned char data3,
					const unsigned char data4);

				void SendXByteCommand(unsigned char* data, unsigned char dataLength);

				inline void SendRequestLocoData(const unsigned char slot)
				{
					Send4ByteCommand(OPC_RQ_SL_DATA, slot, 0);
				}

				inline void SendLocoAddress(const Address address)
				{
					Send4ByteCommand(OPC_LOCO_ADR,
						static_cast<unsigned char>((address >> 7) & 0x7F),
						static_cast<unsigned char>(address & 0x7F));
				}

				inline void SendLocoSpeed(const unsigned char slot, const Speed speed)
				{
					Send4ByteCommand(OPC_LOCO_SPD, slot, CalcSpeed(speed));
				}

				inline void SendLocoOrientationF0F4(const unsigned char slot, const uint8_t orientationF0F4)
				{
					Send4ByteCommand(OPC_LOCO_DIRF, slot, orientationF0F4);
				}

				inline void SendLocoF5F8(const unsigned char slot, const uint8_t f5F8)
				{
					Send4ByteCommand(OPC_LOCO_SND, slot, f5F8);
				}

				inline void SendLocoF9F12(const unsigned char slot, const uint8_t f9F12)
				{
					Send4ByteCommand(OPC_LOCO_FUNC, slot, f9F12);
				}

				uint8_t SetOrientationF0F4Bit(const unsigned char slot, const bool on, const unsigned char shift);

				uint8_t SetF5F8Bit(const unsigned char slot, const bool on, const unsigned char shift);

				uint8_t SetF9F12Bit(const unsigned char slot, const bool on, const unsigned char shift);

				uint32_t SetF13F44Bit(const unsigned char slot, const bool on, const unsigned char shift);

				void ProgramStart();

				void ProgramEnd();

//				void ProgramMain(const Address address,
//					const CvNumber cv,
//					const CvValue value);

				void ProgramPT(const bool write,
					const ProgramMode mode,
					const CvNumber cv,
					const CvValue value = 0);

				volatile bool run;
				mutable Network::Serial serialLine;
				std::thread senderThread;
				std::thread receiverThread;

				LocoNetLocoCache locoCache;

				Utils::ThreadSafeQueue<SendingQueueEntry> sendingQueue;
				SendingQueueEntry entryToVerify;
				mutable std::mutex entryToVerifyMutex;
				std::condition_variable entryToVerifyCV;

				CvNumber lastCv;
				bool isProgramming;
		};
	} // namespace
} // namespace

