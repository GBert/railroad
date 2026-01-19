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

#include "DataModel/ObjectIdentifier.h"
#include "Hardware/LocoCache.h"
#include "Manager.h"

namespace Hardware
{
	void LocoCacheEntry::UpdateSlaves(Manager* manager)
	{
		for (auto & slave : slaves)
		{
			DataModel::Loco* loco = manager->GetLocoByMatchKey(controlId, slave.GetMatchKey());
			if (!loco)
			{
				continue;
			}
			slave.SetLocoID(loco->GetID());
		}
	}

	void LocoCache::Save(LocoCacheEntry& entry, const std::string& oldMatchKey)
	{
		// FIXME: move this to Manager to always use locoMutex and use it only once
		const std::string& matchKey = entry.GetMatchKey();
		DataModel::Loco* loco = nullptr;

		const bool matchKeyChanged = (matchKey.compare(oldMatchKey) != 0);
		if (matchKeyChanged)
		{
			const LocoID locoId = Delete(oldMatchKey);
			// FIXME: remove GetLocoBaseInternal here
			loco = dynamic_cast<DataModel::Loco*>(manager->GetLocoBaseInternal(DataModel::ObjectIdentifier(ObjectTypeLoco, locoId)));
		}

		if (!loco)
		{
			loco = manager->GetLocoByMatchKey(GetControlId(), oldMatchKey);
		}

		if (!loco && matchKeyChanged)
		{
			loco = manager->GetLocoByMatchKey(GetControlId(), matchKey);
		}

		if (loco)
		{
			entry.SetLocoID(loco->GetID());
			*loco = entry;
		}
		entries.emplace(matchKey, entry);
	}

	LocoID LocoCache::Delete(const std::string& matchKey)
	{
		LocoID locoId = Get(matchKey).GetLocoID();
		manager->LocoRemoveMatchKey(locoId);
		entries.erase(matchKey);
		return locoId;
	}

	void LocoCache::SetLocoId(const LocoID locoId, const std::string& matchKey)
	{
		if (locoId != LocoNone)
		{
			for (auto& locoCacheEntry : entries)
			{
				LocoCacheEntry& entry = locoCacheEntry.second;
				if (entry.GetLocoID() == locoId)
				{
					entry.SetLocoID(LocoNone);
				}
			}
		}

		auto entry = entries.find(matchKey);
		if (entry == entries.end())
		{
			return;
		}
		entry->second.SetLocoID(locoId);
	}
} // namespace Hardware

