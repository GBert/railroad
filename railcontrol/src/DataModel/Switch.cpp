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

#include <map>

#include "DataModel/Switch.h"
#include "Utils/Utils.h"

using std::map;
using std::stringstream;
using std::string;

namespace DataModel
{
	std::string Switch::Serialize() const
	{
		string str = "objectType=Switch;";
		str += AccessoryBase::Serialize();
		str += ";";
		str += LayoutItem::Serialize();
		str += ";";
		str += LockableItem::Serialize();
		return str;
	}

	bool Switch::Deserialize(const std::string& serialized)
	{
		map<string,string> arguments;
		ParseArguments(serialized, arguments);
		string objectType = Utils::Utils::GetStringMapEntry(arguments, "objectType");
		if (objectType.compare("Switch") != 0)
		{
			return false;
		}

		AccessoryBase::Deserialize(arguments);
		LayoutItem::Deserialize(arguments);
		LockableItem::Deserialize(arguments);
		SetSizeFromType();
		SetVisible(VisibleYes);
		return true;
	}

	void Switch::SetAccessoryState(const AccessoryState state)
	{
		AccessoryState checkedState = state;
		if (GetAccessoryType() != SwitchTypeThreeWay && state == SwitchStateThird)
		{
			checkedState = SwitchStateTurnout;
		}
		AccessoryBase::SetAccessoryState(checkedState);
	}

	bool Switch::UsesAddress(Address address) const
	{
		switch(GetAccessoryType())
		{
			case SwitchTypeThreeWay:
				return (GetAddress() == address)
					|| (GetAddress() + 1 == address);

			case SwitchTypeLeft:
			case SwitchTypeRight:
			case SwitchTypeMaerklinLeft:
			case SwitchTypeMaerklinRight:
			default:
				return (GetAddress() == address);
		}
	}

	std::map<DataModel::AccessoryState, Languages::TextSelector> Switch::GetStateOptions() const
	{
		std::map<DataModel::AccessoryState,Languages::TextSelector> out;
		out[DataModel::SwitchStateStraight] = Languages::TextStraight;
		switch(GetAccessoryType())
		{
			case DataModel::SwitchTypeThreeWay:
				out[DataModel::SwitchStateTurnout] = Languages::TextLeft;
				out[DataModel::SwitchStateThird] = Languages::TextRight;
				break;

			default:
				out[DataModel::SwitchStateTurnout] = Languages::TextTurnout;
				break;
		}
		return out;
	}

	AccessoryState Switch::CalculateInvertedSwitchState(const Address address, const AccessoryState state) const
	{
		if (!GetInverted())
		{
			if ((GetAddress() + 1) == address && state == SwitchStateTurnout)
			{
				return SwitchStateThird;
			}
			return state;
		}

		switch(state)
		{
			case SwitchStateTurnout:
			case SwitchStateThird:
				return SwitchStateStraight;

			case SwitchStateStraight:
				return GetAddress() == address ? SwitchStateTurnout : SwitchStateThird;

			default:
				return state;
		}
	}

	DataModel::LayoutItem::LayoutItemSize Switch::CalculateHeightFromType(AccessoryType type)
	{
		switch (type)
		{
			case SwitchTypeMaerklinLeft:
			case SwitchTypeMaerklinRight:
				return 2;

			default:
				return Height1;
		}
	}

	Switch& Switch::operator=(const Hardware::AccessoryCacheEntry& accessory)
	{
		SetControlID(accessory.GetControlID());
		SetName(accessory.GetName());
		SetAddress(accessory.GetAddress());
		SetProtocol(accessory.GetProtocol());
		SetMatchKey(accessory.GetMatchKey());
		return *this;
	}
} // namespace DataModel
