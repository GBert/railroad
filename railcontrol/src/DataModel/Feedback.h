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

#include <mutex>
#include <string>

#include "DataTypes.h"
#include "DataModel/ObjectIdentifier.h"
#include "DataModel/LayoutItem.h"
#include "Hardware/FeedbackCache.h"
#include "Languages.h"

class Manager;

namespace DataModel
{
	class Track;

	class Feedback : public LayoutItem
	{
		public:
			enum FeedbackState : bool
			{
				FeedbackStateFree = false,
				FeedbackStateOccupied = true
			};

			inline Feedback(Manager* manager,
				const FeedbackID feedbackID)
			:	LayoutItem(feedbackID),
			 	controlID(ControlIdNone),
			 	pin(FeedbackPinNone),
			 	manager(manager),
			 	inverted(false),
			 	trackID(TrackNone),
			 	track(nullptr),
				stateCounter(0)
			{
			}

			inline Feedback(Manager* manager, const std::string& serialized)
			:	manager(manager),
				track(nullptr)
			{
				Deserialize(serialized);
			}

			inline ObjectType GetObjectType() const override
			{
				return ObjectTypeFeedback;
			}

			std::string Serialize() const override;

			bool Deserialize(const std::string& serialized) override;

			inline bool IsInUse() const
			{
				return IsRelatedTrackSet();
			}

			inline std::string GetLayoutType() const override
			{
				return Languages::GetText(Languages::TextFeedback);
			}

			inline void SetInverted(const bool inverted)
			{
				this->inverted = inverted;
			}

			inline bool GetInverted() const
			{
				return inverted;
			}

			void SetState(const FeedbackState state);

			inline FeedbackState GetState() const
			{
				return static_cast<FeedbackState>(stateCounter > 0);
			}

			void Debounce();

			inline void SetControlID(const ControlID controlID)
			{
				this->controlID = controlID;
			}

			inline ControlID GetControlID() const
			{
				return controlID;
			}

			inline void SetPin(const FeedbackPin pin)
			{
				this->pin = pin;
			}

			inline FeedbackPin GetPin() const
			{
				return pin;
			}

			inline void ClearRelatedTrack()
			{
				trackID = TrackNone;
				track = nullptr;
			}

			inline bool IsRelatedTrackSet() const
			{
				return trackID != TrackNone;
			}

			inline void SetRelatedTrack(const TrackID trackID)
			{
				this->trackID = trackID;
				track = nullptr;
			}

			inline TrackID GetRelatedTrack() const
			{
				return trackID;
			}

			inline bool CompareRelatedTrack(const TrackID trackID) const
			{
				return this->trackID == trackID;
			}

			inline Track* GetTrack()
			{
				UpdateTrack();
				return track;
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

			Feedback& operator=(const Hardware::FeedbackCacheEntry& loco);

		private:
			void UpdateTrack();
			void UpdateTrackState(const FeedbackState state);

			ControlID controlID;
			FeedbackPin pin;

			Manager* manager;
			bool inverted;
			TrackID trackID;
			Track* track;
			unsigned char stateCounter;
			static const unsigned char MaxStateCounter = 10;
			mutable std::mutex updateMutex;
			std::string matchKey;
	};

} // namespace DataModel

