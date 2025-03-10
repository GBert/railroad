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

#include <map>
#include <string>

#include "DataModel/Cluster.h"
#include "DataModel/LockableItem.h"
#include "DataModel/Relation.h"
#include "DataModel/Signal.h"
#include "DataModel/Track.h"
#include "Manager.h"
#include "Utils/Utils.h"

using std::map;
using std::string;
using std::to_string;

namespace DataModel
{
	std::string Cluster::Serialize() const
	{
		string str;
		str += "objectType=Cluster;";
		str += Object::Serialize();
		str += ";orientation=";
		str += to_string(orientation);
		return str;
	}

	bool Cluster::Deserialize(const string& serialized)
	{
		map<string,string> arguments;
		ParseArguments(serialized, arguments);
		Object::Deserialize(arguments);
		if (!arguments.count("objectType") || arguments.at("objectType").compare("Cluster") != 0)
		{
			return false;
		}
		orientation = static_cast<Orientation>(Utils::Utils::GetBoolMapEntry(arguments, "orientation", OrientationRight));
		return true;
	}

	bool Cluster::CanSetLocoBaseOrientationUnlocked(const Orientation orientation, const ObjectIdentifier& locoBaseIdentifier)
	{
		if (this->orientation == orientation)
		{
			return true;
		}
		for (auto relation : tracks)
		{
			Track* track = dynamic_cast<Track*>(relation->GetObject2());
			if (!track)
			{
				return false;
			}

			// invert track orientation if needed (XOR) and check with desired orientation value
			const Relation::Data invert = relation->GetData();
			if ((track->GetLocoBaseOrientation() ^ invert) == orientation)
			{
				continue;
			}

			const DataModel::LockableItem::LockState lockState = track->GetLockState();
			if (lockState == DataModel::LockableItem::LockStateFree)
			{
				continue;
			}

			const ObjectIdentifier& locoBaseOfTrack = track->GetLocoBase();
			if ((lockState == DataModel::LockableItem::LockStateReserved) && locoBaseOfTrack.IsSet() && (locoBaseOfTrack == locoBaseIdentifier))
			{
				continue;
			}

			return false;
		}
		return true;
	}

	bool Cluster::SetLocoBaseOrientation(const Orientation orientation,
		const ObjectIdentifier& locoBaseIdentifier)
	{
		std::lock_guard<std::mutex> Guard(orientationMutex);
		if (!CanSetLocoBaseOrientationUnlocked(orientation, locoBaseIdentifier))
		{
			return false;
		}
		this->orientation = orientation;
		for (auto relation : tracks)
		{
			Track* track = dynamic_cast<Track*>(relation->GetObject2());
			if (!track)
			{
				continue;
			}
			// invert orientation if needed (XOR) and set track orientation
			const Relation::Data invert = relation->GetData();
			track->SetLocoBaseOrientationForce(static_cast<Orientation>(orientation ^ invert));
		}
		return true;
	}

	void Cluster::DeleteTracks()
	{
		while (tracks.size() > 0)
		{
			Relation* trackRelation = tracks.back();
			Track* track = dynamic_cast<Track*>(trackRelation->GetObject2());
			if (track)
			{
				track->DeleteCluster();
			}
			tracks.pop_back();
			delete trackRelation;
		}
	}

	void Cluster::DeleteTrack(Track* trackToDelete)
	{
		for (unsigned int index = 0; index < tracks.size(); ++index)
		{
			if (tracks[index]->GetObject2() != trackToDelete)
			{
				continue;
			}
			delete tracks[index];
			tracks.erase(tracks.begin() + index);
			trackToDelete->DeleteCluster();
			return;
		}
	}

	void Cluster::AssignTracks(const std::vector<DataModel::Relation*>& newTracks)
	{
		DeleteTracks();
		tracks = newTracks;
		for (auto trackRelation : tracks)
		{
			Track* track = dynamic_cast<Track*>(trackRelation->GetObject2());
			if (track)
			{
				track->SetCluster(this, static_cast<bool>(trackRelation->GetData()));
			}
		}
	}
} // namespace DataModel
