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

			inline MultipleUnit(Manager* manager, const LocoID locoID)
			:	LocoBase(manager, locoID)
			{
			}

			inline MultipleUnit(Manager* manager, const std::string& serialized)
			:	LocoBase(manager, LocoNone)
			{
				Deserialize(serialized);
			}
			virtual ~MultipleUnit()
			{
			}

			inline ObjectType GetObjectType() const override
			{
				return ObjectTypeMultipleUnit;
			}

			std::string Serialize() const override;

			bool Deserialize(const std::string& serialized) override;

			virtual bool GetPushpull() const override;

			virtual Propulsion GetPropulsion() const override;

			void DeleteSlaves();

			bool AssignSlaves(const std::vector<DataModel::Relation*>& newslaves);

			inline const std::vector<DataModel::Relation*>& GetSlaves() const
			{
				return slaves;
			}

			void SetSpeed(const Speed speed) override;

			void SetFunctionState(const DataModel::LocoFunctionNr nr,
				const DataModel::LocoFunctionState state) override;

			void SetOrientation(const Orientation orientation) override;

		private:
			std::vector<DataModel::Relation*> slaves;
	};
} // namespace DataModel
