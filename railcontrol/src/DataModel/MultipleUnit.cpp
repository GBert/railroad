/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2023 Dominik (Teddy) Mahrer - www.railcontrol.org

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

#include "Manager.h"
#include "MultipleUnit.h"

#include <map>
#include <string>

using std::map;
using std::string;

namespace DataModel
{
	std::string MultipleUnit::Serialize() const
	{
		string str;
		str += "objectType=MultipleUnit;";
		str += LocoBase::Serialize();
		// FIXME: serialize slaves
		return str;
	}

	bool MultipleUnit::Deserialize(const std::string& serialized)
	{
		map<string,string> arguments;
		ParseArguments(serialized, arguments);
		if (!arguments.count("objectType") || arguments.at("objectType").compare("MultipleUnit") != 0)
		{
			return false;
		}
		// FIXME: deserialize slaves
		LocoBase::Deserialize(arguments);
		return true;
	}

	void MultipleUnit::DeleteSlaves()
	{
		while (slaves.size() > 0)
		{
			Relation* slave = slaves.back();
			slaves.pop_back();
			delete slave;
		}
	}

	bool MultipleUnit::AssignSlaves(const std::vector<DataModel::Relation*>& newslaves)
	{
		DeleteSlaves();
		slaves = newslaves;
		return true;
	}

	void MultipleUnit::SetSpeed(const Speed speed)
	{
		LocoBase::SetSpeed(speed);
		for (auto slave : slaves)
		{
			manager->LocoBaseSpeed(ControlTypeInternal, slave->ObjectID2(), speed);
		}
	}

	void MultipleUnit::SetFunctionState(const DataModel::LocoFunctionNr nr,
		const DataModel::LocoFunctionState state)
	{
		LocoBase::SetFunctionState(nr, state);
		for (auto slave : slaves)
		{
			manager->LocoBaseFunctionState(ControlTypeInternal, slave->ObjectID2(), nr, state);
		}
	}

	void MultipleUnit::SetOrientation(const Orientation orientation)
	{
		LocoBase::SetOrientation(orientation);
		for (auto slave : slaves)
		{
			manager->LocoBaseOrientation(ControlTypeInternal, slave->ObjectID2(), orientation);
		}
	}

} // namespace DataModel
