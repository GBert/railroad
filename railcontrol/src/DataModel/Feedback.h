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

#pragma once

#include <mutex>
#include <string>

#include "DataTypes.h"
#include "DataModel/ObjectIdentifier.h"
#include "DataModel/LayoutItem.h"
#include "Hardware/FeedbackCache.h"
#include "Languages.h"
#include "Logger/Logger.h"

class Manager;

namespace DataModel
{
	class Track;

	enum FeedbackType : unsigned char
	{
		FeedbackTypeDefault = 0,
		FeedbackTypeStraight = 1,
		FeedbackTypeTurn = 2
	};

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
				device(FeedbackDeviceNone),
				bus(FeedbackBusNone),
			 	manager(manager),
			 	feedbackType(FeedbackTypeDefault),
			 	routeId(RouteNone),
			 	inverted(false),
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

			void Deserialize(const std::string& serialized) override;

			inline bool IsInUse() const
			{
				return (!track);
			}

			inline std::string GetLayoutType() const override
			{
				return Languages::GetText(Languages::TextFeedback);
			}

			inline FeedbackType GetFeedbackType() const
			{
				return feedbackType;
			}

			inline void SetFeedbackType(const FeedbackType type)
			{
				this->feedbackType = type;
			}

			inline RouteID GetRouteId() const
			{
				return routeId;
			}

			inline void SetRouteId(const RouteID routeId)
			{
				this->routeId = routeId;
			}

			inline void SetInverted(const bool inverted)
			{
				this->inverted = inverted;
			}

			inline bool GetInverted() const
			{
				return inverted;
			}

			void SetState(Logger::Logger* logger,
				const FeedbackState state);

			inline FeedbackState GetState() const
			{
				return static_cast<FeedbackState>(stateCounter > 0);
			}

			inline bool CheckState(const FeedbackState state) const
			{
				return (GetState() == state);
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

			inline void SetDevice(const FeedbackDevice device)
			{
				this->device = device;
			}

			inline FeedbackDevice GetDevice() const
			{
				return device;
			}

			inline void SetBus(const FeedbackBus bus)
			{
				this->bus = bus;
			}

			inline FeedbackBus GetBus() const
			{
				return bus;
			}

			inline bool CheckControl(const LayerID layer) const
			{
				const ControlID controlID = (-layer) >> 10;
				const FeedbackDevice device = ((-layer) >> 2) & 0x000000FF;
				const FeedbackBus bus = (-layer) & 0x00000003;
				return CheckControl(controlID, device, bus);
			}

			inline bool CheckControl(const ControlID controlID, const FeedbackDevice device, const FeedbackBus bus) const
			{
				return (GetControlID() == controlID) && (GetDevice() == device) && (GetBus() == bus);
			}

			inline void SetTrack(DataModel::Track* track = nullptr)
			{
				this->track = track;
			}

			inline Track* GetTrack()
			{
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

			Feedback& operator=(const Hardware::FeedbackCacheEntry& feedback);

		private:
			void UpdateTrackState(const FeedbackState state);

			ControlID controlID;
			FeedbackPin pin;
			FeedbackDevice device;
			FeedbackBus bus;

			Manager* manager;
			FeedbackType feedbackType;
			RouteID routeId;
			bool inverted;
			Track* track;
			unsigned char stateCounter;
			static const unsigned char MaxStateCounter = 10;
			mutable std::mutex updateMutex;
			std::string matchKey;
	};

} // namespace DataModel

