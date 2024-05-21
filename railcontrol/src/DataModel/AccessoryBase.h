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

#pragma once

#include <ctime>
#include <map>
#include <string>

#include "DataModel/HardwareHandle.h"
#include "Hardware/AccessoryCache.h"

namespace DataModel
{
	enum AccessoryType : unsigned char
	{
		AccessoryTypeDefault = 0,
		AccessoryTypeStraight = 1,
		AccessoryTypeTurn = 2,

		SignalTypeSimpleLeft  =  0,
		SignalTypeSimpleRight =  1,
		SignalTypeChDwarf     = 10,
		SignalTypeChLMain     = 11,
		SignalTypeChLDistant  = 12,
		SignalTypeChLCombined = 13,
		SignalTypeChNMain     = 14,
		SignalTypeChNDistant  = 15,
		SignalTypeDeCombined  = 20,
		SignalTypeDeHVMain    = 21,
		SignalTypeDeHVDistant = 22,
		SignalTypeDeBlock     = 23,

		SwitchTypeLeft = 0,
		SwitchTypeRight = 1,
		SwitchTypeThreeWay = 2,
		SwitchTypeMaerklinLeft = 3,
		SwitchTypeMaerklinRight = 4
	};

	enum AccessoryState : unsigned char
	{
		DefaultState = 0,

		AccessoryStateOff           = 0,
		AccessoryStateOn            = 1,

		SignalStateStop             = 0,
		SignalStateClear,
		SignalStateAspect2,
		SignalStateAspect3,
		SignalStateAspect4,
		SignalStateAspect5,
		SignalStateAspect6,
		SignalStateAspect7,
		SignalStateAspect8,
		SignalStateAspect9,
		SignalStateAspect10,
		SignalStateDark             = 0x1F,
		SignalStateStopExpected     = SignalStateStop + 0x20,
		SignalStateClearExpected,
		SignalStateAspect2Expected,
		SignalStateAspect3Expected,
		SignalStateAspect4Expected,
		SignalStateAspect5Expected,
		SignalStateAspect6Expected,
		SignalStateAspect7Expected,
		SignalStateAspect8Expected,
		SignalStateAspect9Expected,
		SignalStateAspect10Expected,
		SignalStateMax              = SignalStateAspect10Expected,

		SwitchStateTurnout          = 0,
		SwitchStateStraight         = 1,
		SwitchStateThird            = 2,

		InvalidState                = 0xFF
	};

	typedef signed char AddressOffset;

	typedef unsigned short AccessoryPulseDuration;
	static const AccessoryPulseDuration DefaultAccessoryPulseDuration = 100;

	class AccessoryBase : public HardwareHandle
	{
		public:
			AccessoryBase()
			:	HardwareHandle(),
				accessoryType(AccessoryTypeDefault),
				accessoryState(AccessoryStateOff),
				duration(0),
				inverted(false),
				lastUsed(0),
				counter(0),
			 	matchKey("")
			{
			}

			virtual ~AccessoryBase() {}

			inline AccessoryType GetAccessoryType() const
			{
				return accessoryType;
			}

			virtual inline void SetAccessoryType(AccessoryType accessoryType)
			{
				this->accessoryType = accessoryType;
			}

			inline AccessoryState GetAccessoryState() const
			{
				return accessoryState;
			}

			AccessoryState CalculateInvertedAccessoryState(const AccessoryState state) const;

			inline AccessoryState GetInvertedAccessoryState() const
			{
				return CalculateInvertedAccessoryState(accessoryState);
			}

			inline void SetAccessoryState(const AccessoryState state)
			{
				this->accessoryState = state;
				lastUsed = std::time(nullptr);
				++counter;
			}

			inline AccessoryPulseDuration GetAccessoryPulseDuration() const
			{
				return duration;
			}

			inline void SetAccessoryPulseDuration(const AccessoryPulseDuration duration)
			{
				this->duration = duration;
			}

			inline bool GetInverted() const
			{
				return inverted;
			}

			inline void SetInverted(const bool inverted)
			{
				this->inverted = inverted;
			}

			inline time_t GetLastUsed() const
			{
				return lastUsed;
			}


			inline void SetMatchKey(const std::string& matchKey)
			{
				this->matchKey = matchKey;
			}

			inline void ClearMatchKey()
			{
				matchKey.clear();
			}

			inline std::string GetMatchKey() const
			{
				return matchKey;
			}

		protected:
			virtual std::string Serialize() const;
			virtual bool Deserialize(const std::map<std::string,std::string>& arguments);

		private:
			AccessoryType accessoryType;
			AccessoryState accessoryState;
			AccessoryPulseDuration duration; // duration in ms after which the accessory command will be turned off on rails. 0 = no turn off / turn off must be made manually
			bool inverted;

			time_t lastUsed;
			unsigned int counter;

			std::string matchKey;
	};
} // namespace DataModel

