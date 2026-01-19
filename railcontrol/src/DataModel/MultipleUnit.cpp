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

#include "Manager.h"
#include "DataModel/MultipleUnit.h"

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

	void MultipleUnit::Deserialize(const std::string& serialized)
	{
		map<string,string> arguments;
		ParseArguments(serialized, arguments);
		string objectType = Utils::Utils::GetStringMapEntry(arguments, "objectType");
		if (objectType.compare("MultipleUnit") != 0)
		{
			return;
		}
		LocoBase::Deserialize(arguments);
	}

	void MultipleUnit::CalculatePropulsion()
	{
		uint8_t newPropulsion = PropulsionUnknown;
		for (auto slave : slaves)
		{
			const LocoID slaveID = slave->ObjectID2();
			const LocoConfig loco = manager->GetLoco(slaveID);
			if (loco.GetLocoID() != slaveID)
			{
				continue;
			}
			const uint8_t locoPropulsion = loco.GetPropulsion();
			newPropulsion |= locoPropulsion;
		}
		LocoBase::SetPropulsion(static_cast<Propulsion>(newPropulsion));
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
		CalculatePropulsion();
		return true;
	}

	void MultipleUnit::SetSpeed(const Speed speed)
	{
		LocoBase::SetSpeed(speed);
		for (auto slave : slaves)
		{
			manager->LocoBaseSpeed(ControlTypeInternal, slave->ObjectIdentifier2(), speed);
			Utils::Utils::SleepForMilliseconds(10);
		}
	}

	void MultipleUnit::SetFunctionState(const DataModel::LocoFunctionNr nr,
		const DataModel::LocoFunctionState state)
	{
		LocoBase::SetFunctionState(nr, state);
		for (auto slave : slaves)
		{
			manager->LocoBaseFunctionState(ControlTypeInternal, slave->ObjectIdentifier2(), nr, state);
			Utils::Utils::SleepForMilliseconds(10);
		}
	}

	void MultipleUnit::SetOrientation(const Orientation orientation)
	{
		LocoBase::SetOrientation(orientation);
		for (auto slave : slaves)
		{
			manager->LocoBaseOrientation(ControlTypeInternal, slave->ObjectIdentifier2(), OrientationChange);
			Utils::Utils::SleepForMilliseconds(10);
		}
	}

} // namespace DataModel
