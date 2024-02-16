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
		string str = "objectType=MultipleUnit;";
		str += LocoBase::Serialize();
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
		LocoBase::Deserialize(arguments);
		return true;
	}

	bool MultipleUnit::GetPushpull() const
	{
		bool slavePushpull = true;
		for (auto slave : slaves)
		{
			Loco* loco = manager->GetLoco(slave->ObjectID2());
			if (loco == nullptr)
			{
				continue;
			}
			slavePushpull &= loco->GetPushpull();
		}
		return slavePushpull;
	}

	Propulsion MultipleUnit::GetPropulsion() const
	{
		uint8_t slavePropulsion = PropulsionUnknown;
		for (auto slave : slaves)
		{
			Loco* loco = manager->GetLoco(slave->ObjectID2());
			if (loco == nullptr)
			{
				continue;
			}
			const uint8_t locoPropulsion = loco->GetPropulsion();
			slavePropulsion |= locoPropulsion;
		}
		return static_cast<Propulsion>(slavePropulsion);
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
			manager->LocoBaseSpeed(ControlTypeInternal, slave->ObjectIdentifier2(), speed);
		}
	}

	void MultipleUnit::SetFunctionState(const DataModel::LocoFunctionNr nr,
		const DataModel::LocoFunctionState state)
	{
		LocoBase::SetFunctionState(nr, state);
		for (auto slave : slaves)
		{
			manager->LocoBaseFunctionState(ControlTypeInternal, slave->ObjectIdentifier2(), nr, state);
		}
	}

	void MultipleUnit::SetOrientation(const Orientation orientation)
	{
		LocoBase::SetOrientation(orientation);
		for (auto slave : slaves)
		{
			manager->LocoBaseOrientation(ControlTypeInternal, slave->ObjectIdentifier2(), orientation);
		}
	}

} // namespace DataModel
