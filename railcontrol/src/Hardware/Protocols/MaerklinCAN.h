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

#pragma once

#include "DataModel/AccessoryBase.h"
#include "DataModel/LocoFunctions.h"
#include "Hardware/Capabilities.h"
#include "Hardware/HardwareInterface.h"
#include "Hardware/HardwareParams.h"
#include "Hardware/LocoCache.h"
#include "Hardware/Protocols/MaerklinCANCommon.h"
#include "Logger/Logger.h"
#include "Utils/Integer.h"

// CAN protocol specification at http://streaming.maerklin.de/public-media/cs2/cs2CAN-Protokoll-2_0.pdf
// Very interesting is also http://www.mbernstein.de/modellbahn/can/bem.htm

namespace Hardware { namespace Protocols
{
	class MaerklinCAN: protected HardwareInterface, protected MaerklinCANCommon
	{
		public:
			MaerklinCAN() = delete;
			MaerklinCAN(const MaerklinCAN&) = delete;
			MaerklinCAN& operator=(const MaerklinCAN&) = delete;

			inline Hardware::Capabilities GetCapabilities() const override
			{
				return Hardware::CapabilityLoco
					| Hardware::CapabilityAccessory
					| Hardware::CapabilityFeedback
					| Hardware::CapabilityProgram
					| Hardware::CapabilityProgramMmWrite
					| Hardware::CapabilityProgramMfxRead
					| Hardware::CapabilityProgramMfxWrite
					| Hardware::CapabilityProgramDccDirectRead
					| Hardware::CapabilityProgramDccDirectWrite
					| Hardware::CapabilityProgramDccPomLocoWrite
					| Hardware::CapabilityProgramDccPomAccessoryWrite
					| Hardware::CapabilityLocoDatabase;
			}

			void GetLocoProtocols(std::vector<Protocol>& protocols) const override
			{
				protocols.push_back(ProtocolMM);
				protocols.push_back(ProtocolMFX);
				protocols.push_back(ProtocolDCC);
			}

			inline bool LocoProtocolSupported(Protocol protocol) const override
			{
				return ((protocol == ProtocolMM)
					|| (protocol == ProtocolMFX)
					|| (protocol == ProtocolDCC));
			}

			inline void GetAccessoryProtocols(std::vector<Protocol>& protocols) const override
			{
				protocols.push_back(ProtocolMM);
				protocols.push_back(ProtocolDCC);
			}

			inline bool AccessoryProtocolSupported(Protocol protocol) const override
			{
				return ((protocol == ProtocolMM)
					|| (protocol == ProtocolDCC));
			}

			void Booster(const BoosterState status) override
			{
				MaerklinCANCommon::Booster(status);
			}

			void LocoSpeed(const Protocol protocol,
				const Address address,
				const Speed speed) override
			{
				MaerklinCANCommon::LocoSpeed(protocol, address, speed);
			}
			void LocoOrientation(const Protocol protocol,
				const Address address,
				const Orientation orientation) override
			{
				MaerklinCANCommon::LocoOrientation(protocol, address, orientation);
			}

			void LocoFunction(const Protocol protocol,
				const Address address,
				const DataModel::LocoFunctionNr function,
				const DataModel::LocoFunctionState on) override
			{
				MaerklinCANCommon::LocoFunction(protocol, address, function, on);
			}

			void Accessory(const Protocol protocol,
				const Address address,
				const DataModel::AccessoryState state,
				const bool on,
				__attribute__((unused)) const DataModel::AccessoryPulseDuration duration) override
			{
				MaerklinCANCommon::Accessory(protocol, address, state, on);
			}

			void ProgramRead(const ProgramMode mode,
				const Address address,
				const CvNumber cv) override
			{
				MaerklinCANCommon::ProgramRead(mode, address, cv);
			}

			void ProgramWrite(const ProgramMode mode,
				const Address address,
				const CvNumber cv,
				const CvValue value) override
			{
				MaerklinCANCommon::ProgramWrite(mode, address, cv, value);
			}

			const std::map<std::string, Hardware::LocoCacheEntry>& GetLocoDatabase() const override
			{
				return locoCache.GetAll();
			}

			DataModel::LocoConfig GetLocoByMatchKey(const std::string& matchKey) const override
			{
				return DataModel::LocoConfig(locoCache.Get(matchKey));
			}

			DataModel::LocoConfig GetMultipleUnitByMatchKey(const std::string& matchKey) const override
			{
				return DataModel::LocoConfig(locoCache.Get(matchKey));
			}

			void SetLocoIdOfMatchKey(const LocoID locoId, const std::string& matchKey) override
			{
				locoCache.SetLocoId(locoId, matchKey);
			}

			void SetMultipleUnitIdOfMatchKey(const LocoID locoId, const std::string& matchKey) override
			{
				locoCache.SetLocoId(locoId, matchKey);
			}

		protected:
			inline MaerklinCAN(const HardwareParams* params,
				const std::string& fullName,
				const std::string& shortName)
			:	HardwareInterface(params->GetManager(),
					params->GetControlID(),
					fullName,
					shortName),
				MaerklinCANCommon("affeaffe",
					params->GetControlID(),
					params->GetManager(),
					Utils::Utils::StringToBool(params->GetArg2()),
					HardwareInterface::logger),
				locoCache(params->GetControlID(), params->GetManager())
			{
			}

			virtual ~MaerklinCAN()
			{
			}

		protected:
			virtual void CacheSave(LocoCacheEntry& entry) override
			{
				locoCache.Save(entry);
			}

			virtual void CacheSave(LocoCacheEntry& entry, const std::string& oldMatchKey) override
			{
				locoCache.Save(entry, oldMatchKey);
			}

			virtual LocoID CacheDelete(const std::string& matchKey) override
			{
				return locoCache.Delete(matchKey);
			}

			virtual void CacheUpdateSlaves() override
			{
				locoCache.UpdateSlaves();
			}

		private:
			LocoCache locoCache;
	};
}} // namespace
