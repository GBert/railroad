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

#include <string>

#include "DataModel/Loco.h"
#include "DataModel/MultipleUnit.h"
#include "Hardware/LocoCache.h"

namespace DataModel
{
	class LocoConfig
	{
		public:
			inline LocoConfig(const LocoType type = LocoTypeLoco)
			:	controlId(ControlNone),
				locoId(LocoNone),
				address(AddressDefault),
				protocol(ProtocolNone),
				type(type),
				isInUse(false)
			{
			}

			inline LocoConfig(const DataModel::Loco& loco)
			:	controlId(loco.GetControlID()),
				locoId(loco.GetID()),
				address(loco.GetAddress()),
				protocol(loco.GetProtocol()),
				type(LocoTypeLoco),
				name(loco.GetName()),
				matchKey(loco.GetMatchKey()),
				isInUse(loco.IsInUse())
			{
				ConfigureFunctions(loco.GetFunctionStates());
			}

			inline LocoConfig(const Hardware::LocoCacheEntry& loco)
			:	controlId(loco.GetControlID()),
				locoId(loco.GetLocoID()),
				address(loco.GetAddress()),
				protocol(loco.GetProtocol()),
				type(loco.GetType()),
				name(loco.GetName()),
				matchKey(loco.GetMatchKey()),
				isInUse(false),
				slaves(loco.GetSlaveIDs())
			{
				ConfigureFunctions(loco.GetFunctionStates());
			}

			inline LocoConfig& operator=(const DataModel::Loco& loco)
			{
				controlId = loco.GetControlID();
				locoId = loco.GetID();
				address = loco.GetAddress();
				protocol = loco.GetProtocol();
				type = LocoTypeLoco;
				name = loco.GetName();
				matchKey = loco.GetMatchKey();
				isInUse = loco.IsInUse();
				ConfigureFunctions(loco.GetFunctionStates());
				return *this;
			}

			inline LocoConfig& operator=(const Hardware::LocoCacheEntry& loco)
			{
				controlId = loco.GetControlID();
				locoId = loco.GetLocoID();
				address = loco.GetAddress();
				protocol = loco.GetProtocol();
				type = loco.GetType();
				name = loco.GetName();
				matchKey = loco.GetMatchKey();
				ConfigureFunctions(loco.GetFunctionStates());
				slaves = loco.GetSlaveIDs();
				return *this;
			}

			inline LocoConfig& operator=(const DataModel::MultipleUnit& multipleUnit)
			{
				controlId = multipleUnit.GetControlID();
				locoId = multipleUnit.GetID();
				address = multipleUnit.GetAddress();
				protocol = ProtocolNone;
				type = LocoTypeMultipleUnit;
				name = multipleUnit.GetName();
				matchKey = multipleUnit.GetMatchKey();
				isInUse = multipleUnit.IsInUse();
				ConfigureFunctions(multipleUnit.GetFunctionStates());
				slaves = multipleUnit.GetSlaveIDs();
				return *this;
			}

			inline ControlID GetControlId() const
			{
				return controlId;
			}

			inline LocoID GetLocoId() const
			{
				return locoId;
			}

			inline Address GetAddress() const
			{
				return address;
			}

			inline Protocol GetProtocol() const
			{
				return protocol;
			}

			inline LocoType GetType() const
			{
				return type;
			}

			inline void SetType(const LocoType type)
			{
				this->type = type;
			}

			inline std::string GetName() const
			{
				return name;
			}

			inline void SetName(const std::string& name)
			{
				this->name = name;
			}

			inline std::string GetMatchKey() const
			{
				return matchKey;
			}

			inline bool IsInUse() const
			{
				return isInUse;
			}

			inline void GetFunctions(LocoFunctionEntry* out) const
			{
				functions.GetFunctions(out);
			}

			inline void ConfigureFunctions(const std::vector<LocoFunctionEntry>& newEntries)
			{
				functions.ConfigureFunctions(newEntries);
			}

			inline void AddSlave(const LocoID locoID)
			{
				slaves.push_back(locoID);
			}

			inline const std::vector<LocoID>& GetSlaves() const
			{
				return slaves;
			}

		private:
			ControlID controlId;
			LocoID locoId;
			Address address;
			Protocol protocol;
			LocoType type;
			std::string name;
			std::string matchKey;
			bool isInUse;
			LocoFunctions functions;
			std::vector<LocoID> slaves;
	};
} // namespace DataModel
