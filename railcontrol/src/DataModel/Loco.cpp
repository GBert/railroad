/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2024 by Teddy / Dominik Mahrer - www.railcontrol.org

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

#include <algorithm>
#include <map>
#include <string>

#include "DataModel/Loco.h"
#include "Hardware/LocoCache.h"
#include "Utils/Utils.h"

using std::map;
using std::string;
using std::to_string;
using std::vector;

namespace DataModel
{
	std::string Loco::Serialize() const
	{
		string str;
		str += "objectType=Loco;";
		str += LocoBase::Serialize();
		return str;
	}

	bool Loco::Deserialize(const std::string& serialized)
	{
		map<string,string> arguments;
		ParseArguments(serialized, arguments);
		if (!arguments.count("objectType") || arguments.at("objectType").compare("Loco") != 0)
		{
			return false;
		}
		LocoBase::Deserialize(arguments);
		return true;
	}

	Loco& Loco::operator=(const Hardware::LocoCacheEntry& loco)
	{
		SetControlID(loco.GetControlID());
		SetAddress(loco.GetAddress());
		SetProtocol(loco.GetProtocol());
		SetName(loco.GetName());
		SetMatchKey(loco.GetMatchKey());
		ConfigureFunctions(loco.GetFunctionStates());
		return *this;
	}
} // namespace DataModel
