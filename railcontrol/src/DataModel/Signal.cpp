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

#include <map>

#include "DataModel/Signal.h"
#include "Manager.h"
#include "Utils/Utils.h"

using std::map;
using std::string;

namespace DataModel
{
	std::string Signal::Serialize() const
	{
		string str = "objectType=Signal;";
		str += AccessoryBase::Serialize();
		str += ";";
		str += LayoutItem::Serialize();
		str += ";";
		str += LockableItem::Serialize();
		for (auto& stateAddress : stateAddressMap)
		{
			str += ";address";
			str += std::to_string(stateAddress.first);
			str += "=";
			str += std::to_string(stateAddress.second);
		}
		return str;
	}

	void Signal::Deserialize(const std::string& serialized)
	{
		map<string,string> arguments;
		ParseArguments(serialized, arguments);
		string objectType = Utils::Utils::GetStringMapEntry(arguments, "objectType");
		if (objectType.compare("Signal") != 0)
		{
			return;
		}

		AccessoryBase::Deserialize(arguments);
		LayoutItem::Deserialize(arguments);
		LockableItem::Deserialize(arguments);
		SetWidth(Width1);
		SetVisible(VisibleYes);
		for (int i = 0; i <= SignalStateMax; ++i)
		{
			int address = Utils::Utils::GetIntegerMapEntry(arguments, "address" + std::to_string(i), -1);
			if (address == -1)
			{
				continue;
			}
			SetStateAddressOffset(static_cast<AccessoryState>(i), address);
		}
	}

	Signal& Signal::operator=(const Hardware::AccessoryCacheEntry& accessory)
	{
		SetControlID(accessory.GetControlID());
		SetName(accessory.GetName());
		SetAddress(accessory.GetAddress());
		SetProtocol(accessory.GetProtocol());
		SetMatchKey(accessory.GetMatchKey());
		return *this;
	}

	void Signal::ResetStateAddressMap()
	{
		stateAddressMap.clear();
		switch(GetAccessoryType())
		{
			case SignalTypeChDwarf:
				SetStateAddressOffset(SignalStateStop, 0);
				SetStateAddressOffset(SignalStateClear, 1);
				SetStateAddressOffset(SignalStateStopExpected, 2);
				break;

			case SignalTypeChLMain:
				SetStateAddressOffset(SignalStateStop, 0);
				SetStateAddressOffset(SignalStateClear, 1);
				SetStateAddressOffset(SignalStateAspect2, 2);
				SetStateAddressOffset(SignalStateAspect3, 3);
				SetStateAddressOffset(SignalStateAspect5, 4);
				SetStateAddressOffset(SignalStateAspect6, 5);
				break;

			case SignalTypeChLDistant:
				SetStateAddressOffset(SignalStateStopExpected, 0);
				SetStateAddressOffset(SignalStateClearExpected, 1);
				SetStateAddressOffset(SignalStateAspect2Expected, 2);
				SetStateAddressOffset(SignalStateAspect3Expected, 3);
				SetStateAddressOffset(SignalStateAspect5Expected, 4);
				break;

			case SignalTypeChNMain:
				SetStateAddressOffset(SignalStateStop, 0);
				SetStateAddressOffset(SignalStateClear, 1);
				SetStateAddressOffset(SignalStateAspect2, 2);
				SetStateAddressOffset(SignalStateAspect3, 3);
				SetStateAddressOffset(SignalStateAspect4, 4);
				SetStateAddressOffset(SignalStateAspect5, 5);
				SetStateAddressOffset(SignalStateAspect6, 6);
				SetStateAddressOffset(SignalStateAspect7, 7);
				SetStateAddressOffset(SignalStateAspect8, 8);
				SetStateAddressOffset(SignalStateAspect9, 9);
				SetStateAddressOffset(SignalStateAspect10, 10);
				SetStateAddressOffset(SignalStateStopExpected, 11);
				SetStateAddressOffset(SignalStateAspect2Expected, 12);
				SetStateAddressOffset(SignalStateAspect3Expected, 13);
				SetStateAddressOffset(SignalStateAspect4Expected, 14);
				SetStateAddressOffset(SignalStateAspect5Expected, 15);
				SetStateAddressOffset(SignalStateAspect6Expected, 16);
				SetStateAddressOffset(SignalStateAspect7Expected, 17);
				SetStateAddressOffset(SignalStateAspect8Expected, 18);
				SetStateAddressOffset(SignalStateAspect9Expected, 19);
				SetStateAddressOffset(SignalStateAspect10Expected, 20);
				break;

			case SignalTypeDeCombined:
				SetStateAddressOffset(SignalStateStop, 0);
				SetStateAddressOffset(SignalStateClear, 1);
				SetStateAddressOffset(SignalStateStopExpected, 2);
				SetStateAddressOffset(SignalStateAspect4, 3);
				SetStateAddressOffset(SignalStateAspect7, 4);
				SetStateAddressOffset(SignalStateDark, 5);
				break;

			case SignalTypeDeHVMain:
				SetStateAddressOffset(SignalStateStop, 0);
				SetStateAddressOffset(SignalStateClear, 1);
				SetStateAddressOffset(SignalStateAspect2, 2);
				SetStateAddressOffset(SignalStateAspect3, 3);
				break;

			case SignalTypeSimpleLeft:
			case SignalTypeSimpleRight:
			case SignalTypeDeBlock:
			default:
				SetStateAddressOffset(SignalStateStop, 0);
				SetStateAddressOffset(SignalStateClear, 1);
				break;
		}
	}

