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

#pragma once

#include <string>
#include <mutex>

#include "DataTypes.h"
#include "DataModel/LayoutItem.h"
#include "DataModel/LockableItem.h"

class Manager;

namespace DataModel
{
	enum CounterType : uint8_t
	{
		CounterTypeIncrement = 0,
		CounterTypeDecrement
	};

	class Counter : public LayoutItem
	{
		public:
			inline Counter(__attribute__((unused)) Manager* manager, const CounterID counterID)
			:	LayoutItem(counterID),
				counter(0),
				max(0),
				min(0)
			{
			}

			inline Counter(const std::string& serialized)
			:	LayoutItem(CounterNone)
			{
				Deserialize(serialized);
			}

			virtual ~Counter()
			{
			}

			inline ObjectType GetObjectType() const override
			{
				return ObjectTypeCounter;
			}

			inline std::string GetLayoutType() const override
			{
				return Languages::GetText(Languages::TextCounter);
			}

			std::string Serialize() const override;

			void Deserialize(const std::string& serialized) override;

			bool Check(const CounterType type);

			bool Count(const CounterType type);

			inline int GetCounter() const
			{
				return counter;
			}

			inline void SetMax(int max)
			{
				this->max = max;
			}

			inline int GetMax() const
			{
				return max;
			}

			inline void SetMin(int min)
			{
				this->min = min;
			}

			inline int GetMin() const
			{
				return min;
			}

		private:
			mutable std::mutex countMutex;
			int counter;
			int max;
			int min;
	};
} // namespace DataModel
