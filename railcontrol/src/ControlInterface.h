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

#include <map>
#include <string>
#include <vector>

#include "DataModel/AccessoryConfig.h"
#include "DataModel/Feedback.h"
#include "DataModel/FeedbackConfig.h"
#include "DataModel/LocoConfig.h"
#include "DataModel/LocoFunctions.h"
#include "DataTypes.h"
#include "Hardware/Capabilities.h"

namespace DataModel
{
	class Accessory;
	class Counter;
	class Loco;
	class Signal;
	class Route;
	class Switch;
	class Track;
}

namespace Hardware
{
	class HardwareParams;
}

class ControlInterface
{
	public:
		inline ControlInterface(ControlType controlType)
		:	controlType(controlType)
		{
		}

		virtual ~ControlInterface()
		{
		}

		virtual void Start()
		{
		}

		virtual void ReInit(__attribute__((unused)) const Hardware::HardwareParams* params)
		{
		}

		virtual void CheckHealth()
		{
		}

		virtual void Stop()
		{
		}

		ControlType GetControlType() const
		{
			return controlType;
		}

		virtual const std::string& GetName() const = 0;

		virtual const std::string& GetShortName() const
		{
			return GetName();
		}

		virtual void Warning(__attribute__((unused)) Languages::TextSelector textSelector)
		{
		}

		virtual void AccessoryDelete(__attribute__((unused)) const AccessoryID accessoryID,
			__attribute__((unused)) const std::string& name,
			__attribute__((unused)) const std::string& matchKey)
		{
		}

		virtual void AccessoryProtocols(__attribute__((unused)) std::vector<Protocol>& protocols) const
		{
		}

		virtual bool AccessoryProtocolSupported(__attribute__((unused)) Protocol protocol) const
		{
			return false;
		}

		virtual void AccessorySettings(__attribute__((unused)) const AccessoryID accessoryID,
			__attribute__((unused)) const std::string& name,
			__attribute__((unused)) const std::string& matchKey)
		{
		}

		virtual void AccessoryState(__attribute__((unused)) const ControlType controlType,
			__attribute__((unused)) const DataModel::Accessory* accessory)
		{
		}

		virtual void ArgumentTypes(__attribute__((unused)) std::map<unsigned char,ArgumentType>& argumentTypes) const
		{
		}

		virtual void Booster(__attribute__((unused)) const ControlType controlType,
			__attribute__((unused)) const BoosterState state)
		{
		}

		virtual Hardware::Capabilities GetCapabilities() const
		{
			return Hardware::CapabilityNone;
		}

		inline bool CanHandle(const Hardware::Capabilities capability) const
		{
			Hardware::Capabilities hardwareCapabilities = GetCapabilities();
			return capability == (hardwareCapabilities & capability);
		}

		virtual void FeedbackDelete(__attribute__((unused)) const FeedbackID feedbackID,
			__attribute__((unused)) const std::string& name)
		{
		}

		virtual void FeedbackSettings(__attribute__((unused)) const FeedbackID feedbackID,
			__attribute__((unused)) const std::string& name)
		{
		}

		virtual void FeedbackState(__attribute__((unused)) const std::string& name,
			__attribute__((unused)) const FeedbackID feedbackID,
			__attribute__((unused)) const DataModel::Feedback::FeedbackState state)
		{
		}

		virtual void LayerDelete(__attribute__((unused)) const LayerID layerID,
			__attribute__((unused)) const std::string& name)
		{
		}

		virtual void LayerSettings(__attribute__((unused)) const LayerID layerID,
			__attribute__((unused)) const std::string& name)
		{
		}

		virtual void LocoBaseDestinationReached(__attribute__((unused)) const DataModel::ObjectIdentifier& locoIdentifier,
			__attribute__((unused)) const std::string& locoName,
			__attribute__((unused)) const RouteID routeID,
			__attribute__((unused)) const std::string& routeName,
			__attribute__((unused)) const TrackID trackID,
			__attribute__((unused)) const std::string& trackName)
		{
		}

		virtual void LocoBaseSpeed(__attribute__((unused)) const ControlType controlType,
			__attribute__((unused)) const ControlID controlID,
			__attribute__((unused)) const LocoID locoID,
			__attribute__((unused)) const LocoType locoType,
			__attribute__((unused)) const Protocol protocol,
			__attribute__((unused)) const Address address,
			__attribute__((unused)) const Address serverAddress,
			__attribute__((unused)) const std::string& name,
			__attribute__((unused)) const Speed speed)
		{
		}