	bool Signal::UsesAddress(Address address) const
	{
		const Address baseAddress = GetAddress();
		for (auto offset : stateAddressMap)
		{
			const Address addressOffset = offset.second >> 1;
			if ((baseAddress + addressOffset) == address)
			{
				return true;
			}
		}
		return false;
	}

	std::map<DataModel::AccessoryState,Signal::StateOption> Signal::GetStateOptions() const
	{
		std::map<DataModel::AccessoryState,Signal::StateOption> out;
		switch(GetAccessoryType())
		{
			case SignalTypeChDwarf:
				out.emplace(SignalStateStop, StateOption(Languages::TextSignalStateStop, GetStateAddressOffset(SignalStateStop)));
				out.emplace(SignalStateClear, StateOption(Languages::TextSignalStateClear, GetStateAddressOffset(SignalStateClear)));
				out.emplace(SignalStateStopExpected, StateOption(Languages::TextSignalStateCaution, GetStateAddressOffset(SignalStateStopExpected)));
				break;

			case SignalTypeChLMain:
				out.emplace(SignalStateStop, StateOption(Languages::TextSignalStateStop, GetStateAddressOffset(SignalStateStop)));
				out.emplace(SignalStateClear, StateOption(Languages::TextSignalStateClear, GetStateAddressOffset(SignalStateClear)));
				out.emplace(SignalStateAspect2, StateOption(Languages::TextSignalStateClear40, GetStateAddressOffset(SignalStateAspect2)));
				out.emplace(SignalStateAspect3, StateOption(Languages::TextSignalStateClear60, GetStateAddressOffset(SignalStateAspect3)));
				out.emplace(SignalStateAspect5, StateOption(Languages::TextSignalStateClear90, GetStateAddressOffset(SignalStateAspect5)));
				out.emplace(SignalStateAspect6, StateOption(Languages::TextSignalStateShortClear, GetStateAddressOffset(SignalStateAspect6)));
				break;

			case SignalTypeChLDistant:
				out.emplace(SignalStateStopExpected, StateOption(Languages::TextSignalStateStopExpected, GetStateAddressOffset(SignalStateStopExpected)));
				out.emplace(SignalStateClearExpected, StateOption(Languages::TextSignalStateClearExpected, GetStateAddressOffset(SignalStateClearExpected)));
				out.emplace(SignalStateAspect2Expected, StateOption(Languages::TextSignalStateClear40Expected, GetStateAddressOffset(SignalStateAspect2Expected)));
				out.emplace(SignalStateAspect3Expected, StateOption(Languages::TextSignalStateClear60Expected, GetStateAddressOffset(SignalStateAspect3Expected)));
				out.emplace(SignalStateAspect5Expected, StateOption(Languages::TextSignalStateClear90Expected, GetStateAddressOffset(SignalStateAspect5Expected)));
				break;

			case SignalTypeChNMain:
				out.emplace(SignalStateStop, StateOption(Languages::TextSignalStateStop, GetStateAddressOffset(SignalStateStop)));
				out.emplace(SignalStateClear, StateOption(Languages::TextSignalStateClear, GetStateAddressOffset(SignalStateClear)));
				out.emplace(SignalStateAspect2, StateOption(Languages::TextSignalStateClear40, GetStateAddressOffset(SignalStateAspect2)));
				out.emplace(SignalStateAspect3, StateOption(Languages::TextSignalStateClear50, GetStateAddressOffset(SignalStateAspect3)));
				out.emplace(SignalStateAspect4, StateOption(Languages::TextSignalStateClear60, GetStateAddressOffset(SignalStateAspect4)));
				out.emplace(SignalStateAspect5, StateOption(Languages::TextSignalStateClear70, GetStateAddressOffset(SignalStateAspect5)));
				out.emplace(SignalStateAspect6, StateOption(Languages::TextSignalStateClear80, GetStateAddressOffset(SignalStateAspect6)));
				out.emplace(SignalStateAspect7, StateOption(Languages::TextSignalStateClear90, GetStateAddressOffset(SignalStateAspect7)));
				out.emplace(SignalStateAspect8, StateOption(Languages::TextSignalStateClear100, GetStateAddressOffset(SignalStateAspect8)));
				out.emplace(SignalStateAspect9, StateOption(Languages::TextSignalStateClear110, GetStateAddressOffset(SignalStateAspect9)));
				out.emplace(SignalStateAspect10, StateOption(Languages::TextSignalStateClear120, GetStateAddressOffset(SignalStateAspect10)));
				out.emplace(SignalStateStopExpected, StateOption(Languages::TextSignalStateStopExpected, GetStateAddressOffset(SignalStateStopExpected)));
				out.emplace(SignalStateAspect2Expected, StateOption(Languages::TextSignalStateClear40Expected, GetStateAddressOffset(SignalStateAspect2Expected)));
				out.emplace(SignalStateAspect3Expected, StateOption(Languages::TextSignalStateClear50Expected, GetStateAddressOffset(SignalStateAspect3Expected)));
				out.emplace(SignalStateAspect4Expected, StateOption(Languages::TextSignalStateClear60Expected, GetStateAddressOffset(SignalStateAspect4Expected)));
				out.emplace(SignalStateAspect5Expected, StateOption(Languages::TextSignalStateClear70Expected, GetStateAddressOffset(SignalStateAspect5Expected)));
				out.emplace(SignalStateAspect6Expected, StateOption(Languages::TextSignalStateClear80Expected, GetStateAddressOffset(SignalStateAspect6Expected)));
				out.emplace(SignalStateAspect7Expected, StateOption(Languages::TextSignalStateClear90Expected, GetStateAddressOffset(SignalStateAspect7Expected)));
				out.emplace(SignalStateAspect8Expected, StateOption(Languages::TextSignalStateClear100Expected, GetStateAddressOffset(SignalStateAspect8Expected)));
				out.emplace(SignalStateAspect9Expected, StateOption(Languages::TextSignalStateClear110Expected, GetStateAddressOffset(SignalStateAspect9Expected)));
				out.emplace(SignalStateAspect10Expected, StateOption(Languages::TextSignalStateClear120Expected, GetStateAddressOffset(SignalStateAspect10Expected)));
				break;

			case SignalTypeDeCombined:
				out.emplace(SignalStateStop, StateOption(Languages::TextSignalStateStop, GetStateAddressOffset(SignalStateStop)));
				out.emplace(SignalStateClear, StateOption(Languages::TextSignalStateClear, GetStateAddressOffset(SignalStateClear)));
				out.emplace(SignalStateStopExpected, StateOption(Languages::TextSignalStateStopExpected, GetStateAddressOffset(SignalStateStopExpected)));
				out.emplace(SignalStateAspect4, StateOption(Languages::TextSignalStateShunting, GetStateAddressOffset(SignalStateAspect4)));
				out.emplace(SignalStateAspect7, StateOption(Languages::TextSignalStateZs7, GetStateAddressOffset(SignalStateAspect7)));
				out.emplace(SignalStateDark, StateOption(Languages::TextSignalStateDark, GetStateAddressOffset(SignalStateDark)));
				break;

			case SignalTypeDeHVMain:
				out.emplace(SignalStateStop, StateOption(Languages::TextSignalStateStop, GetStateAddressOffset(SignalStateStop)));
				out.emplace(SignalStateClear, StateOption(Languages::TextSignalStateClear, GetStateAddressOffset(SignalStateClear)));
				out.emplace(SignalStateAspect2, StateOption(Languages::TextSignalStateSlow, GetStateAddressOffset(SignalStateAspect2)));
				out.emplace(SignalStateAspect3, StateOption(Languages::TextSignalStateShunting, GetStateAddressOffset(SignalStateAspect3)));
				break;

			case SignalTypeSimpleLeft:
			case SignalTypeSimpleRight:
			case SignalTypeDeBlock:
			default:
				out.emplace(SignalStateStop, StateOption(Languages::TextSignalStateStop, GetStateAddressOffset(SignalStateStop)));
				out.emplace(SignalStateClear, StateOption(Languages::TextSignalStateClear, GetStateAddressOffset(SignalStateClear)));
				break;
		}
		return out;
	}

	AccessoryState Signal::CalculateMappedSignalState(const Address address, const AccessoryState state) const
	{
		const Address baseAddress = GetAddress();
		const AddressOffset addressOffset = ((address - baseAddress) << 1) + state;
		for (auto& state : stateAddressMap)
		{
			if (state.second == addressOffset)
			{
				return state.first;
			}
		}
		return InvalidState;
	}
} // namespace DataModel

