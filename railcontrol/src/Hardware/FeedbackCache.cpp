/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2021 Dominik (Teddy) Mahrer - www.railcontrol.org

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

#include "DataModel/Feedback.h"
#include "Hardware/FeedbackCache.h"
#include "Manager.h"

namespace Hardware
{
	void FeedbackCache::Save(FeedbackCacheEntry& entry, const std::string& oldMatchKey)
	{
		const std::string& matchKey = entry.GetMatchKey();
		DataModel::Feedback* feedback = nullptr;

		const bool matchKeyChanged = matchKey.compare(oldMatchKey) != 0;
		if (matchKeyChanged)
		{
			const FeedbackID feedbackId = Delete(oldMatchKey);
			feedback = manager->GetFeedback(feedbackId);
		}

		if (feedback == nullptr)
		{
			feedback = manager->GetFeedbackByMatchKey(GetControlId(), oldMatchKey);
		}

		if (feedback == nullptr && matchKeyChanged)
		{
			feedback = manager->GetFeedbackByMatchKey(GetControlId(), matchKey);
		}

		if (feedback != nullptr)
		{
			entry.SetFeedbackID(feedback->GetID());
			*feedback = entry;
		}
		entries.emplace(matchKey, entry);
	}

	FeedbackID FeedbackCache::Delete(const std::string& matchKey)
	{
		FeedbackID feedbackId = Get(matchKey).GetFeedbackID();
		manager->FeedbackRemoveMatchKey(feedbackId);
		entries.erase(matchKey);
		return feedbackId;
	}

	void FeedbackCache::SetFeedbackId(const FeedbackID feedbackId, const std::string& matchKey)
	{
		if (feedbackId != FeedbackNone)
		{
			for (auto& feedbackCacheEntry : entries)
			{
				FeedbackCacheEntry& entry = feedbackCacheEntry.second;
				if (entry.GetFeedbackID() == feedbackId)
				{
					entry.SetFeedbackID(FeedbackNone);
				}
			}
		}

		auto entry = entries.find(matchKey);
		if (entry == entries.end())
		{
			return;
		}
		entry->second.SetFeedbackID(feedbackId);
	}
} // namespace Hardware

