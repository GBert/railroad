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

#include <string>
#include <vector>

#include "Logger/Logger.h"
#include "Storage/Sqlite.h"
#include "Storage/StorageHandler.h"
#include "Utils/Utils.h"

using DataModel::Accessory;
using DataModel::Cluster;
using DataModel::Feedback;
using DataModel::Layer;
using DataModel::Loco;
using DataModel::MultipleUnit;
using DataModel::Relation;
using DataModel::Route;
using DataModel::Signal;
using DataModel::Switch;
using DataModel::Text;
using DataModel::Track;
using std::map;
using std::string;
using std::vector;

namespace Storage
{
	void StorageHandler::AllLocos(map<LocoID,DataModel::Loco*>& locos)
	{
		vector<string> serializedObjects;
		sqlite.ObjectsOfType(ObjectTypeLoco, serializedObjects);
		for (auto& serializedObject : serializedObjects)
		{
			Loco* loco = new Loco(manager, serializedObject);
			locos[loco->GetID()] = loco;
		}
	}

	void StorageHandler::DeleteLoco(const LocoID locoID)
	{
		sqlite.DeleteRelationsFrom(DataModel::Relation::RelationTypeLocoSlave, locoID);
		sqlite.DeleteRelationsTo(ObjectTypeLoco, locoID);
		sqlite.DeleteObject(ObjectTypeLoco, locoID);
	}

	void StorageHandler::AllMultipleUnits(map<MultipleUnitID,DataModel::MultipleUnit*>& multipleUnits)
	{
		vector<string> serializedObjects;
		sqlite.ObjectsOfType(ObjectTypeMultipleUnit, serializedObjects);
		for (auto& serializedObject : serializedObjects)
		{
			MultipleUnit* multipleUnit = new MultipleUnit(manager, serializedObject);
			const MultipleUnitID multipleUnitID = multipleUnit->GetID();
			multipleUnit->AssignSlaves(RelationsFrom(DataModel::Relation::RelationTypeMultipleUnitLoco, multipleUnitID));
			multipleUnits[multipleUnitID] = multipleUnit;
		}
	}

	void StorageHandler::DeleteMultipleUnit(const MultipleUnitID multipleUnitID)
	{
		sqlite.DeleteRelationsFrom(DataModel::Relation::RelationTypeMultipleUnitLoco, multipleUnitID);
		sqlite.DeleteRelationsTo(ObjectTypeMultipleUnit, multipleUnitID);
		sqlite.DeleteObject(ObjectTypeMultipleUnit, multipleUnitID);
	}

	void StorageHandler::AllAccessories(std::map<AccessoryID,DataModel::Accessory*>& accessories)
	{
		vector<string> serializedObjects;
		sqlite.ObjectsOfType(ObjectTypeAccessory, serializedObjects);
		for (auto& serializedObject : serializedObjects)
		{
			Accessory* accessory = new Accessory(serializedObject);
			if (accessory == nullptr)
			{
				continue;
			}
			accessories[accessory->GetID()] = accessory;
		}
	}

	void StorageHandler::AllFeedbacks(std::map<FeedbackID,DataModel::Feedback*>& feedbacks)
	{
		vector<string> serializedObjects;
		sqlite.ObjectsOfType(ObjectTypeFeedback, serializedObjects);
		for (auto& serializedObject : serializedObjects)
		{
			Feedback* feedback = new Feedback(manager, serializedObject);
			if (feedback == nullptr)
			{
				continue;
			}
			feedbacks[feedback->GetID()] = feedback;
		}
	}

	void StorageHandler::AllTracks(std::map<TrackID,DataModel::Track*>& tracks)
	{
		vector<string> serializedObjects;
		sqlite.ObjectsOfType(ObjectTypeTrack, serializedObjects);
		for (auto& serializedObject : serializedObjects)
		{
			Track* track = new Track(manager, serializedObject);
			if (track == nullptr)
			{
				continue;
			}
			const TrackID trackId = track->GetID();

			// FIXME: remove later: 2024-03-22 feedback vector has been replaced by relation
			// feedback vector was stored in track data
			// if no feedbacks are restored from track data override with data from relation
			const std::vector<DataModel::Relation*>& feedbacksFromOldRelation = track->GetFeedbacks();
			if (feedbacksFromOldRelation.size() == 0)
			{
				track->AssignFeedbacks(RelationsFrom(DataModel::Relation::RelationTypeTrackFeedback, trackId));
			}
			track->AssignSignals(RelationsFrom(DataModel::Relation::RelationTypeTrackSignal, trackId));
			tracks[trackId] = track;
		}
	}

