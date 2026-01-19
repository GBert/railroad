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

#include <string>

#include "DataModel/Loco.h"
#include "DataModel/MultipleUnit.h"
#include "DataModel/ObjectIdentifier.h"
#include "Hardware/LocoCache.h"

namespace DataModel
{
	class LocoConfig
	{
		public:
			inline LocoConfig(const LocoType type = LocoTypeNone)
			:	type(type),
				controlID(ControlNone),
				locoID(LocoNone),
				protocol(ProtocolNone),
				address(AddressNone),
				serverAddress(AddressNone),
				isInUse(false),
				pushpull(false),
				length(0),
				maxSpeed(MinSpeed),
				travelSpeed(MinSpeed),
				reducedSpeed(MinSpeed),
				creepingSpeed(MinSpeed),
				propulsion(PropulsionUnknown),
				trainType(TrainTypeUnknown),
				speed(MinSpeed),
				orientation(OrientationLeft)
			{
			}

			inline LocoConfig(const DataModel::Loco& loco)
			:	type(LocoTypeLoco),
				controlID(loco.GetControlID()),
				locoID(loco.GetID()),
				protocol(loco.GetProtocol()),
				address(loco.GetAddress()),
				serverAddress(loco.GetServerAddress()),
				name(loco.GetName()),
				matchKey(loco.GetMatchKey()),
				isInUse(loco.IsInUse()),
				pushpull(loco.GetPushpull()),
				length(loco.GetLength()),
				maxSpeed(loco.GetMaxSpeed()),
				travelSpeed(loco.GetTravelSpeed()),
				reducedSpeed(loco.GetReducedSpeed()),
				creepingSpeed(loco.GetCreepingSpeed()),
				propulsion(loco.GetPropulsion()),
				trainType(loco.GetTrainType()),
				speed(loco.GetSpeed()),
				orientation(loco.GetOrientation())
			{
				SetFunctionStates(loco.GetFunctionStates());
			}

			inline LocoConfig(const Hardware::LocoCacheEntry& loco)
			:	type(loco.GetType()),
				controlID(loco.GetControlID()),
				locoID(loco.GetLocoID()),
				protocol(loco.GetProtocol()),
				address(loco.GetAddress()),
				serverAddress(AddressNone),
				name(loco.GetName()),
				matchKey(loco.GetMatchKey()),
				isInUse(false),
				pushpull(false),
				length(0),
				maxSpeed(MinSpeed),
				travelSpeed(MinSpeed),
				reducedSpeed(MinSpeed),
				creepingSpeed(MinSpeed),
				propulsion(PropulsionUnknown),
				trainType(TrainTypeUnknown),
				slaves(loco.GetSlaveIDs()),
				speed(MinSpeed),
				orientation(OrientationLeft)
			{
				SetFunctionStates(loco.GetFunctionStates());
			}

			inline LocoConfig(const DataModel::MultipleUnit& multipleUnit)
			:	type(LocoTypeMultipleUnit),
				controlID(multipleUnit.GetControlID()),
				locoID(multipleUnit.GetID()),
				protocol(multipleUnit.GetProtocol()),
				address(multipleUnit.GetAddress()),
				serverAddress(multipleUnit.GetServerAddress()),
				name(multipleUnit.GetName()),
				matchKey(multipleUnit.GetMatchKey()),
				isInUse(multipleUnit.IsInUse()),
				pushpull(multipleUnit.GetPushpull()),
				length(multipleUnit.GetLength()),
				maxSpeed(multipleUnit.GetMaxSpeed()),
				travelSpeed(multipleUnit.GetTravelSpeed()),
				reducedSpeed(multipleUnit.GetReducedSpeed()),
				creepingSpeed(multipleUnit.GetCreepingSpeed()),
				propulsion(multipleUnit.GetPropulsion()),
				trainType(multipleUnit.GetTrainType()),
				slaves(multipleUnit.GetSlaveIDs()),
				speed(multipleUnit.GetSpeed()),
				orientation(multipleUnit.GetOrientation())
			{
				SetFunctionStates(multipleUnit.GetFunctionStates());
			}

