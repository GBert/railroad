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

#include <string>

#include "DataModel/Feedback.h"
#include "Hardware/FeedbackCache.h"

namespace DataModel
{
	class FeedbackConfig
	{
		public:
			inline FeedbackConfig()
			:	controlId(ControlNone),
				feedbackId(FeedbackNone),
				pin(FeedbackPinNone),
				isInUse(false)
			{
			}

			inline FeedbackConfig(const DataModel::Feedback& feedback)
			:	controlId(feedback.GetControlID()),
				feedbackId(feedback.GetID()),
				pin(feedback.GetPin()),
				name(feedback.GetName()),
				matchKey(feedback.GetMatchKey()),
				isInUse(feedback.IsInUse())
			{
			}

			inline FeedbackConfig(const Hardware::FeedbackCacheEntry& feedback)
			:	controlId(feedback.GetControlID()),
				feedbackId(feedback.GetFeedbackID()),
				pin(feedback.GetPin()),
				name(feedback.GetName()),
				matchKey(feedback.GetMatchKey()),
				isInUse(false)
			{
			}

			inline FeedbackConfig& operator=(const DataModel::Feedback& feedback)
			{
				controlId = feedback.GetControlID();
				feedbackId = feedback.GetID();
				pin = feedback.GetPin();
				name = feedback.GetName();
				matchKey = feedback.GetMatchKey();
				isInUse = feedback.IsInUse();
				return *this;
			}

			inline FeedbackConfig& operator=(const Hardware::FeedbackCacheEntry& feedback)
			{
				controlId = feedback.GetControlID();
				feedbackId = feedback.GetFeedbackID();
				pin = feedback.GetPin();
				name = feedback.GetName();
				matchKey = feedback.GetMatchKey();
				return *this;
			}

			inline ControlID GetControlId() const
			{
				return controlId;
			}

			inline FeedbackID GetFeedbackId() const
			{
				return feedbackId;
			}

			inline FeedbackPin GetPin() const
			{
				return pin;
			}

			inline std::string GetName() const
			{
				return name;
			}

			inline void SetName(const std::string& name)
			{
				this->name = name;
			}

			inline std::string GetMatchKey() const
			{
				return matchKey;
			}

			inline bool IsInUse() const
			{
				return isInUse;
			}

		private:
			ControlID controlId;
			FeedbackID feedbackId;
			FeedbackPin pin;
			std::string name;
			std::string matchKey;
			bool isInUse;
	};
} // namespace DataModel
