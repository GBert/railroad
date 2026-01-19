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

#include <vector>
#include <map>

#include "DataModel/DataModel.h"
#include "DataTypes.h"
#include "Hardware/HardwareParams.h"
#include "Storage/Sqlite.h"
#include "Storage/StorageInterface.h"
#include "Storage/TransactionGuard.h"
#include "Storage/StorageParams.h"

namespace Storage
{
	class StorageHandler
	{
		public:
			inline StorageHandler(Manager* manager, const StorageParams* params)
			:	manager(manager),
				sqlite(params)
			{
			}

			inline ~StorageHandler()
			{
			}

			inline void AllHardwareParams(std::map<ControlID,Hardware::HardwareParams*>& hardwareParams)
			{
				sqlite.AllHardwareParams(hardwareParams);
			}

			inline void DeleteHardwareParams(const ControlID controlID)
			{
				sqlite.DeleteHardwareParams(controlID);
			}

			void AllLocos(std::map<LocoID,DataModel::Loco*>& locos);
			void DeleteLoco(LocoID locoID);
			void AllMultipleUnits(std::map<LocoID,DataModel::MultipleUnit*>& multipleUnits);
			void DeleteMultipleUnit(MultipleUnitID multipleUnitID);
			void AllAccessories(std::map<AccessoryID,DataModel::Accessory*>& accessories);

			inline void DeleteAccessory(AccessoryID accessoryID)
			{
				sqlite.DeleteObject(ObjectTypeAccessory, accessoryID);
			}

			void AllFeedbacks(std::map<FeedbackID,DataModel::Feedback*>& feedbacks);

			inline void DeleteFeedback(FeedbackID feedbackID)
			{
				sqlite.DeleteObject(ObjectTypeFeedback, feedbackID);
			}

			void AllTracks(std::map<TrackID,DataModel::Track*>& tracks);
			void DeleteTrack(TrackID trackID);
			void AllSwitches(std::map<SwitchID,DataModel::Switch*>& switches);

			inline void DeleteSwitch(SwitchID switchID)
			{
				sqlite.DeleteObject(ObjectTypeSwitch, switchID);
			}

			void AllRoutes(std::map<RouteID,DataModel::Route*>& routes);
			void DeleteRoute(RouteID routeID);
			void AllLayers(std::map<LayerID,DataModel::Layer*>& layers);

			inline void DeleteLayer(LayerID layerID)
			{
				sqlite.DeleteObject(ObjectTypeLayer, layerID);
			}

			void AllSignals(std::map<SignalID,DataModel::Signal*>& signals);
			void DeleteSignal(SignalID signalID);
			void AllClusters(std::map<ClusterID,DataModel::Cluster*>& clusters);

			inline void DeleteCluster(ClusterID clusterID)
			{
				sqlite.DeleteObject(ObjectTypeCluster, clusterID);
			}

			void AllTexts(std::map<TextID,DataModel::Text*>& texts);

			inline void DeleteText(TextID textID)
			{
				sqlite.DeleteObject(ObjectTypeText, textID);
			}

			void AllCounters(std::map<CounterID,DataModel::Counter*>& counters);

			inline void DeleteCounter(CounterID counterID)
			{
				sqlite.DeleteObject(ObjectTypeCounter, counterID);
			}

			inline void Save(const Hardware::HardwareParams& hardwareParams)
			{
				sqlite.SaveHardwareParams(hardwareParams);
			}

			void Save(const DataModel::Route& route);
			void Save(const DataModel::Loco& loco);
			void Save(const DataModel::MultipleUnit& multipleUnit);
			void Save(const DataModel::Cluster& cluster);
			void Save(const DataModel::Track& track);

			template<class T> void Save(const T& t)
			{
				const std::string serialized = t.Serialize();
				sqlite.SaveObject(t.GetObjectType(), t.GetID(), t.GetName(), serialized);
			}

			template <class T> static void Save(StorageHandler* storageHandler, const T* t)
			{
				storageHandler->Save(*t);
			}

			inline void SaveSetting(const std::string& key, const std::string& value)
			{
				sqlite.SaveSetting(key, value);
			}


			inline std::string GetSetting(const std::string& key)
			{
				return sqlite.GetSetting(key);
			}

			inline void StartTransaction()
			{
				transactionMutex.lock();
				sqlite.StartTransaction();
			}

			inline void CommitTransaction()
			{
				sqlite.CommitTransaction();
				transactionMutex.unlock();
			}

		private:
			void SaveRelations(const std::vector<DataModel::Relation*> relations);
			std::vector<DataModel::Relation*> RelationsFrom(const DataModel::Relation::RelationType type, const ObjectID objectID);

			Manager* manager;
			Storage::SQLite sqlite;
			std::mutex transactionMutex;
	};
} // namespace Storage

