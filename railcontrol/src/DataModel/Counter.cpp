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

#include "DataModel/Counter.h"
#include "DataModel/LockableItem.h"
#include "Manager.h"
#include "Utils/Utils.h"

using std::map;
using std::string;
using std::to_string;

namespace DataModel
{
	std::string Counter::Serialize() const
	{
		string str;
		str += "objectType=Counter;";
		str += LayoutItem::Serialize();
		str += ";counter=";
		str += to_string(counter);
		str += ";max=";
		str += to_string(max);
		str += ";min=";
		str += to_string(min);
		return str;
	}

	void Counter::Deserialize(const string& serialized)
	{
		map<string,string> arguments;
		ParseArguments(serialized, arguments);
		string objectType = Utils::Utils::GetStringMapEntry(arguments, "objectType");
		if (objectType.compare("Counter") != 0)
		{
			return;
		}
		LayoutItem::Deserialize(arguments);
		counter = Utils::Utils::GetIntegerMapEntry(arguments, "counter", 0);
		max = Utils::Utils::GetIntegerMapEntry(arguments, "max", 0);
		min = Utils::Utils::GetIntegerMapEntry(arguments, "min", 0);
	}

	bool Counter::CheckCount(const CounterType type)
	{
		switch(type)
		{
			case CounterTypeIncrement:
				return (counter < max);

			case CounterTypeDecrement:
				return (counter > min);

			default:
				return false;
		}
	}

	bool Counter::Count(const CounterType type)
	{
		std::lock_guard<std::mutex> Guard(countMutex);
		switch(type)
		{
			case CounterTypeIncrement:
			{
				if (counter >= max)
				{
					return false;
				}
				++counter;
				return true;
			}

			case CounterTypeDecrement:
			{
				if (counter <= min)
				{
					return false;
				}
				--counter;
				return true;
			}

			default:
				return false;
		}
	}
} // namespace DataModel
