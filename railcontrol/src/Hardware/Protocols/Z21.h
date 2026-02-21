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

#include <arpa/inet.h>
#include <cstring>
#include <string>

#include "DataModel/AccessoryBase.h"
#include "Hardware/HardwareInterface.h"
#include "Hardware/HardwareParams.h"
#include "Hardware/Protocols/Z21DataTypes.h"
#include "Hardware/Protocols/Z21FeedbackCache.h"
#include "Hardware/Protocols/Z21LocoCache.h"
#include "Hardware/Protocols/Z21TurnoutCache.h"
#include "Logger/Logger.h"
#include "Network/UdpConnection.h"

namespace Z21Enums = Hardware::Protocols::Z21Enums;

// protocol specification at https://www.z21.eu/media/Kwc_Basic_DownloadTag_Component/47-1652-959-downloadTag/default/69bad87e/1558674980/z21-lan-protokoll.pdf

namespace Hardware
{
	namespace Protocols
	{
		class Z21: Hardware::HardwareInterface
		{
			public:
				Z21() = delete;
				Z21(const Z21&) = delete;
				Z21& operator=(const Z21&) = delete;

				Z21(const HardwareParams* params, const std::string& controlName);
				virtual ~Z21();

				inline Hardware::Capabilities GetCapabilities() const override
				{
					return Hardware::CapabilityLoco
						| Hardware::CapabilityAccessory
						| Hardware::CapabilityFeedback
						| Hardware::CapabilityProgram
						| Hardware::CapabilityProgramMmWrite
						| Hardware::CapabilityProgramDccDirectRead
						| Hardware::CapabilityProgramDccDirectWrite
						| Hardware::CapabilityProgramDccPomLocoRead
						| Hardware::CapabilityProgramDccPomLocoWrite
						| Hardware::CapabilityProgramDccPomAccessoryRead
						| Hardware::CapabilityProgramDccPomAccessoryWrite;
				}

				void GetLocoProtocols(std::vector<Protocol>& protocols) const override
				{
					protocols.push_back(ProtocolMM1);
					protocols.push_back(ProtocolMM15);
					protocols.push_back(ProtocolMM2);
					protocols.push_back(ProtocolDCC14);
					protocols.push_back(ProtocolDCC28);
					protocols.push_back(ProtocolDCC128);
				}

				bool LocoProtocolSupported(Protocol protocol) const override
				{
					return (protocol == ProtocolMM1
					    || protocol == ProtocolMM15
					    || protocol == ProtocolMM2
					    || protocol == ProtocolDCC14
					    || protocol == ProtocolDCC28
					    || protocol == ProtocolDCC128);
				}

				void GetAccessoryProtocols(std::vector<Protocol>& protocols) const override
				{
					protocols.push_back(ProtocolMM);
					protocols.push_back(ProtocolDCC);
				}

				bool AccessoryProtocolSupported(Protocol protocol) const override
				{
					return (protocol == ProtocolMM
						|| protocol == ProtocolDCC);
				}

				static void GetArgumentTypesAndHint(std::map<unsigned char, ArgumentType>& argumentTypes,
				    std::string& hint)
				{
					argumentTypes[1] = ArgumentTypeIpAddress;
					hint = Languages::GetText(Languages::TextHintZ21);
				}

				void Booster(const BoosterState status) override;
				void LocoSpeed(const Protocol protocol, const Address address, const Speed speed) override;
				void LocoOrientation(const Protocol protocol, const Address address, const Orientation orientation)
				    override;

				void LocoFunctionState(const Protocol protocol,
				    const Address address,
				    const DataModel::LocoFunctionNr function,
				    const DataModel::LocoFunctionState on) override;

				void LocoSpeedOrientationFunctionStates(const Protocol protocol,
				    const Address address,
				    const Speed speed,
				    const Orientation orientation,
				    const std::vector<DataModel::LocoFunctionEntry>& functions) override;

				void Accessory(const Protocol protocol,
					const Address address,
					const DataModel::AccessoryState state,
					const bool on,
				    const DataModel::AccessoryPulseDuration duration) override;

				void ProgramRead(const ProgramMode mode,
					const Address address,
					const CvNumber cv) override;