	void StorageHandler::DeleteTrack(const TrackID trackID)
	{
		sqlite.DeleteRelationsFrom(Relation::RelationTypeTrackSignal, trackID);
		sqlite.DeleteRelationsFrom(Relation::RelationTypeTrackFeedback, trackID);
		sqlite.DeleteRelationsTo(ObjectTypeTrack, trackID);
		sqlite.DeleteObject(ObjectTypeTrack, trackID);
	}

	void StorageHandler::AllSwitches(std::map<SwitchID,DataModel::Switch*>& switches)
	{
		vector<string> serializedObjects;
		sqlite.ObjectsOfType(ObjectTypeSwitch, serializedObjects);
		for (auto& serializedObject : serializedObjects)
		{
			Switch* mySwitch = new Switch(serializedObject);
			if (mySwitch == nullptr)
			{
				continue;
			}
			switches[mySwitch->GetID()] = mySwitch;
		}
	}

	void StorageHandler::Save(const DataModel::Route& route)
	{
		const string serialized = route.Serialize();
		const RouteID routeID = route.GetID();
		sqlite.SaveObject(ObjectTypeRoute, routeID, route.GetName(), serialized);
		sqlite.DeleteRelationsFrom(DataModel::Relation::RelationTypeRouteAtLock, routeID);
		SaveRelations(route.GetRelationsAtLock());
		sqlite.DeleteRelationsFrom(DataModel::Relation::RelationTypeRouteAtUnlock, routeID);
		SaveRelations(route.GetRelationsAtUnlock());
	}

	void StorageHandler::Save(const DataModel::Loco& loco)
	{
		const string serialized = loco.Serialize();
		const LocoID locoID = loco.GetID();
		sqlite.SaveObject(ObjectTypeLoco, locoID, loco.GetName(), serialized);
		sqlite.DeleteRelationsFrom(DataModel::Relation::RelationTypeLocoSlave, locoID);
	}

	void StorageHandler::Save(const DataModel::MultipleUnit& multipleUnit)
	{
		const string serialized = multipleUnit.Serialize();
		const MultipleUnitID multipleUnitID = multipleUnit.GetID();
		sqlite.SaveObject(ObjectTypeMultipleUnit, multipleUnitID, multipleUnit.GetName(), serialized);
		sqlite.DeleteRelationsFrom(DataModel::Relation::RelationTypeMultipleUnitLoco, multipleUnitID);
		SaveRelations(multipleUnit.GetSlaves());
	}

	void StorageHandler::Save(const DataModel::Cluster& cluster)
	{
		const string serialized = cluster.Serialize();
		const ClusterID clusterID = cluster.GetID();
		sqlite.SaveObject(ObjectTypeCluster, clusterID, cluster.GetName(), serialized);
		sqlite.DeleteRelationsFrom(DataModel::Relation::RelationTypeClusterTrack, clusterID);
		SaveRelations(cluster.GetTracks());
	}

	void StorageHandler::Save(const DataModel::Track& track)
	{
		const string serialized = track.Serialize();
		const TrackID trackId = track.GetID();
		sqlite.SaveObject(ObjectTypeTrack, trackId, track.GetName(), serialized);
		sqlite.DeleteRelationsFrom(DataModel::Relation::RelationTypeTrackFeedback, trackId);
		SaveRelations(track.GetFeedbacks());
		sqlite.DeleteRelationsFrom(DataModel::Relation::RelationTypeTrackSignal, trackId);
		SaveRelations(track.GetSignals());
	}

