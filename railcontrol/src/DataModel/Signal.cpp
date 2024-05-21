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

#include "DataModel/Signal.h"
#include "Manager.h"
#include "Utils/Utils.h"

using std::map;
using std::string;

namespace DataModel
{
	void Signal::SetAccessoryType(AccessoryType type)
	{
		AccessoryBase::SetAccessoryType(type);
		ResetStateAddressMap();
	}

	ObjectType Signal::GetObjectType() const
	{
		return ObjectTypeSignal;
	}

	std::string Signal::GetLayoutType() const
	{
		return Languages::GetText(Languages::TextSignal);
	}

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

	bool Signal::Deserialize(const std::string& serialized)
	{
		map<string,string> arguments;
		ParseArguments(serialized, arguments);
		string objectType = Utils::Utils::GetStringMapEntry(arguments, "objectType");
		if (objectType.compare("Signal") != 0)
		{
			return false;
		}

		AccessoryBase::Deserialize(arguments);
		LayoutItem::Deserialize(arguments);
		LockableItem::Deserialize(arguments);
		SetWidth(Width1);
		SetVisible(VisibleYes);
		for (int i = 0; i < SignalStateMax; ++i)
		{
			int address = Utils::Utils::GetIntegerMapEntry(arguments, "address" + std::to_string(i), -1);
			if (address == -1)
			{
				continue;
			}
			SetStateAddressOffset(static_cast<AccessoryState>(i), address);
		}
		return true;
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
		SetStateAddressOffset(SignalStateStop, 0);
		SetStateAddressOffset(SignalStateClear, 1);
		switch(GetAccessoryType())
		{
			case SignalTypeChDwarf:
				SetStateAddressOffset(SignalStateStopExpected, 2);
				break;

			case SignalTypeChLMain:
				SetStateAddressOffset(SignalStateAspect2, 2);
				SetStateAddressOffset(SignalStateAspect3, 3);
				SetStateAddressOffset(SignalStateAspect5, 4);
				SetStateAddressOffset(SignalStateAspect6, 5);
				break;

			case SignalTypeDeCombined:
				SetStateAddressOffset(SignalStateStopExpected, 2);
				SetStateAddressOffset(SignalStateAspect4, 3);
				SetStateAddressOffset(SignalStateAspect7, 4);
				SetStateAddressOffset(SignalStateDark, 5);
				break;

			case SignalTypeDeHVMain:
				SetStateAddressOffset(SignalStateAspect2, 2);
				SetStateAddressOffset(SignalStateAspect3, 3);
				break;

			case SignalTypeSimpleLeft:
			case SignalTypeSimpleRight:
			case SignalTypeDeBlock:
			default:
				break;
		}
	}

	bool Signal::UsesAddress(Address address) const
	{
		switch(GetAccessoryType())
		{
			case SignalTypeChLMain:
			case SignalTypeDeCombined:
				return (GetAddress() == address)
					|| (GetAddress() + 1 == address)
					|| (GetAddress() + 2 == address);

			case SignalTypeChDwarf:
			case SignalTypeDeHVMain:
				return (GetAddress() == address)
					|| (GetAddress() + 1 == address);

			case SignalTypeSimpleLeft:
			case SignalTypeSimpleRight:
			case SignalTypeDeBlock:
			default:
				return (GetAddress() == address);
		}
	}

	std::map<DataModel::AccessoryState,Signal::StateOption> Signal::GetStateOptions() const
	{
		std::map<DataModel::AccessoryState,Signal::StateOption> out;
		out.emplace(SignalStateStop, StateOption(Languages::TextSignalStateStop, GetStateAddressOffset(SignalStateStop)));
		out.emplace(SignalStateClear, StateOption(Languages::TextSignalStateClear, GetStateAddressOffset(SignalStateClear)));
		switch(GetAccessoryType())
		{
			case SignalTypeChDwarf:
				out.emplace(SignalStateStopExpected, StateOption(Languages::TextSignalStateCaution, GetStateAddressOffset(SignalStateStopExpected)));
				break;

			case SignalTypeChLMain:
				out.emplace(SignalStateAspect2, StateOption(Languages::TextSignalStateClear40, GetStateAddressOffset(SignalStateAspect2)));
				out.emplace(SignalStateAspect3, StateOption(Languages::TextSignalStateClear60, GetStateAddressOffset(SignalStateAspect3)));
				out.emplace(SignalStateAspect5, StateOption(Languages::TextSignalStateClear90, GetStateAddressOffset(SignalStateAspect5)));
				out.emplace(SignalStateAspect6, StateOption(Languages::TextSignalStateShortClear, GetStateAddressOffset(SignalStateAspect6)));
				break;

			case SignalTypeDeCombined:
				out.emplace(SignalStateStopExpected, StateOption(Languages::TextSignalStateStopExpected, GetStateAddressOffset(SignalStateStopExpected)));
				out.emplace(SignalStateAspect4, StateOption(Languages::TextSignalStateShunting, GetStateAddressOffset(SignalStateAspect4)));
				out.emplace(SignalStateAspect7, StateOption(Languages::TextSignalStateZs7, GetStateAddressOffset(SignalStateAspect7)));
				out.emplace(SignalStateDark, StateOption(Languages::TextSignalStateDark, GetStateAddressOffset(SignalStateDark)));
				break;

			case SignalTypeDeHVMain:
				out.emplace(SignalStateAspect2, StateOption(Languages::TextSignalStateSlow, GetStateAddressOffset(SignalStateAspect2)));
				out.emplace(SignalStateAspect3, StateOption(Languages::TextSignalStateShunting, GetStateAddressOffset(SignalStateAspect3)));
				break;

			case SignalTypeSimpleLeft:
			case SignalTypeSimpleRight:
			case SignalTypeDeBlock:
			default:
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