				void ProgramWrite(const ProgramMode mode,
					const Address address,
					const CvNumber cv,
					const CvValue value) override;

				static unsigned char EncodeSpeed14(const Speed speed);
				static unsigned char EncodeSpeed28(const Speed speed);
				static unsigned char EncodeSpeed128(const Speed speed);
				static Speed DecodeSpeed14(unsigned char data);
				static Speed DecodeSpeed28(unsigned char data);
				static Speed DecodeSpeed128(unsigned char data);

			private:
				static const unsigned short Z21Port = 21105;
				static const unsigned int Z21CommandBufferLength = 1472; // = Max Ethernet MTU
				static const Address MaxMMAddress = 255;

				class AccessoryQueueEntry
				{
					public:
						inline AccessoryQueueEntry()
							: protocol(ProtocolNone),
							    address(AddressNone),
							    state(DataModel::DefaultState),
							    duration(DataModel::DefaultAccessoryPulseDuration)
						{
						}

						inline AccessoryQueueEntry(const Protocol protocol, const Address address,
						    const DataModel::AccessoryState state, const DataModel::AccessoryPulseDuration duration)
							: protocol(protocol),
							    address(address),
							    state(state),
							    duration(duration)
						{
						}

						Protocol protocol;
						Address address;
						DataModel::AccessoryState state;
						DataModel::AccessoryPulseDuration duration;
				};

				volatile bool run;
				Network::UdpConnection connection;
				std::thread receiverThread;
				std::thread heartBeatThread;
				std::thread accessorySenderThread;
				Z21LocoCache locoCache;
				Z21TurnoutCache turnoutCache;
				Z21FeedbackCache feedbackCache;
				ProgramMode lastProgramMode;
				volatile bool connected;

				Utils::ThreadSafeQueue<AccessoryQueueEntry> accessoryQueue;

				void ProgramMm(const CvNumber cv, const CvValue value);
				void ProgramDccRead(const CvNumber cv);
				void ProgramDccWrite(const CvNumber cv, const CvValue value);
				void ProgramDccPom(const Z21Enums::PomDB0 db0,
					const Z21Enums::PomOption option,
					const Address address, const CvNumber cv,
					const CvValue value = 0);

				void LocoSpeedOrientation(const Protocol protocol,
					const Address address,
					const Speed speed,
					const Orientation orientation);

				void AccessorySender();

				void AccessoryOnOrOff(const Address address,
					const DataModel::AccessoryState state,
					const bool on);

				void HeartBeatSender();

				void Receiver();

				ssize_t ParseData(const unsigned char* buffer, size_t bufferLength);
				void ParseXHeader(const unsigned char* buffer);
				void ParseDB0(const unsigned char* buffer);
				void ParseTurnoutData(const unsigned char* buffer);
				void ParseLocoData(const unsigned char* buffer);
				void ParseCvData(const unsigned char* buffer);
				void ParseRmBusData(const unsigned char* buffer);
				void ParseLocoNetDetector(const unsigned char* buffer);
				void ParseDetectorData(const unsigned char* buffer);

				void StartUpConnection();
				void SendGetSerialNumber();
				void SendGetHardwareInfo();
				void SendGetStatus();
				void SendGetCode();
				void SendGetDetectorState();
				void SendLogOff();
				void SendBroadcastFlags();
				void SendBroadcastFlags(const Z21Enums::BroadCastFlags flags);
				void SendSetMode(const Address address, const Z21Enums::Command command, const Z21Enums::ProtocolMode mode);
				void SendSetLocoMode(const Address address, const Protocol protocol);
				void SendSetLocoModeMM(const Address address);
				void SendSetLocoModeDCC(const Address address);
				void SendSetTurnoutMode(const Address address, const Protocol protocol);
				void SendSetTurnoutModeMM(const Address address);
				void SendSetTurnoutModeDCC(const Address address);
				int Send(const unsigned char* buffer, const size_t bufferLength);

				inline int Send(const char* buffer, const size_t bufferLength)
				{
					return Send(reinterpret_cast<const unsigned char*>(buffer), bufferLength);
				}
		};
	} // namespace
} // namespace

