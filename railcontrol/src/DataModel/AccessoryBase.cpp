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

#include <map>
#include <sstream>

#include "DataModel/AccessoryBase.h"
#include "Utils/Utils.h"

using std::map;
using std::string;

namespace DataModel
{
	std::string AccessoryBase::Serialize() const
	{
		string str = HardwareHandle::Serialize();
		str += ";type=" + std::to_string(accessoryType);
		str += ";state=" + std::to_string(accessoryState);
		str += ";duration=" + std::to_string(duration);
		str += ";inverted=" + std::to_string(inverted);
		str += ";lastused=" + std::to_string(lastUsed);
		str += ";counter=" + std::to_string(counter);
		str += ";matchkey=" + matchKey;
		return str;
	}

	void AccessoryBase::Deserialize(const map<string,string>& arguments)
	{
		HardwareHandle::Deserialize(arguments);
		accessoryType = static_cast<AccessoryType>(Utils::Utils::GetIntegerMapEntry(arguments, "type"));
		accessoryState = static_cast<AccessoryState>(Utils::Utils::GetIntegerMapEntry(arguments, "state", AccessoryStateOff));
		duration = static_cast<AccessoryPulseDuration>(Utils::Utils::GetIntegerMapEntry(arguments, "duration", DefaultAccessoryPulseDuration));
		inverted = Utils::Utils::GetBoolMapEntry(arguments, "inverted");
		lastUsed = Utils::Utils::GetIntegerMapEntry(arguments, "lastused", 0);
		counter = Utils::Utils::GetIntegerMapEntry(arguments, "counter", 0);
		matchKey = Utils::Utils::GetStringMapEntry(arguments, "matchkey");
	}

	AccessoryState AccessoryBase::CalculateInvertedAccessoryState(const AccessoryState state) const
	{
		if (inverted == false)
		{
			return state;
		}

		switch(state)
		{
			case AccessoryStateOff:
				return AccessoryStateOn;

			case AccessoryStateOn:
				return AccessoryStateOff;

			default:
				return state;
		}
	}
} // namespace DataModel
