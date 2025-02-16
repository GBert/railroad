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

#include <map>
#include <string>
#include <vector>

#include "DataModel/AccessoryBase.h"
#include "DataModel/Loco.h"
#include "DataTypes.h"
#include "Hardware/AccessoryCache.h"
#include "Hardware/Capabilities.h"
#include "Hardware/LocoCache.h"
#include "Manager.h"
#include "Utils/Utils.h"

namespace Hardware
{
	class HardwareInterface
	{
		public:
			HardwareInterface() = delete;
			HardwareInterface(const HardwareInterface&) = delete;
			HardwareInterface& operator=(const HardwareInterface&) = delete;

			// non virtual constructor is needed to prevent polymorphism
			inline HardwareInterface(Manager* manager,
				const ControlID controlID,
				const std::string& fullName,
				const std::string& shortName)
			:	manager(manager),
			 	controlID(controlID),
			 	logger(Logger::Logger::GetLogger(shortName)),
			 	fullName(fullName),
			 	shortName(shortName)
			{
			}

			virtual ~HardwareInterface()
			{
			}

			virtual void Start()
			{
			}

			// get the full name of the hardware
			inline const std::string& GetFullName() const
			{
				return fullName;
			}

			// get the short name of the hardware
			inline const std::string& GetShortName() const
			{
				return shortName;
			}

			// get hardware capabilities
			virtual Hardware::Capabilities GetCapabilities() const
			{
				return Hardware::CapabilityNone;
			}

			// get available loco protocols of this control
			virtual void GetLocoProtocols(__attribute__((unused)) std::vector<Protocol>& protocols) const
			{
			}

			// is given loco protocol supported
			virtual bool LocoProtocolSupported(__attribute__((unused)) const Protocol protocol) const
			{
				return false;
			}

			// get available accessory protocols of this control
			virtual void GetAccessoryProtocols(__attribute__((unused)) std::vector<Protocol>& protocols) const
			{
			}

			// is given accessory protocol supported
			virtual bool AccessoryProtocolSupported(__attribute__((unused)) const Protocol protocol) const
			{
				return false;
			}

			// turn booster on or off
			virtual void Booster(__attribute__((unused)) const BoosterState status)
			{
			}

			// set loco speed
			virtual void LocoSpeed(__attribute__((unused)) const Protocol protocol,
				__attribute__((unused)) const Address address,
				__attribute__((unused)) const Speed speed)
			{
			}

			// set loco orientation
			virtual void LocoOrientation(__attribute__((unused)) const Protocol protocol,
				__attribute__((unused)) const Address address,
				__attribute__((unused)) const Orientation orientation)
			{
			}

			// set loco function
			virtual void LocoFunction(__attribute__((unused)) const Protocol protocol,
				__attribute__((unused)) const Address address,
				__attribute__((unused)) const DataModel::LocoFunctionNr function,
				__attribute__((unused)) const DataModel::LocoFunctionState on)
			{
			}

			// set loco
			virtual void LocoSpeedOrientationFunctions(const Protocol protocol,
				const Address address,
				const Speed speed,
				const Orientation orientation,
				std::vector<DataModel::LocoFunctionEntry>& functions)
			{
				// sleeps are necessary to prevent command overflow in command stations (especially Märklin Gleisbox)
				LocoSpeed(protocol, address, speed);
				Utils::Utils::SleepForMilliseconds(25);
				LocoOrientation(protocol, address, orientation);
				Utils::Utils::SleepForMilliseconds(25);
				for (const DataModel::LocoFunctionEntry& functionEntry : functions)
				{
					LocoFunction(protocol, address, functionEntry.nr, functionEntry.state);
					Utils::Utils::SleepForMilliseconds(25);
				}
			}

			// accessory command
			virtual void Accessory(__attribute__((unused)) const Protocol protocol,
				__attribute__((unused)) const Address address,
				__attribute__((unused)) const DataModel::AccessoryState state,
				__attribute__((unused)) const bool on,
				__attribute__((unused)) const DataModel::AccessoryPulseDuration duration)
			{
			}

			// read CV value
			virtual void ProgramRead(__attribute__((unused)) const ProgramMode mode,
				__attribute__((unused)) const Address address,
				__attribute__((unused)) const CvNumber cv)
			{
			}

			// write CV value
			virtual void ProgramWrite(__attribute__((unused)) const ProgramMode mode,
				__attribute__((unused)) const Address address,
				__attribute__((unused)) const CvNumber cv,
				__attribute__((unused)) const CvValue value)
			{
			}

			virtual const std::map<std::string,Hardware::LocoCacheEntry>& GetLocoDatabase() const
			{
				return emptyLocoDatabase;
			}

			virtual DataModel::LocoConfig GetLocoByMatchKey(__attribute__((unused)) const std::string& matchKey) const
			{
				return DataModel::LocoConfig(LocoTypeLoco);
			}

			virtual DataModel::LocoConfig GetMultipleUnitByMatchKey(__attribute__((unused)) const std::string& matchKey) const
			{
				return DataModel::LocoConfig(LocoTypeMultipleUnit);
			}

			virtual void SetLocoIdOfMatchKey(__attribute__((unused)) const LocoID locoId,
				__attribute__((unused)) const std::string& matchKey)
			{
			}

			virtual void SetMultipleUnitIdOfMatchKey(__attribute__((unused)) const LocoID locoId,
				__attribute__((unused)) const std::string& matchKey)
			{
			}

			virtual const std::map<std::string,Hardware::AccessoryCacheEntry>& GetAccessoryDatabase() const
			{
				return emptyAccessoryDatabase;
			}

			virtual DataModel::AccessoryConfig GetAccessoryByMatchKey(__attribute__((unused)) const std::string& matchKey) const
			{
				return DataModel::AccessoryConfig();
			}

			virtual void SetAccessoryIdentifierOfMatchKey(__attribute__((unused)) const DataModel::ObjectIdentifier objectIdentifier,
				__attribute__((unused)) const std::string& matchKey)
			{
			}

			virtual const std::map<std::string,Hardware::FeedbackCacheEntry>& GetFeedbackDatabase() const
			{
				return emptyFeedbackDatabase;
			}

			virtual DataModel::FeedbackConfig GetFeedbackByMatchKey(__attribute__((unused)) const std::string& matchKey) const
			{
				return DataModel::FeedbackConfig();
			}

			virtual void SetFeedbackIdOfMatchKey(__attribute__((unused)) const FeedbackID feedbackId,
				__attribute__((unused)) const std::string& matchKey)
			{
			}

			virtual void FeedbackDelete(__attribute__((unused)) const FeedbackID feedbackID,
				__attribute__((unused)) const std::string& name)
			{
			}

			virtual void FeedbackSettings(__attribute__((unused)) const FeedbackID feedbackID,
				__attribute__((unused)) const std::string& name)
			{
			}

		protected:
			Manager* const manager;
			const ControlID controlID;
			Logger::Logger* const logger;

		private:
			const std::string fullName;
			const std::string shortName;
			std::map<std::string,Hardware::LocoCacheEntry> emptyLocoDatabase;
			std::map<std::string,Hardware::AccessoryCacheEntry> emptyAccessoryDatabase;
			std::map<std::string,Hardware::FeedbackCacheEntry> emptyFeedbackDatabase;
	};
} // namespace