			inline LocoConfig(const DataModel::LocoBase& locoBase)
			:	type(locoBase.GetLocoType()),
				controlID(locoBase.GetControlID()),
				locoID(locoBase.GetID()),
				protocol(locoBase.GetProtocol()),
				address(locoBase.GetAddress()),
				serverAddress(locoBase.GetServerAddress()),
				name(locoBase.GetName()),
				matchKey(locoBase.GetMatchKey()),
				isInUse(locoBase.IsInUse()),
				pushpull(locoBase.GetPushpull()),
				length(locoBase.GetLength()),
				maxSpeed(locoBase.GetMaxSpeed()),
				travelSpeed(locoBase.GetTravelSpeed()),
				reducedSpeed(locoBase.GetReducedSpeed()),
				creepingSpeed(locoBase.GetCreepingSpeed()),
				propulsion(locoBase.GetPropulsion()),
				trainType(locoBase.GetTrainType()),
				speed(locoBase.GetSpeed()),
				orientation(locoBase.GetOrientation())
			{
				SetFunctionStates(locoBase.GetFunctionStates());
			}

			inline LocoConfig& operator=(const DataModel::Loco& loco)
			{
				type = LocoTypeLoco;
				controlID = loco.GetControlID();
				locoID = loco.GetID();
				protocol = loco.GetProtocol();
				address = loco.GetAddress();
				serverAddress = loco.GetServerAddress();
				name = loco.GetName();
				matchKey = loco.GetMatchKey();
				isInUse = loco.IsInUse();
				SetFunctionStates(loco.GetFunctionStates());
				pushpull = loco.GetPushpull();
				length = loco.GetLength();
				maxSpeed = loco.GetMaxSpeed();
				travelSpeed = loco.GetTravelSpeed();
				reducedSpeed = loco.GetReducedSpeed();
				creepingSpeed = loco.GetCreepingSpeed();
				propulsion = loco.GetPropulsion();
				trainType = loco.GetTrainType();
				slaves.clear();
				speed = loco.GetSpeed();
				orientation = loco.GetOrientation();
				return *this;
			}

			inline LocoConfig& operator=(const Hardware::LocoCacheEntry& locoCache)
			{
				type = locoCache.GetType();
				controlID = locoCache.GetControlID();
				locoID = locoCache.GetLocoID();
				protocol = locoCache.GetProtocol();
				address = locoCache.GetAddress();
				serverAddress = AddressNone;
				name = locoCache.GetName();
				matchKey = locoCache.GetMatchKey();
				isInUse = false;
				SetFunctionStates(locoCache.GetFunctionStates());
				pushpull = false;
				length = 0;
				maxSpeed = MinSpeed;
				travelSpeed = MinSpeed;
				reducedSpeed = MinSpeed;
				creepingSpeed = MinSpeed;
				propulsion = PropulsionUnknown;
				trainType = TrainTypeUnknown;
				slaves = locoCache.GetSlaveIDs();
				speed = MinSpeed;
				orientation = OrientationLeft;
				return *this;
			}

			inline LocoConfig& operator=(const DataModel::MultipleUnit& multipleUnit)
			{
				type = LocoTypeMultipleUnit;
				controlID = multipleUnit.GetControlID();
				locoID = multipleUnit.GetID();
				protocol = ProtocolNone;
				address = multipleUnit.GetAddress();
				serverAddress = multipleUnit.GetServerAddress();
				name = multipleUnit.GetName();
				matchKey = multipleUnit.GetMatchKey();
				isInUse = multipleUnit.IsInUse();
				SetFunctionStates(multipleUnit.GetFunctionStates());
				pushpull = multipleUnit.GetPushpull();
				length = multipleUnit.GetLength();
				maxSpeed = multipleUnit.GetMaxSpeed();
				travelSpeed = multipleUnit.GetTravelSpeed();
				reducedSpeed = multipleUnit.GetReducedSpeed();
				creepingSpeed = multipleUnit.GetCreepingSpeed();
				propulsion = multipleUnit.GetPropulsion();
				trainType = multipleUnit.GetTrainType();
				slaves = multipleUnit.GetSlaveIDs();
				speed = multipleUnit.GetSpeed();
				orientation = multipleUnit.GetOrientation();
				return *this;
			}

