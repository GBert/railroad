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

#include <algorithm>
#include <arpa/inet.h>
#include <cstring>		//memset
#include <ifaddrs.h>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "ControlInterface.h"
#include "DataTypes.h"
#include "DataModel/LocoBase.h"
#include "DataModel/LocoFunctions.h"
#include "DataModel/ObjectIdentifier.h"
#include "Languages.h"
#include "RailControl.h"
#include "Utils/Network.h"
#include "Version.h"
#include "Server/Web/WebClient.h"
#include "Server/Web/WebServer.h"

using std::map;
using std::thread;
using std::string;
using std::stringstream;
using std::to_string;
using std::vector;

using DataModel::LocoBase;
using DataModel::LocoFunctionNr;
using DataModel::LocoFunctionState;
using DataModel::Route;
using DataModel::Track;

namespace Server { namespace Web
{
	WebServer::WebServer(Manager& manager, const std::string& webserveraddress, const unsigned short port)
	:	ControlInterface(ControlTypeWebServer),
		Network::TcpServer(webserveraddress, port, "WebServer"),
		logger(Logger::Logger::GetLogger("WebServer")),
		lastClientID(0),
		manager(manager),
		updateID(1),
		updateAvailable(false)
	{
		AddUpdate(Languages::TextRailControlStarted);

		LogBrowserInfo(webserveraddress, port);
		updateAvailable = Utils::Network::HostResolves(GetVersionInfoGitHash() + ".hash.railcontrol.org");
	}

	WebServer::~WebServer()
	{
		// delete all client memory
		while (clients.size())
		{
			WebClient* client = clients.back();
			clients.pop_back();
			delete client;
		}
		logger->Info(Languages::TextWebServerStopped);
	}

	void WebServer::Start()
	{
		StartTcpServer();
		logger->Info(Languages::TextWebServerStarted);
	}

	void WebServer::Stop()
	{
		AddUpdate(Languages::TextShutdownRailControl);
		TerminateTcpServer();
		// stopping all clients
		for (auto client : clients)
		{
			client->Stop();
		}
	}

