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

#include <cstring>
#include <string>
#include <map>

#include "DataTypes.h"

class Manager;

namespace Hardware
{
	class FeedbackCacheEntry
	{
		public:
			FeedbackCacheEntry() = delete;

			inline FeedbackCacheEntry(const ControlID controlId)
			:	controlId(controlId),
				feedbackId(FeedbackNone),
				pin(FeedbackPinNone),
				matchKey("")
			{
			}

			inline ControlID GetControlID() const
			{
				return controlId;
			}

			inline FeedbackID GetFeedbackID() const
			{
				return feedbackId;
			}

			inline void SetFeedbackID(const FeedbackID feedbackId)
			{
				this->feedbackId = feedbackId;
			}

			inline const std::string& GetName() const
			{
				return name;
			}

			inline void SetName(const std::string& name)
			{
				this->name = name;
			}

			inline FeedbackPin GetPin() const
			{
				return pin;
			}

			inline void SetPin(const FeedbackPin pin)
			{
				this->pin = pin;
			}

			inline const std::string& GetMatchKey() const
			{
				return matchKey;
			}

			inline void SetMatchKey(const std::string& matchKey)
			{
				this->matchKey = matchKey;
			}

			inline void SetMatchKey(const unsigned int matchKey)
			{
				this->matchKey = std::to_string(matchKey);
			}

		private:
			const ControlID controlId;
			FeedbackID feedbackId;
			std::string name;
			FeedbackPin pin;
			std::string matchKey;
	};

	class FeedbackCache
	{
		public:
			inline FeedbackCache(const ControlID controlId,
				Manager* const manager)
			:	controlId(controlId),
				manager(manager)
			{
			}

			FeedbackCache() = delete;

			FeedbackCache(const FeedbackCache& rhs) = delete;

			FeedbackCache& operator= (const FeedbackCache& rhs) = delete;

			inline ControlID GetControlId() const
			{
				return controlId;
			}

			inline void Save(FeedbackCacheEntry& entry)
			{
				const std::string& oldMatchKey = entry.GetMatchKey();
				Save(entry, oldMatchKey);
			}

			void Save(FeedbackCacheEntry& entry, const std::string& oldMatchKey);

			FeedbackID Delete(const std::string& matchKey);

			inline const FeedbackCacheEntry Get(const std::string& matchKey) const
			{
				return entries.count(matchKey) == 0 ? FeedbackCacheEntry(controlId) : entries.at(matchKey);
			}

			inline const std::map<std::string,FeedbackCacheEntry>& GetAll() const
			{
				return entries;
			}

			void SetFeedbackId(const FeedbackID locoId, const std::string& matckKey);

		private:
			const ControlID controlId;
			Manager* const manager;
			std::map<std::string,FeedbackCacheEntry> entries;
	};
} // namespace Hardware