	void StorageHandler::AllRoutes(std::map<RouteID,DataModel::Route*>& routes)
	{
		vector<string> serializedObjects;
		sqlite.ObjectsOfType(ObjectTypeRoute, serializedObjects);
		for (auto& serializedObject : serializedObjects)
		{
			Route* route = new Route(manager, serializedObject);
			if (route == nullptr)
			{
				continue;
			}
			const RouteID routeID = route->GetID();
			route->AssignRelationsAtLock(RelationsFrom(Relation::RelationTypeRouteAtLock, routeID));
			route->AssignRelationsAtUnlock(RelationsFrom(Relation::RelationTypeRouteAtUnlock, routeID));
			routes[routeID] = route;
		}
	}

	void StorageHandler::DeleteRoute(const RouteID routeID)
	{
		sqlite.DeleteRelationsFrom(DataModel::Relation::RelationTypeRouteAtLock, routeID);
		sqlite.DeleteRelationsFrom(DataModel::Relation::RelationTypeRouteAtUnlock, routeID);
		sqlite.DeleteObject(ObjectTypeRoute, routeID);
	}

	void StorageHandler::AllLayers(std::map<LayerID,DataModel::Layer*>& layers)
	{
		vector<string> serializedObjects;
		sqlite.ObjectsOfType(ObjectTypeLayer, serializedObjects);
		for (auto& serializedObject : serializedObjects)
		{
			Layer* layer = new Layer(serializedObject);
			if (layer == nullptr)
			{
				continue;
			}
			layers[layer->GetID()] = layer;
		}
	}

	void StorageHandler::AllSignals(std::map<SignalID,DataModel::Signal*>& signals)
	{
		vector<string> serializedObjects;
		sqlite.ObjectsOfType(ObjectTypeSignal, serializedObjects);
		for (auto& serializedObject : serializedObjects)
		{
			Signal* signal = new Signal(manager, serializedObject);
			if (signal == nullptr)
			{
				continue;
			}
			signals[signal->GetID()] = signal;
		}
	}

	void StorageHandler::DeleteSignal(const SignalID signalID)
	{
		sqlite.DeleteRelationsTo(ObjectTypeSignal, signalID);
		sqlite.DeleteObject(ObjectTypeSignal, signalID);
	}

	void StorageHandler::AllClusters(std::map<ClusterID,DataModel::Cluster*>& clusters)
	{
		vector<string> serializedObjects;
		sqlite.ObjectsOfType(ObjectTypeCluster, serializedObjects);
		for (auto& serializedObject : serializedObjects)
		{
			Cluster* cluster = new Cluster(serializedObject);
			if (cluster == nullptr)
			{
				continue;
			}
			const ClusterID clusterId = cluster->GetID();
			cluster->AssignTracks(RelationsFrom(DataModel::Relation::RelationTypeClusterTrack, clusterId));
			clusters[clusterId] = cluster;
		}
	}

	void StorageHandler::AllTexts(std::map<TextID,DataModel::Text*>& texts)
	{
		vector<string> serializedObjects;
		sqlite.ObjectsOfType(ObjectTypeText, serializedObjects);
		for (auto& serializedObject : serializedObjects)
		{
			Text* text = new Text(serializedObject);
			if (text == nullptr)
			{
				continue;
			}
			texts[text->GetID()] = text;
		}
	}

	void StorageHandler::SaveRelations(const vector<DataModel::Relation*> relations)
	{
		for (auto relation : relations)
		{
			string serializedRelation = relation->Serialize();
			sqlite.SaveRelation(relation->GetType(), relation->ObjectID1(), relation->ObjectType2(), relation->ObjectID2(), relation->GetPriority(), serializedRelation);
		}
	}

	vector<Relation*> StorageHandler::RelationsFrom(const DataModel::Relation::RelationType type, const ObjectID objectID)
	{
		vector<string> relationStrings;
		sqlite.RelationsFrom(type, objectID, relationStrings);
		vector<Relation*> output;
		for (auto& relationString : relationStrings)
		{
			Relation* relation = new Relation(manager, relationString);
			if (relation == nullptr)
			{
				continue;
			}
			output.push_back(relation);
		}
		return output;
	}
} // namespace Storage