		virtual void LocoBaseOrientation(__attribute__((unused)) const ControlType controlType,
			__attribute__((unused)) const ControlID controlID,
			__attribute__((unused)) const LocoID locoID,
			__attribute__((unused)) const LocoType locoType,
			__attribute__((unused)) const Protocol protocol,
			__attribute__((unused)) const Address address,
			__attribute__((unused)) const Address serverAddress,
			__attribute__((unused)) const std::string& name,
			__attribute__((unused)) const Orientation orientation)
		{
		}


		virtual void LocoBaseFunctionState(__attribute__((unused)) const ControlType controlType,
			__attribute__((unused)) const ControlID controlID,
			__attribute__((unused)) const LocoID locoID,
			__attribute__((unused)) const LocoType locoType,
			__attribute__((unused)) const Protocol protocol,
			__attribute__((unused)) const Address address,
			__attribute__((unused)) const Address serverAddress,
			__attribute__((unused)) const std::string& name,
			__attribute__((unused)) const DataModel::LocoFunctionNr function,
			__attribute__((unused)) const DataModel::LocoFunctionState state)
		{
		}

		virtual void LocoBaseSpeedOrientationFunctionStates(const ControlID controlID,
			const LocoID locoID,
			const LocoType locoType,
			const Protocol protocol,
			const Address address,
			const Address serverAddress,
			const std::string& name,
			const Speed speed,
			const Orientation orientation,
			const std::vector<DataModel::LocoFunctionEntry>& functions)
		{
			LocoBaseSpeed(ControlTypeInternal, controlID, locoID, locoType, protocol, address, serverAddress, name, speed);
			LocoBaseOrientation(ControlTypeInternal, controlID, locoID, locoType, protocol, address, serverAddress, name, orientation);

			for (const DataModel::LocoFunctionEntry& function : functions)
			{
				LocoBaseFunctionState(ControlTypeInternal, controlID, locoID, locoType, protocol, address, serverAddress, name, function.nr, function.state);
			}
		}

		virtual void LocoBaseRelease(__attribute__((unused)) const DataModel::ObjectIdentifier& locoIdentifier,
			__attribute__((unused)) const std::string& locoName)
		{
		}

		virtual void LocoBaseStart(__attribute__((unused)) const DataModel::ObjectIdentifier& locoIdentifier,
			__attribute__((unused)) const std::string& locoName)
		{
		}

		virtual void LocoBaseStop(__attribute__((unused)) const DataModel::ObjectIdentifier& locoIdentifier,
				__attribute__((unused)) const std::string& locoName)
		{
		}

		virtual void LocoProtocols(__attribute__((unused)) std::vector<Protocol>& protocols) const
		{
		}

		virtual bool LocoProtocolSupported(__attribute__((unused)) Protocol protocol) const
		{
			return false;
		}

		virtual void LocoDelete(__attribute__((unused)) const LocoID locoID,
			__attribute__((unused)) const std::string& name,
			__attribute__((unused)) const std::string& matchKey)
		{
		}

		virtual void LocoSettings(__attribute__((unused)) const LocoID locoID,
			__attribute__((unused)) const std::string& name,
			__attribute__((unused)) const std::string& matchKey)
		{
		}

		virtual void MultipleUnitDelete(__attribute__((unused)) const MultipleUnitID multipleUnitID,
			__attribute__((unused)) const std::string& name,
			__attribute__((unused)) const std::string& matchKey)
		{
		}

		virtual void MultipleUnitSettings(__attribute__((unused)) const MultipleUnitID multipleUnitID,
			__attribute__((unused)) const std::string& name,
			__attribute__((unused)) const std::string& matchKey)
		{
		}

		virtual void RouteDelete(__attribute__((unused)) const RouteID routeID,
			__attribute__((unused)) const std::string& name)
		{
		}

		virtual void RouteRelease(__attribute__((unused)) const RouteID routeID)
		{
		}

		virtual void RouteSettings(__attribute__((unused)) const RouteID routeID,
			__attribute__((unused)) const std::string& name)
		{
		}

		virtual void SwitchDelete(__attribute__((unused)) const SwitchID switchID,
			__attribute__((unused)) const std::string& name,
			__attribute__((unused)) const std::string& matchKey)
		{
		}

		virtual void SwitchSettings(__attribute__((unused)) const SwitchID switchID,
			__attribute__((unused)) const std::string& name,
			__attribute__((unused)) const std::string& matchKey)
		{
		}

		virtual void SwitchState(__attribute__((unused)) const ControlType controlType,
			__attribute__((unused)) const DataModel::Switch* mySwitch)
		{
		}

		virtual void TrackDelete(__attribute__((unused)) const TrackID trackID,
			__attribute__((unused)) const std::string& name)
		{
		}

		virtual void TrackSettings(__attribute__((unused)) const TrackID trackID,
			__attribute__((unused)) const std::string& name)
		{
		}

