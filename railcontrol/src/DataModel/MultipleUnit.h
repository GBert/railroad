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

#include "DataTypes.h"
#include "DataModel/LocoBase.h"
#include "DataModel/Relation.h"

namespace DataModel
{
	class ObjectIdentifier;

	class MultipleUnit : public LocoBase
	{
		public:
			MultipleUnit() = delete;
			MultipleUnit(const MultipleUnit&) = delete;
			MultipleUnit& operator=(const MultipleUnit&) = delete;

			inline MultipleUnit(Manager* manager, const MultipleUnitID multipleUnitID)
			:	LocoBase(manager, multipleUnitID)
			{
			}

			inline MultipleUnit(Manager* manager, const std::string& serialized)
			:	LocoBase(manager, serialized)
			{
				MultipleUnit::Deserialize(serialized);
			}

			inline ~MultipleUnit()
			{
				DeleteSlaves();
			}

			inline ObjectType GetObjectType() const override
			{
				return ObjectTypeMultipleUnit;
			}

			std::string Serialize() const override;

			bool Deserialize(const std::string& serialized) override;

			void CalculatePropulsion();

			void DeleteSlaves();

			bool AssignSlaves(const std::vector<DataModel::Relation*>& newslaves);

			inline const std::vector<DataModel::Relation*>& GetSlaves() const
			{
				return slaves;
			}

			inline const std::vector<LocoID> GetSlaveIDs() const
			{
				std::vector<LocoID> out;
				for (auto const & slave : slaves)
				{
					out.push_back(slave->ObjectID2());
				}
				return out;
			}

			void SetSpeed(const Speed speed) override;

			void SetFunctionState(const DataModel::LocoFunctionNr nr,
				const DataModel::LocoFunctionState state) override;

			void SetOrientation(const Orientation orientation) override;

		private:
			std::vector<DataModel::Relation*> slaves;
	};
} // namespace DataModel
