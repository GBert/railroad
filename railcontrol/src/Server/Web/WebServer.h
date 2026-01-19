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

#include <map>
#include <mutex>
#include <sstream>
#include <vector>

#include "ControlInterface.h"
#include "DataModel/AccessoryBase.h"
#include "Logger/Logger.h"
#include "Manager.h"
#include "Network/TcpServer.h"

namespace Server { namespace Web
{
	class WebClient;

	class WebServer : public ControlInterface, private Network::TcpServer
	{
		public:
			WebServer() = delete;
			WebServer(const WebServer&) = delete;
			WebServer& operator=(const WebServer&) = delete;

			WebServer(Manager& manager, const std::string& webserveraddress, const unsigned short port);
			~WebServer();

			void Start() override;

			void Stop() override;

			void Work(Network::TcpConnection* connection) override;

			void Warning(__attribute__((unused)) Languages::TextSelector textSelector) override
			{
				AddUpdate("warning", textSelector);
			}

			bool NextUpdate(unsigned int& updateIDClient, std::string& s);

			inline const std::string& GetName() const override
			{
				static const std::string WebserverName("WebServer");
				return WebserverName;
			}

			void AccessoryDelete(const AccessoryID accessoryID, const std::string& name, const std::string& matchKey) override;
			void AccessorySettings(const AccessoryID accessoryID, const std::string& name, const std::string& matchkey) override;
			void AccessoryState(const ControlType controlType, const DataModel::Accessory* accessory) override;
			void Booster(const ControlType controlType, const BoosterState status) override;
			void FeedbackDelete(const FeedbackID feedbackID, const std::string& name) override;
			void FeedbackSettings(const FeedbackID feedbackID, const std::string& name) override;
			void FeedbackState(const std::string& name, const FeedbackID feedbackID, const DataModel::Feedback::FeedbackState state) override;
			void LayerDelete(const LayerID layerID, const std::string& name) override;
			void LayerSettings(const LayerID layerID, const std::string& name) override;

			void LocoBaseDestinationReached(const DataModel::ObjectIdentifier& locoIdentifier,
				const std::string& locoName,
				const RouteID routeID,
				const std::string& routeName,
				const TrackID trackID,
				const std::string& trackName) override;

			void LocoBaseSpeed(const ControlType controlType,
				const DataModel::LocoConfig& locoConfig) override;

			void LocoBaseOrientation(const ControlType controlType,
				const DataModel::LocoConfig& locoConfig) override;

			void LocoBaseFunctionState(const ControlType controlType,
				const DataModel::LocoConfig& locoConfig,
				const DataModel::LocoFunctionNr function) override;

			void LocoBaseRelease(const DataModel::ObjectIdentifier& locolocoIdentifier,
				const std::string& locoName) override;

			void LocoBaseStart(const DataModel::ObjectIdentifier& locoIdentifier,
				const std::string& locoName) override;

			void LocoBaseStop(const DataModel::ObjectIdentifier& locoIdentifier,
				const std::string& locoName) override;

			void LocoSettings(const LocoID locoID,
				const std::string& name,
				const std::string& matchKey) override;

			void LocoDelete(const LocoID locoID,
				const std::string& name,
				const std::string& matchKey) override;

			void MultipleUnitSettings(const MultipleUnitID multipleUnitID,
				const std::string& name,
				const std::string& matchKey) override;

			void MultipleUnitDelete(const MultipleUnitID multipleUnitID,
				const std::string& name,
				const std::string& matchkey) override;

			void RouteDelete(const RouteID routeID, const std::string& name) override;
			void RouteRelease(const RouteID routeID) override;
			void RouteSettings(const RouteID routeID, const std::string& name) override;
			void SwitchDelete(const SwitchID switchID, const std::string& name, const std::string& matchKey) override;
			void SwitchSettings(const SwitchID switchID, const std::string& name, const std::string& matchKey) override;
			void SwitchState(const ControlType controlType, const DataModel::Switch* mySwitch) override;
			void TrackDelete(const TrackID trackID, const std::string& name) override;
			void TrackSettings(const TrackID trackID, const std::string& name) override;
			void TrackState(const DataModel::Track* track) override;
			void SignalDelete(const SignalID signalID, const std::string& name, const std::string& matchKey) override;
			void SignalSettings(const SignalID signalID, const std::string& name, const std::string& matchKey) override;
			void SignalState(const ControlType controlType, const DataModel::Signal* signal) override;
			void ClusterDelete(const ClusterID clusterID, const std::string& name) override;
			void ClusterSettings(const ClusterID clusterID, const std::string& name) override;
			void TextDelete(const TextID textID, const std::string& name) override;
			void TextSettings(const TextID textID, const std::string& name) override;
			void CounterDelete(const CounterID counterID, const std::string& name) override;
			void CounterSettings(const CounterID counterID, const std::string& name) override;
			void CounterState(const DataModel::Counter* const counter) override;
			void ProgramValue(const CvNumber cv, const CvValue value) override;

			inline bool UpdateAvailable()
			{
				return updateAvailable;
			}

			inline void AddUpdate(const std::string& command, const Languages::TextSelector status)
			{
				AddUpdate(command, Languages::GetText(status));
			}

		private:
			template<typename... Args>
			inline void AddUpdate(const std::string& command, const Languages::TextSelector text, Args... args)
			{
				AddUpdate(command, Logger::Logger::Format(Languages::GetText(text), args...));
			}

			inline void AddUpdate(const std::string& command, const std::string& status)
			{
				AddUpdateInternal("data: command=" + command + ";status=" + status + "\r\n\r\n");
			}

			inline void AddUpdate(const std::string& command)
			{
				AddUpdateInternal("data: command=" + command + "\r\n\r\n");
			}

			inline void AddUpdate(const Languages::TextSelector status)
			{
				AddUpdate(std::string("data: status=") + Languages::GetText(status) + "\r\n\r\n");
			}

			void AddUpdateInternal(const std::string& data);

			void LogBrowserInfo(const std::string& webserveraddress, const unsigned short port);

			static inline LocoID LocoIDWithPrefix(const LocoID locoID, const LocoType type)
			{
				return locoID + (type == LocoTypeMultipleUnit ? MultipleUnitIdPrefix : 0);
			}

			Logger::Logger* logger;
			unsigned int lastClientID;
			std::vector<WebClient*> clients;
			Manager& manager;

			std::map<unsigned int,std::string> updates;
			std::mutex updateMutex;
			unsigned int updateID;
			bool updateAvailable;

			static const unsigned int MaxUpdates = 10;
	};
}} // namespace Server::Web