		virtual void TrackState(__attribute__((unused)) const DataModel::Track* track)
		{
		}

		virtual void SignalDelete(__attribute__((unused)) const SignalID signalID,
			__attribute__((unused)) const std::string& name,
			__attribute__((unused)) const std::string& matchKey)
		{
		}

		virtual void SignalSettings(__attribute__((unused)) const SignalID signalID,
			__attribute__((unused)) const std::string& name,
			__attribute__((unused)) const std::string& matchKey)
		{
		}

		virtual void SignalState(__attribute__((unused)) const ControlType controlType,
			__attribute__((unused)) const DataModel::Signal* signal)
		{
		}

		virtual void ClusterDelete(__attribute__((unused)) const ClusterID clusterID,
			__attribute__((unused)) const std::string& name)
		{
		}

		virtual void ClusterSettings(__attribute__((unused)) const ClusterID clusterID,
			__attribute__((unused)) const std::string& name)
		{
		}

		virtual void TextDelete(__attribute__((unused)) const TextID textID,
			__attribute__((unused)) const std::string& name)
		{
		}

		virtual void TextSettings(__attribute__((unused)) const TextID textID,
			__attribute__((unused)) const std::string& name)
		{
		}

		virtual void CounterDelete(__attribute__((unused)) const CounterID counterID,
			__attribute__((unused)) const std::string& name)
		{
		}

		virtual void CounterSettings(__attribute__((unused)) const CounterID counterID,
			__attribute__((unused)) const std::string& name)
		{
		}

		virtual void CounterState(__attribute__((unused)) const DataModel::Counter* const counter)
		{
		}

		virtual void ProgramRead(__attribute__((unused)) const ProgramMode mode,
			__attribute__((unused)) const Address address, __attribute__((unused)) const CvNumber cv)
		{
		}

		virtual void ProgramWrite(__attribute__((unused)) const ProgramMode mode,
			__attribute__((unused)) const Address address, __attribute__((unused)) const CvNumber cv,
			__attribute__((unused)) const CvValue value)
		{
		}

		virtual void ProgramValue(__attribute__((unused)) const CvNumber cv,
			__attribute__((unused)) const CvValue value)
		{
		}

		virtual void AddUnmatchedLocos(__attribute__((unused)) std::map<std::string,DataModel::LocoConfig>& list) const
		{
		}

		virtual std::map<std::string,DataModel::LocoConfig> GetUnmatchedLocos(__attribute__((unused)) const std::string& matchKey) const
		{
			std::map<std::string,DataModel::LocoConfig> out;
			return out;
		}

		virtual DataModel::LocoConfig GetLocoByMatchKey(__attribute__((unused)) const std::string& matchKey) const
		{
			return DataModel::LocoConfig(LocoTypeNone);
		}

		virtual void AddUnmatchedMultipleUnits(__attribute__((unused)) std::map<std::string,DataModel::LocoConfig>& list) const
		{
		}

		virtual std::map<std::string,DataModel::LocoConfig> GetUnmatchedMultipleUnits(__attribute__((unused)) const std::string& matchKey) const
		{
			std::map<std::string,DataModel::LocoConfig> out;
			return out;
		}

		virtual DataModel::LocoConfig GetMultipleUnitByMatchKey(__attribute__((unused)) const std::string& matchKey) const
		{
			return DataModel::LocoConfig(LocoTypeMultipleUnit);
		}

		virtual void AddUnmatchedAccessories(__attribute__((unused)) std::map<std::string,DataModel::AccessoryConfig>& list) const
		{
		}

		virtual std::map<std::string,DataModel::AccessoryConfig> GetUnmatchedAccessories(__attribute__((unused)) const std::string& matchKey) const
		{
			std::map<std::string,DataModel::AccessoryConfig> out;
			return out;
		}

		virtual DataModel::AccessoryConfig GetAccessoryByMatchKey(__attribute__((unused)) const std::string& matchKey) const
		{
			return DataModel::AccessoryConfig();
		}

		virtual void AddUnmatchedFeedbacks(__attribute__((unused)) std::map<std::string,DataModel::FeedbackConfig>& list) const
		{
		}

		virtual std::map<std::string,DataModel::FeedbackConfig> GetUnmatchedFeedbacks(__attribute__((unused)) const std::string& matchKey) const
		{
			std::map<std::string,DataModel::FeedbackConfig> out;
			return out;
		}

		virtual DataModel::FeedbackConfig GetFeedbackByMatchKey(__attribute__((unused)) const std::string& matchKey) const
		{
			return DataModel::FeedbackConfig();
		}

	private:
		ControlType controlType;
};
