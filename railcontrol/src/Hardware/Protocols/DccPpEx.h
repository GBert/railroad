/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2021 Dominik (Teddy) Mahrer - www.railcontrol.org

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

#include "Hardware/Capabilities.h"
#include "Hardware/HardwareInterface.h"
#include "Hardware/HardwareParams.h"
#include "Hardware/Protocols/DccPpExLocoCache.h"
#include "Logger/Logger.h"

namespace Hardware
{
	namespace Protocols
	{
		class DccPpEx: protected HardwareInterface
		{
			public:
				DccPpEx() = delete;
				DccPpEx(const DccPpEx&) = delete;
				DccPpEx& operator=(const DccPpEx&) = delete;

				inline Hardware::Capabilities GetCapabilities() const override
				{
					return CapabilityLoco
						| CapabilityAccessory
						| CapabilityFeedback
						| CapabilityProgram
						| CapabilityProgramDccDirectWrite
						| CapabilityProgramDccPomLocoWrite;
				}

				void GetLocoProtocols(std::vector<Protocol>& protocols) const override
				{
					protocols.push_back(ProtocolDCC128);
				}

				inline bool LocoProtocolSupported(Protocol protocol) const override
				{
					return (protocol == ProtocolDCC128);
				}

				inline void GetAccessoryProtocols(std::vector<Protocol>& protocols) const override
				{
					protocols.push_back(ProtocolDCC);
				}

				inline bool AccessoryProtocolSupported(Protocol protocol) const override
				{
					return (protocol == ProtocolDCC);
				}

				void Booster(const BoosterState status) override;

				void LocoSpeed(const Protocol protocol,
					const Address address,
					const Speed speed) override;

				void LocoOrientation(const Protocol protocol,
					const Address address,
					const Orientation orientation)
				    override;

				void LocoFunction(const Protocol protocol,
				    const Address address,
				    const DataModel::LocoFunctionNr function,
				    const DataModel::LocoFunctionState on) override;

				void AccessoryOnOrOff(const Protocol protocol,
					const Address address,
				    const DataModel::AccessoryState state,
				    const bool on) override;

				void ProgramWrite(const ProgramMode mode,
					const Address address,
					const CvNumber cv,
					const CvValue value) override;

				void FeedbackDelete(const FeedbackID feedbackID,
					const std::string& name) override;

				void FeedbackSettings(const FeedbackID feedbackID,
					const std::string& name) override;

			protected:
				inline DccPpEx(const HardwareParams* params,
				    const std::string& fullName,
				    const std::string& shortName)
				:	HardwareInterface(params->GetManager(),
						params->GetControlID(),
						fullName,
						shortName),
					run(true)
				{
					receiverThread = std::thread(&DccPpEx::Receiver, this);
				}

				virtual ~DccPpEx()
				{
					run = false;
					receiverThread.join();
				}

			private:
				void LocoSpeedOrientation(const Address address,
					const Speed speed,
					const Orientation orientation);

				void ProgramWriteMain(const Address address,
					const CvNumber cv,
					const CvValue value);

				void ProgramWriteProgram(const CvNumber cv,
					const CvValue value);

				inline bool SendInternal(const std::string& buffer)
				{
					logger->Hex(buffer);
					return Send(buffer);
				}

				inline bool ReceiveInternal(std::string& buffer)
				{
					const bool ret = Receive(buffer);
					if (buffer.size())
					{
						logger->Hex(buffer);
					}
					return ret;
				}

				virtual bool Send(const std::string& buffer) = 0;

				virtual bool Receive(std::string& buffer) = 0;

				void Receiver();

				bool ReceiveData(std::string& buffer);

				void Parse(const std::string& buffer);

				unsigned int ParseInt(const std::string& buffer, unsigned int& pos);

				DccPpExLocoCache locoCache;

				volatile bool run;
				std::thread receiverThread;
		};
	} // namespace
} // namespace