			inline LocoConfig& operator=(const DataModel::LocoBase& locoBase)
			{
				type = locoBase.GetLocoType();
				controlID = locoBase.GetControlID();
				locoID = locoBase.GetID();
				protocol = locoBase.GetProtocol();
				address = locoBase.GetAddress();
				serverAddress = locoBase.GetServerAddress();
				name = locoBase.GetName();
				matchKey = locoBase.GetMatchKey();
				isInUse = locoBase.IsInUse();
				SetFunctionStates(locoBase.GetFunctionStates());
				pushpull = locoBase.GetPushpull();
				length = locoBase.GetLength();
				maxSpeed = locoBase.GetMaxSpeed();
				travelSpeed = locoBase.GetTravelSpeed();
				reducedSpeed = locoBase.GetReducedSpeed();
				creepingSpeed = locoBase.GetCreepingSpeed();
				propulsion = locoBase.GetPropulsion();
				trainType = locoBase.GetTrainType();
				slaves.clear();
				speed = locoBase.GetSpeed();
				orientation = locoBase.GetOrientation();
				return *this;
			}

			inline LocoType GetType() const
			{
				return type;
			}

			inline ControlID GetControlID() const
			{
				return controlID;
			}

			inline LocoID GetLocoID() const
			{
				return locoID;
			}

			inline DataModel::ObjectIdentifier GetObjectIdentifier() const
			{
				switch (type)
				{
					case LocoTypeLoco:
						return DataModel::ObjectIdentifier(ObjectTypeLoco, locoID);

					case LocoTypeMultipleUnit:
						return DataModel::ObjectIdentifier(ObjectTypeMultipleUnit, locoID);

					default:
						return DataModel::ObjectIdentifier(ObjectTypeNone, LocoNone);
				}
			}

			inline Protocol GetProtocol() const
			{
				return protocol;
			}

			inline Address GetAddress() const
			{
				return address;
			}

			inline Address GetServerAddress() const
			{
				return serverAddress;
			}

			inline const std::string& GetName() const
			{
				return name;
			}

			inline const std::string& GetMatchKey() const
			{
				return matchKey;
			}

			inline bool IsInUse() const
			{
				return isInUse;
			}

			inline std::vector<DataModel::LocoFunctionEntry> GetFunctionStates() const
			{
				return functions.GetFunctionStates();
			}

			inline LocoFunctionState GetFunctionState(const LocoFunctionNr nr) const
			{
				return functions.GetFunctionState(nr);
			}

			inline void GetFunctionStates(LocoFunctionEntry* out) const
			{
				functions.GetFunctions(out);
			}

			inline void SetFunctionStates(const std::vector<LocoFunctionEntry>& newEntries)
			{
				functions.SetFunctionStates(newEntries);
			}

			inline bool GetPushpull() const
			{
				return pushpull;
			}

			inline Length GetLength() const
			{
				return length;
			}

			inline Speed GetMaxSpeed() const
			{
				return maxSpeed;
			}

			inline Speed GetTravelSpeed() const
			{
				return travelSpeed;
			}

			inline Speed GetReducedSpeed() const
			{
				return reducedSpeed;
			}

			inline Speed GetCreepingSpeed() const
			{
				return creepingSpeed;
			}

			inline Propulsion GetPropulsion() const
			{
				return propulsion;
			}

			inline TrainType GetTrainType() const
			{
				return trainType;
			}

			inline void AddSlave(const LocoID locoID)
			{
				slaves.push_back(locoID);
			}

			inline const std::vector<LocoID>& GetSlaves() const
			{
				return slaves;
			}

			inline Speed GetSpeed() const
			{
				return speed;
			}

			inline Orientation GetOrientation() const
			{
				return orientation;
			}

		private:
			LocoType type;
			ControlID controlID;
			LocoID locoID;
			Protocol protocol;
			Address address;
			Address serverAddress;
			std::string name;
			std::string matchKey;
			bool isInUse;
			LocoFunctions functions;
			bool pushpull;
			Length length;
			Speed maxSpeed;
			Speed travelSpeed;
			Speed reducedSpeed;
			Speed creepingSpeed;
			Propulsion propulsion;
			TrainType trainType;
			std::vector<LocoID> slaves;

			Speed speed;
			Orientation orientation;
};
} // namespace DataModel