	void WebServer::LogBrowserInfo(const std::string& webserveraddress, const unsigned short port)
	{
		static const string Http("\n   http://");
		static const string Port(to_string(port));

		string localhostInfo(Http);
		localhostInfo += "localhost:" + Port + "/";
		string ipv4Info;
		string ipv6Info;

		struct ifaddrs* ifAddrStruct = nullptr;
		struct ifaddrs* ifa = nullptr;
		getifaddrs(&ifAddrStruct);

		if (webserveraddress.compare("localhost") != 0)
		{
			for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
			{
				if (!ifa->ifa_addr)
				{
					continue;
				}
				if (ifa->ifa_addr->sa_family == AF_INET)
				{
					// is a valid IP4 Address
					void* tmpAddrPtr = &((struct sockaddr_in*) ifa->ifa_addr)->sin_addr;
					char addressBuffer[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
					string address(addressBuffer);
					if (address.compare("0.0.0.0") == 0)
					{
						continue;
					}
					if (address.substr(0, 8).compare("169.254.") == 0)
					{
						continue;
					}
					ipv4Info += Http + addressBuffer + ":" + Port + "/";
				}
				else if (ifa->ifa_addr->sa_family == AF_INET6)
				{
					// is a valid IP6 Address
					void* tmpAddrPtr = &((struct sockaddr_in6*) ifa->ifa_addr)->sin6_addr;
					char addressBuffer[INET6_ADDRSTRLEN];
					inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
					ipv6Info += Http + "[" + addressBuffer + "]:" + Port + "/";
				}
			}
		}

		if (ifAddrStruct != NULL)
		{
			freeifaddrs(ifAddrStruct);
		}

		logger->Info(Languages::TextBrowserInfo, localhostInfo, ipv4Info, ipv6Info);

	}

	void WebServer::Work(Network::TcpConnection* connection)
	{
		clients.push_back(new WebClient(++lastClientID, connection, *this, manager));

		// clean up unused clients
		for (auto iterator = clients.begin(); iterator != clients.end();)
		{
			WebClient* client = *iterator;
			if (client->IsTerminated())
			{
				iterator = clients.erase(iterator);
				delete client;
			}
			else
			{
				++iterator;
			}
		}
	}

	void WebServer::Booster(__attribute__((unused)) const ControlType controlType, const BoosterState status)
	{
		if (status)
		{
			AddUpdate("booster;on=true", Languages::TextTurningBoosterOn);
		}
		else
		{
			AddUpdate("booster;on=false", Languages::TextTurningBoosterOff);
		}
	}

	void WebServer::LocoBaseSpeed(__attribute__((unused)) const ControlType controlType,
		const LocoBase* loco,
		const Speed speed)
	{
		const LocoID locoId = loco->GetLocoIdWithPrefix();
		string command = "locospeed;loco=" + to_string(locoId) + ";speed=" + to_string(speed);
		AddUpdate(command, Languages::TextLocoSpeedIs, loco->GetName(), speed);
	}

	void WebServer::LocoBaseOrientation(__attribute__((unused)) const ControlType controlType,
		const LocoBase* loco,
		const Orientation orientation)
	{
		const LocoID locoId = loco->GetLocoIdWithPrefix();
		string command = "locoorientation;loco=" + to_string(locoId) + ";orientation=" + (orientation ? "true" : "false");
		AddUpdate(command, orientation ? Languages::TextLocoDirectionOfTravelIsRight : Languages::TextLocoDirectionOfTravelIsLeft, loco->GetName());
	}

	void WebServer::LocoBaseFunction(__attribute__((unused)) const ControlType controlType,
		const LocoBase* loco,
		const LocoFunctionNr function,
		const LocoFunctionState state)
	{
		const LocoID locoId = loco->GetLocoIdWithPrefix();
		string command = "locofunction;loco=" + to_string(locoId) + ";function=" + to_string(function) + ";on=" + (state ? "true" : "false");
		AddUpdate(command, state ? Languages::TextLocoFunctionIsOn : Languages::TextLocoFunctionIsOff, loco->GetName(), function);
	}

	void WebServer::AccessoryState(__attribute__((unused)) const ControlType controlType, const DataModel::Accessory* accessory)
	{
		stringstream command;
		const DataModel::AccessoryState state = accessory->GetAccessoryState();
		command << "accessory;accessory=" << accessory->GetID() << ";state=" << (state == DataModel::AccessoryStateOn ? "green" : "red");
		AddUpdate(command.str(), state ? Languages::TextAccessoryStateIsGreen : Languages::TextAccessoryStateIsRed, accessory->GetName());
	}

	void WebServer::AccessorySettings(const AccessoryID accessoryID,
		const std::string& name,
		__attribute__((unused)) const std::string& matchkey)
	{
		stringstream command;
		command << "accessorysettings;accessory=" << accessoryID;
		AddUpdate(command.str(), Languages::TextAccessoryUpdated, name);
	}

	void WebServer::AccessoryDelete(const AccessoryID accessoryID,
		const std::string& name,
		__attribute__((unused)) const std::string& matchkey)
	{
		stringstream command;
		command << "accessorydelete;accessory=" << accessoryID;
		AddUpdate(command.str(), Languages::TextAccessoryDeleted, name);
	}

	void WebServer::FeedbackState(const std::string& name, const FeedbackID feedbackID, const DataModel::Feedback::FeedbackState state)
	{
		stringstream command;
		command << "feedback;feedback=" << feedbackID << ";state=" << (state ? "on" : "off");
		AddUpdate(command.str(), state ? Languages::TextFeedbackStateIsOn : Languages::TextFeedbackStateIsOff, name);
	}

	void WebServer::FeedbackSettings(const FeedbackID feedbackID, const std::string& name)
	{
		stringstream command;
		command << "feedbacksettings;feedback=" << feedbackID;
		AddUpdate(command.str(), Languages::TextFeedbackUpdated, name);
	}

	void WebServer::FeedbackDelete(const FeedbackID feedbackID, const std::string& name)
	{
		stringstream command;
		command << "feedbackdelete;feedback=" << feedbackID;
		AddUpdate(command.str(), Languages::TextFeedbackDeleted, name);
	}

	void WebServer::RouteSettings(const RouteID routeID, const std::string& name)
	{
		stringstream command;
		command << "routesettings;route=" << routeID;
		AddUpdate(command.str(), Languages::TextRouteUpdated, name);
	}

	void WebServer::RouteDelete(const RouteID routeID, const std::string& name)
	{
		stringstream command;
		command << "routedelete;route=" << routeID;
		AddUpdate(command.str(), Languages::TextRouteDeleted, name);
	}

	void WebServer::SwitchState(__attribute__((unused)) const ControlType controlType, const DataModel::Switch* mySwitch)
	{
		stringstream command;
		const DataModel::AccessoryState state = mySwitch->GetAccessoryState();
		command << "switch;switch=" << mySwitch->GetID() << ";state=";
		Languages::TextSelector text;
		switch (state)
		{
			case DataModel::AccessoryState::SwitchStateTurnout:
				command << "turnout";
				text = Languages::TextSwitchStateIsTurnout;
				break;

			case DataModel::AccessoryState::SwitchStateThird:
				command << "third";
				text = Languages::TextSwitchStateIsThird;
				break;

			case DataModel::AccessoryState::SwitchStateStraight:
			default:
				command << "straight";
				text = Languages::TextSwitchStateIsStraight;
				break;
		}
		AddUpdate(command.str(), text, mySwitch->GetName());
	}

	void WebServer::SwitchSettings(const SwitchID switchID,
		const std::string& name,
		__attribute__((unused)) const std::string& matchKey)
	{
		stringstream command;
		command << "switchsettings;switch=" << switchID;
		AddUpdate(command.str(), Languages::TextSwitchUpdated, name);
	}

	void WebServer::SwitchDelete(const SwitchID switchID,
		const std::string& name,
		__attribute__((unused)) const std::string& matchkey)
	{
		stringstream command;
		command << "switchdelete;switch=" << switchID;
		AddUpdate(command.str(), Languages::TextSwitchDeleted, name);
	}

	void WebServer::TrackState(const DataModel::Track* track)
	{
		const LocoBase* locoBase = manager.GetLocoBase(track->GetMainLocoBaseDelayed());
		const bool reserved = locoBase != nullptr;
		const string& trackName = track->GetMainName();
		const string& locoName = reserved ? locoBase->GetName() : "";
		const bool occupied = track->GetMainStateDelayed() == DataModel::Feedback::FeedbackStateOccupied;
		const bool blocked = track->GetMainBlocked();
		const Orientation orientation = track->GetMainLocoOrientation();
		const string occupiedText = (occupied ? "true" : "false");
		const string blockedText = (blocked ? "true" : "false");
		const string reservedText = (reserved ? "true" : "false");
		const string orientationText = (orientation ? "true" : "false");
		string command = "trackstate;track="
			+ to_string(track->GetID())
			+ ";occupied=" + occupiedText
			+ ";reserved=" + reservedText
			+ ";blocked=" + blockedText
			+ ";orientation=" + orientationText
			+ ";loconame=" + locoName;

		if (track->GetMain())
		{
			AddUpdate(command);
		}
		else if (blocked)
		{
			if (reserved)
			{
				AddUpdate(command, Languages::TextTrackStatusIsBlockedAndReserved, trackName, locoName);
			}
			else if (occupied)
			{
				AddUpdate(command, Languages::TextTrackStatusIsBlockedAndOccupied, trackName);
			}
			else
			{
				AddUpdate(command, Languages::TextTrackStatusIsBlocked, trackName);
			}
		}
		else
		{
			if (reserved)
			{
				AddUpdate(command, Languages::TextTrackStatusIsReserved, trackName, locoName);;
			}
			else if (occupied)
			{
				AddUpdate(command, Languages::TextTrackStatusIsOccupied, trackName);
			}
			else
			{
				AddUpdate(command, Languages::TextTrackStatusIsFree, trackName);
			}
		}
	}

	void WebServer::TrackSettings(const TrackID trackID, const std::string& name)
	{
		stringstream command;
		command << "tracksettings;track=" << trackID;
		AddUpdate(command.str(), Languages::TextTrackUpdated, name);
	}

	void WebServer::TrackDelete(const TrackID trackID, const std::string& name)
	{
		stringstream command;
		command << "trackdelete;track=" << trackID;
		AddUpdate(command.str(), Languages::TextTrackDeleted, name);
	}

	void WebServer::SignalState(__attribute__((unused)) const ControlType controlType, const DataModel::Signal* signal)
	{
		const DataModel::AccessoryState state = signal->GetAccessoryState();
		string stateText;
		Languages::TextSelector text;
		switch(state)
		{
			case DataModel::SignalStateStop:
			default:
				stateText = "stop";
				text = Languages::TextSignalStateIsStop;
				break;

			case DataModel::SignalStateClear:
				stateText = "clear";
				text = Languages::TextSignalStateIsClear;
				break;

			case DataModel::SignalStateAspect2:
				stateText = "aspect2";
				text = Languages::TextSignalStateIsAspect2;
				break;

			case DataModel::SignalStateAspect3:
				stateText = "aspect3";
				text = Languages::TextSignalStateIsAspect3;
				break;

			case DataModel::SignalStateAspect4:
				stateText = "aspect4";
				text = Languages::TextSignalStateIsAspect4;
				break;

			case DataModel::SignalStateAspect5:
				stateText = "aspect5";
				text = Languages::TextSignalStateIsAspect5;
				break;

			case DataModel::SignalStateAspect6:
				stateText = "aspect6";
				text = Languages::TextSignalStateIsAspect6;
				break;

			case DataModel::SignalStateAspect7:
				stateText = "aspect7";
				text = Languages::TextSignalStateIsAspect7;
				break;

			case DataModel::SignalStateAspect8:
				stateText = "aspect8";
				text = Languages::TextSignalStateIsAspect8;
				break;

			case DataModel::SignalStateAspect9:
				stateText = "aspect9";
				text = Languages::TextSignalStateIsAspect9;
				break;

			case DataModel::SignalStateAspect10:
				stateText = "aspect10";
				text = Languages::TextSignalStateIsAspect10;
				break;

			case DataModel::SignalStateDark:
				stateText = "dark";
				text = Languages::TextSignalStateIsDark;
				break;

			case DataModel::SignalStateStopExpected:
				stateText = "stopexpected";
				text = Languages::TextSignalStateIsStopExpected;
				break;

			case DataModel::SignalStateClearExpected:
				stateText = "clearexpected";
				text = Languages::TextSignalStateIsClearExpected;
				break;

			case DataModel::SignalStateAspect2Expected:
				stateText = "aspect2expected";
				text = Languages::TextSignalStateIsAspect2Expected;
				break;

			case DataModel::SignalStateAspect3Expected:
				stateText = "aspect3expected";
				text = Languages::TextSignalStateIsAspect3Expected;
				break;

			case DataModel::SignalStateAspect4Expected:
				stateText = "aspect4expected";
				text = Languages::TextSignalStateIsAspect4Expected;
				break;

			case DataModel::SignalStateAspect5Expected:
				stateText = "aspect5expected";
				text = Languages::TextSignalStateIsAspect5Expected;
				break;

			case DataModel::SignalStateAspect6Expected:
				stateText = "aspect6expected";
				text = Languages::TextSignalStateIsAspect6Expected;
				break;

			case DataModel::SignalStateAspect7Expected:
				stateText = "aspect7expected";
				text = Languages::TextSignalStateIsAspect7Expected;
				break;

			case DataModel::SignalStateAspect8Expected:
				stateText = "aspect8expected";
				text = Languages::TextSignalStateIsAspect8Expected;
				break;

			case DataModel::SignalStateAspect9Expected:
				stateText = "aspect9expected";
				text = Languages::TextSignalStateIsAspect9Expected;
				break;

			case DataModel::SignalStateAspect10Expected:
				stateText = "aspect10expected";
				text = Languages::TextSignalStateIsAspect10Expected;
				break;
		}
		const string signalIdText(to_string(signal->GetID()));
		const string command = "signal;signal=" + signalIdText + ";state=" + stateText;
		AddUpdate(command, text, signal->GetName());
	}

	void WebServer::SignalSettings(const SignalID signalID,
		const std::string& name,
		__attribute__((unused)) const std::string& matchKey)
	{
		stringstream command;
		command << "signalsettings;signal=" << signalID;
		AddUpdate(command.str(), Languages::TextSignalUpdated, name);
	}

	void WebServer::SignalDelete(const SignalID signalID,
		const std::string& name,
		__attribute__((unused)) const std::string& matchkey)
	{
		stringstream command;
		command << "signaldelete;signal=" << signalID;
		AddUpdate(command.str(), Languages::TextSignalDeleted, name);
	}

	void WebServer::ClusterSettings(const ClusterID clusterID, const std::string& name)
	{
		stringstream command;
		command << "clustersettings;cluster=" << clusterID;
		AddUpdate(command.str(), Languages::TextClusterUpdated, name);
	}

	void WebServer::ClusterDelete(const ClusterID clusterID, const std::string& name)
	{
		stringstream command;
		command << "clusterdelete;cluster=" << clusterID;
		AddUpdate(command.str(), Languages::TextClusterDeleted, name);
	}

	void WebServer::TextSettings(const TextID textID, const std::string& name)
	{
		string command = "textsettings;text=" + to_string(textID);
		AddUpdate(command, Languages::TextTextUpdated, name);
	}

	void WebServer::TextDelete(const TextID textID, const std::string& name)
	{
		string command = "textdelete;text=" + to_string(textID);
		AddUpdate(command, Languages::TextTextDeleted, name);
	}

	void WebServer::CounterSettings(const CounterID counterID, const std::string& name)
	{
		const string command = "countersettings;counter=" + to_string(counterID);
		AddUpdate(command, Languages::TextCounterUpdated, name);
	}

	void WebServer::CounterDelete(const CounterID counterID, const std::string& name)
	{
		const string command = "counterdelete;counter=" + to_string(counterID);
		AddUpdate(command, Languages::TextCounterDeleted, name);
	}

	void WebServer::CounterState(const DataModel::Counter* const counter)
	{
		const string command ="counterstate;counter=" + to_string(counter->GetID()) + ";count=" + to_string(counter->GetCounter());
		AddUpdate(command, Languages::TextCounterUpdated, counter->GetName());
	}

	void WebServer::LocoBaseRelease(const DataModel::LocoBase* loco)
	{
		string command("locorelease;loco=");
		command += loco->GetObjectIdentifier();
		AddUpdate(command, Languages::TextLocoIsReleased, loco->GetName());
	}

	void WebServer::RouteRelease(const RouteID routeID)
	{
		stringstream command;
		command << "routeRelease;route=" << routeID;
		AddUpdate(command.str(), Languages::TextRouteIsReleased, manager.GetRouteName(routeID));
	}

	void WebServer::LocoBaseDestinationReached(const LocoBase* loco,
		const Route* route,
		const Track* track)
	{
		string command("locoDestinationReached;loco=");
		command += loco->GetObjectIdentifier();
		command += ";route=";
		command += to_string(route->GetID());
		command += ";track=";
		command += to_string(track->GetID());
		AddUpdate(command, Languages::TextLocoHasReachedDestination, loco->GetName(), track->GetName(), route->GetName());
	}

	void WebServer::LocoBaseStart(const DataModel::LocoBase* loco)
	{
		string command("locoStart;loco=");
		command += loco->GetObjectIdentifier();
		AddUpdate(command, Languages::TextLocoIsInAutoMode, loco->GetName());
	}

	void WebServer::LocoBaseStop(const DataModel::LocoBase* loco)
	{
		string command("locoStop;loco=");
		command += loco->GetObjectIdentifier();
		AddUpdate(command, Languages::TextLocoIsInManualMode, loco->GetName());
	}

	void WebServer::LocoSettings(const LocoID locoID,
		const std::string& name,
		__attribute__((unused)) const std::string& matchKey)
	{
		stringstream command;
		command << "locosettings;loco=" << locoID;
		AddUpdate(command.str(), Languages::TextLocoUpdated, name);
	}

	void WebServer::LocoDelete(const LocoID locoID,
		const std::string& name,
		__attribute__((unused)) const std::string& matchkey)
	{
		stringstream command;
		command << "locodelete;loco=" << locoID;
		AddUpdate(command.str(), Languages::TextLocoDeleted, name);
	}

	void WebServer::MultipleUnitSettings(const MultipleUnitID multipleUnitID,
		const std::string& name,
		__attribute__((unused)) const std::string& matchKey)
	{
		stringstream command;
		command << "multipleunitsettings;loco=" << multipleUnitID;
		AddUpdate(command.str(), Languages::TextLocoUpdated, name);
	}

	void WebServer::MultipleUnitDelete(const MultipleUnitID multipleUnitID,
		const std::string& name,
		__attribute__((unused)) const std::string& matchkey)
	{
		stringstream command;
		command << "multipleunitdelete;loco=" << multipleUnitID;
		AddUpdate(command.str(), Languages::TextLocoDeleted, name);
	}

	void WebServer::LayerSettings(const LayerID layerID, const std::string& name)
	{
		stringstream command;
		command << "layersettings;layer=" << layerID;
		AddUpdate(command.str(), Languages::TextLayerUpdated, name);
	}

	void WebServer::LayerDelete(const LayerID layerID, const std::string& name)
	{
		stringstream command;
		command << "layerdelete;layer=" << layerID;
		AddUpdate(command.str(), Languages::TextLayerDeleted, name);
	}

	void WebServer::ProgramValue(const CvNumber cv, const CvValue value)
	{
		stringstream command;
		command << "dcccvvalue;cv=" << static_cast<int>(cv) << ";value=" << static_cast<int>(value);
		AddUpdate(command.str(), Languages::TextProgramReadValue , static_cast<int>(cv), static_cast<int>(value));
	}

	void WebServer::AddUpdateInternal(const string& data)
	{
		std::lock_guard<std::mutex> lock(updateMutex);
		updates[updateID] = data;
		++updateID;
		updates.erase(updateID - MaxUpdates);
	}

	bool WebServer::NextUpdate(unsigned int& updateIDClient, string& s)
	{
		std::lock_guard<std::mutex> lock(updateMutex);

		if (updateIDClient + MaxUpdates <= updateID)
		{
			updateIDClient = updateID - MaxUpdates + 1;
		}

		if (updates.count(updateIDClient) == 1)
		{
			s = updates.at(updateIDClient);
			return true;
		}

		return false;
	}

}} // namespace Server::Web
