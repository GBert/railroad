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

#include <mutex>
#include <string>
#include <vector>

#include "DataModel/Cluster.h"
#include "DataModel/Feedback.h"
#include "DataModel/LayoutItem.h"
#include "DataModel/LockableItem.h"
#include "DataModel/Relation.h"
#include "DataTypes.h"
#include "Logger/Logger.h"

class Manager;

namespace DataModel
{
	class LocoBase;
	class Route;

	enum TrackType : unsigned char
	{
		TrackTypeStraight = 0,
		TrackTypeTurn = 1,
		TrackTypeEnd = 2,
		TrackTypeBridge = 3,
		TrackTypeTunnel = 4,
		TrackTypeTunnelEnd = 5,
		TrackTypeLink = 6,
		TrackTypeCrossingLeft = 7,
		TrackTypeCrossingRight = 8,
		TrackTypeCrossingSymetric = 9
	};

	enum SelectRouteApproach : unsigned char
	{
		SelectRouteSystemDefault = 0,
		SelectRouteDoNotCare = 1,
		SelectRouteRandom = 2,
		SelectRouteMinTrackLength = 3,
		SelectRouteLongestUnused = 4
	};

	class Track : public LayoutItem, public LockableItem
	{
		public:
			static const LayoutItem::LayoutItemSize MinLength = 1;
			static const LayoutItem::LayoutItemSize MaxLength = 100;

			inline Track(Manager* manager, const TrackID trackID)
			:	LayoutItem(trackID),
				LockableItem(),
				manager(manager),
				trackType(TrackTypeStraight),
				cluster(nullptr),
				selectRouteApproach(SelectRouteSystemDefault),
				trackState(DataModel::Feedback::FeedbackStateFree),
				trackStateDelayed(DataModel::Feedback::FeedbackStateFree),
				locoOrientation(OrientationRight),
				blocked(false),
				locoBaseDelayed(),
				allowLocoTurn(true),
				releaseWhenFree(false),
				showName(true)
			{
			}

			inline Track(Manager* manager, const std::string& serialized)
			:	Track(manager, TrackNone)
			{
				Deserialize(serialized);
			}

			inline ~Track()
			{
				DeleteSignals();
				DeleteFeedbacks();
			}

			inline ObjectType GetObjectType() const override
			{
				return ObjectTypeTrack;
			}

			std::string Serialize() const override;
			bool Deserialize(const std::string& serialized) override;

			inline std::string GetLayoutType() const override
			{
				return Languages::GetText(Languages::TextTrack);
			}

			inline TrackType GetTrackType() const
			{
				return trackType;
			}

			inline void SetTrackType(const TrackType type)
			{
				this->trackType = type;
			}

			bool SetFeedbackState(const FeedbackID feedbackID, const DataModel::Feedback::FeedbackState state);

			inline DataModel::Feedback::FeedbackState GetFeedbackStateDelayed() const
			{
				return trackStateDelayed;
			}

			bool AddRoute(Route* route);
			bool RemoveRoute(Route* route);

			inline SelectRouteApproach GetSelectRouteApproach() const
			{
				return selectRouteApproach;
			}

			inline void SetSelectRouteApproach(const SelectRouteApproach selectRouteApproach)
			{
				this->selectRouteApproach = selectRouteApproach;
			}

			inline const std::vector<const Route*> GetRoutes() const
			{
				std::vector<const Route*> out;
				std::lock_guard<std::mutex> Guard(updateMutex);
				for (auto route : routes)
				{
					out.push_back(route);
				}
				return out;
			}

			bool GetValidRoutes(Logger::Logger* logger,
				const DataModel::LocoBase* loco,
				const bool allowLocoTurn,
				std::vector<Route*>& validRoutes) const;

			inline Orientation GetLocoOrientation() const
			{
				return locoOrientation;
			}

			inline bool CanSetLocoBaseOrientation(const Orientation orientation, const ObjectIdentifier& locoBaseIdentifier)
			{
				return cluster == nullptr ? true : cluster->CanSetLocoBaseOrientation(orientation, locoBaseIdentifier);
			}

			bool SetLocoOrientation(const Orientation orientation);

			inline bool GetBlocked() const
			{
				return blocked;
			}

			inline void SetBlocked(const bool blocked)
			{
				this->blocked = blocked;
			}

			inline ObjectIdentifier GetLocoBaseDelayed() const
			{
				return this->locoBaseDelayed;
			}

			inline bool GetReleaseWhenFree() const
			{
				return releaseWhenFree;
			}

			inline void SetReleaseWhenFree(const bool releaseWhenFree)
			{
				this->releaseWhenFree = releaseWhenFree;
			}

			inline bool GetShowName() const
			{
				return this->showName;
			}

			inline void SetShowName(const bool showName)
			{
				this->showName = showName;
			}

			inline const std::string& GetDisplayName() const
			{
				return this->displayName;
			}

			inline void SetDisplayName(const std::string& displayName)
			{
				this->displayName = displayName;
			}

			inline DataModel::Cluster* GetCluster() const
			{
				return cluster;
			}

			inline void SetCluster(DataModel::Cluster* const cluster)
			{
				this->cluster = cluster;
			}

			inline bool GetAllowLocoTurn() const
			{
				return allowLocoTurn;
			}

			inline void SetAllowLocoTurn(bool allowLocoTurn)
			{
				this->allowLocoTurn = allowLocoTurn;
			}

			bool Reserve(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier) override;
			bool Lock(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier) override;
			bool Release(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier) override;

			bool ReserveForce(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier);
			bool ReleaseForce(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier);

			void DeleteFeedbacks();
			void AssignFeedbacks(const std::vector<DataModel::Relation*>& newFeedbacks);

			inline const std::vector<DataModel::Relation*>& GetSignals() const
			{
				return signals;
			}

			void DeleteSignals();
			void AssignSignals(const std::vector<DataModel::Relation*>& newSignals);

			inline const std::vector<DataModel::Relation*>& GetFeedbacks() const
			{
				return feedbacks;
			}

		private:
			void PublishState() const;

			void StopAllSignals(const ObjectIdentifier& locoBaseIdentifier);

			bool FeedbackStateInternal(const FeedbackID feedbackID, const DataModel::Feedback::FeedbackState state);
			void OrderValidRoutes(std::vector<DataModel::Route*>& validRoutes) const;
			SelectRouteApproach GetSelectRouteApproachCalculated() const;
			bool ReleaseForceUnlocked(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier);

			Manager* manager;
			TrackType trackType;

			mutable std::mutex updateMutex;
			std::vector<DataModel::Relation*> feedbacks;
			std::vector<DataModel::Relation*> signals;
			Cluster* cluster;
			SelectRouteApproach selectRouteApproach;
			DataModel::Feedback::FeedbackState trackState;
			DataModel::Feedback::FeedbackState trackStateDelayed;
			std::vector<Route*> routes;
			Orientation locoOrientation;
			bool blocked;
			ObjectIdentifier locoBaseDelayed;
			bool allowLocoTurn;
			bool releaseWhenFree;
			bool showName;
			std::string displayName;
	};
} // namespace DataModel
