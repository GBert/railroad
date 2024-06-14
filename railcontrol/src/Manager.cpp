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

#include <future>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include "DataModel/LayoutItem.h"
#include "Languages.h"
#include "Hardware/HardwareHandler.h"
#include "Hardware/HardwareParams.h"
#include "Hardware/Protocols/MaerklinCANCommon.h"
#include "Manager.h"
#include "RailControl.h"
#include "Storage/TransactionGuard.h"
#include "Utils/Integer.h"
#include "Utils/Utils.h"
#include "Server/CS2/CS2Server.h"
#include "Server/Web/WebServer.h"
#include "Server/Z21/Z21Server.h"

using namespace DataModel;
using LayoutPosition = DataModel::LayoutItem::LayoutPosition;
using LayoutItemSize = DataModel::LayoutItem::LayoutItemSize;
using LayoutRotation = DataModel::LayoutItem::LayoutRotation;
using Visible = DataModel::LayoutItem::Visible;
using Hardware::HardwareHandler;
using Hardware::HardwareParams;
using std::map;
using std::to_string;
using std::string;
using std::stringstream;
using std::vector;
using Storage::StorageHandler;
using Storage::StorageParams;

Manager::Manager(Config& config)
:	logger(Logger::Logger::GetLogger(Languages::GetText(Languages::TextManager))),
 	boosterState(BoosterStateStop),
	storage(nullptr),
	defaultAccessoryDuration(DataModel::DefaultAccessoryPulseDuration),
	autoAddFeedback(false),
	stopOnFeedbackInFreeTrack(true),
	executeAccessory(true),
	selectRouteApproach(DataModel::SelectRouteRandom),
	nrOfTracksToReserve(DataModel::Loco::ReserveOne),
	run(false),
	debounceRun(false),
	initLocosDone(false),
	serverEnabled(false),
	unknownControl(Languages::GetText(Languages::TextControlDoesNotExist)),
	unknownLoco(Languages::GetText(Languages::TextLocoDoesNotExist)),
	unknownMultipleUnit(Languages::GetText(Languages::TextMultipleUnitDoesNotExist)),
	unknownAccessory(Languages::GetText(Languages::TextAccessoryDoesNotExist)),
	unknownFeedback(Languages::GetText(Languages::TextFeedbackDoesNotExist)),
	unknownTrack(Languages::GetText(Languages::TextTrackDoesNotExist)),
	unknownSwitch(Languages::GetText(Languages::TextSwitchDoesNotExist)),
	unknownRoute(Languages::GetText(Languages::TextRouteDoesNotExist)),
	unknownSignal(Languages::GetText(Languages::TextSignalDoesNotExist))
{
	StorageParams storageParams;
	storageParams.module = "Sqlite";
	storageParams.filename = config.getStringValue("dbfilename", "railcontrol.sqlite");
	storageParams.keepBackups = config.getIntValue("dbkeepbackups", 10);
	storage = new StorageHandler(this, &storageParams);
	if (storage == nullptr)
	{
		logger->Info(Languages::TextUnableToCreateStorageHandler);
		return;
	}

	Logger::Logger::SetLogLevel(static_cast<Logger::Logger::Level>(Utils::Integer::StringToInteger(storage->GetSetting("LogLevel"), Logger::Logger::LevelInfo)));
	Languages::SetDefaultLanguage(static_cast<Languages::Language>(Utils::Integer::StringToInteger(storage->GetSetting("Language"), Languages::EN)));
	defaultAccessoryDuration = Utils::Integer::StringToInteger(storage->GetSetting("DefaultAccessoryDuration"), 250);
	autoAddFeedback = Utils::Utils::StringToBool(storage->GetSetting("AutoAddFeedback"));
	stopOnFeedbackInFreeTrack = Utils::Utils::StringToBool(storage->GetSetting("StopOnFeedbackInFreeTrack"), true);
	executeAccessory = Utils::Utils::StringToBool(storage->GetSetting("ExecuteAccessory"), true);
	selectRouteApproach = static_cast<DataModel::SelectRouteApproach>(Utils::Integer::StringToInteger(storage->GetSetting("SelectRouteApproach")));
	nrOfTracksToReserve = static_cast<DataModel::Loco::NrOfTracksToReserve>(Utils::Integer::StringToInteger(storage->GetSetting("NrOfTracksToReserve"), 2));

	controls[ControlIdWebServer] = new Server::Web::WebServer(*this, config.getStringValue("webserveraddress", "any"), config.getIntValue("webserverport", 8082));

	if (config.getBoolValue("z21server", false))
	{
		controls[ControlIdZ21Server] = new Server::Z21::Z21Server(*this, Server::Z21::Z21Server::Z21Port);
		serverEnabled = true;
	}

	if (config.getBoolValue("cs2server", false))
	{
		controls[ControlIdCS2Server] = new Server::CS2::CS2Server(*this);
		serverEnabled = true;
	}

	storage->AllHardwareParams(hardwareParams);
	for (auto& hardwareParam : hardwareParams)
	{
		hardwareParam.second->SetManager(this);
		controls[hardwareParam.second->GetControlID()] = new HardwareHandler(hardwareParam.second);
		logger->Info(Languages::TextLoadedControl, hardwareParam.first, hardwareParam.second->GetName());
	}

	storage->AllLayers(layers);
	for (auto& layer : layers)
	{
		logger->Info(Languages::TextLoadedLayer, layer.second->GetID(), layer.second->GetName());
	}
	if (layers.count(LayerUndeletable) != 1)
	{
		string result;
		bool initLayer1 = LayerSave(0, Languages::GetText(Languages::TextLayer1), result);
		if (initLayer1 == false)
		{
			logger->Error(Languages::TextUnableToAddLayer1);
		}
	}

	storage->AllTexts(texts);
	for (auto& text : texts)
	{
		logger->Info(Languages::TextLoadedText, text.second->GetID(), text.second->GetName());
	}

	storage->AllAccessories(accessories);
	for (auto& accessory : accessories)
	{
		logger->Info(Languages::TextLoadedAccessory, accessory.second->GetID(), accessory.second->GetName());
	}

	storage->AllFeedbacks(feedbacks);
	for (auto& feedback : feedbacks)
	{
		logger->Info(Languages::TextLoadedFeedback, feedback.second->GetID(), feedback.second->GetName());
	}

	storage->AllSignals(signals);
	for (auto& signal : signals)
	{
		logger->Info(Languages::TextLoadedSignal, signal.second->GetID(), signal.second->GetName());
	}

	storage->AllTracks(tracks);
	for (auto& track : tracks)
	{
		logger->Info(Languages::TextLoadedTrack, track.second->GetID(), track.second->GetName());
	}

	storage->AllSwitches(switches);
	for (auto& mySwitch : switches)
	{
		logger->Info(Languages::TextLoadedSwitch, mySwitch.second->GetID(), mySwitch.second->GetName());
	}

	storage->AllClusters(clusters);
	for (auto& cluster : clusters)
	{
		logger->Info(Languages::TextLoadedCluster, cluster.second->GetID(), cluster.second->GetName());
	}

	storage->AllRoutes(routes);
	for (auto& route : routes)
	{
		logger->Info(Languages::TextLoadedRoute, route.second->GetID(), route.second->GetName());
	}

	storage->AllLocos(locos);
	for (auto& loco : locos)
	{
		logger->Info(Languages::TextLoadedLoco, loco.second->GetID(), loco.second->GetName());
	}

	storage->AllMultipleUnits(multipleUnits);
	for (auto& multipleUnit : multipleUnits)
	{
		logger->Info(Languages::TextLoadedMultipleUnit, multipleUnit.second->GetID(), multipleUnit.second->GetName());
	}

	debounceRun = true;
	debounceThread = std::thread(&Manager::DebounceWorker, this);

	{
		std::lock_guard<std::mutex> guard(controlMutex);
		for (auto& control : controls)
		{
			control.second->Start();
		}
	}

	run = true;
	InitLocos();
}

Manager::~Manager()
{
	while (!LocoBaseStopAll())
	{
		Utils::Utils::SleepForSeconds(1);
	}

	debounceRun = false;
	debounceThread.join();

	Booster(ControlTypeInternal, BoosterStateStop);

	run = false;
	{
		std::lock_guard<std::mutex> guard(controlMutex);
		for (auto& control : controls)
		{
			control.second->Stop();
		}
	}
	Utils::Utils::SleepForSeconds(1);
	{
		std::lock_guard<std::mutex> guard(controlMutex);
		for (auto& control : controls)
		{
			ControlID controlID = control.first;
			if (controlID < ControlIdFirstHardware)
			{
				delete control.second;
				continue;
			}
			if (hardwareParams.count(controlID) != 1)
			{
				continue;
			}
			HardwareParams* params = hardwareParams.at(controlID);
			if (params == nullptr)
			{
				continue;
			}
			logger->Info(Languages::TextUnloadingControl, controlID, params->GetName());
			delete control.second;
			hardwareParams.erase(controlID);
			if (storage != nullptr)
			{
				logger->Info(Languages::TextSaving, params->GetName());
				storage->Save(*params);
			}
			delete params;
		}
	}

	{
		Storage::TransactionGuard guard(storage);
		DeleteAllMapEntries(multipleUnits, multipleUnitMutex);
		DeleteAllMapEntries(locos, locoMutex);
		DeleteAllMapEntries(routes, routeMutex);
		DeleteAllMapEntries(clusters, clusterMutex);
		DeleteAllMapEntries(switches, switchMutex);
		DeleteAllMapEntries(tracks, trackMutex);
		DeleteAllMapEntries(signals, signalMutex);
		DeleteAllMapEntries(feedbacks, feedbackMutex);
		DeleteAllMapEntries(accessories, accessoryMutex);
		DeleteAllMapEntries(texts, textMutex);
		DeleteAllMapEntries(layers, layerMutex);
	}

	if (storage == nullptr)
	{
		return;
	}

	delete storage;
	storage = nullptr;
}

/***************************
* Booster                  *
***************************/

void Manager::Booster(const ControlType controlType, const BoosterState state)
{
	if (!run)
	{
		return;
	}
	boosterState = state;
	{
		std::lock_guard<std::mutex> guard(controlMutex);
		for (auto& control : controls)
		{
			control.second->Booster(controlType, state);
		}
	}

	if (boosterState != BoosterStateGo || initLocosDone == true)
	{
		return;
	}

	__attribute__((unused)) auto r = std::async(std::launch::async, InitLocosStatic, this);
	initLocosDone = true;
}

void Manager::InitLocos()
{
	Utils::Utils::SleepForSeconds(1);
	map<string,LocoID> locoIds = LocoIdsByName();
	for (auto locoId : locoIds)
	{
		if (!run)
		{
			return;
		}
		Loco* loco = GetLoco(locoId.second);
		if (!loco)
		{
			continue;
		}
		{
			std::vector<DataModel::LocoFunctionEntry> functions = loco->GetFunctionStates();
			std::lock_guard<std::mutex> guard(controlMutex);
			for (auto& control : controls)
			{
				control.second->LocoSpeedOrientationFunctions(loco, loco->GetSpeed(), loco->GetOrientation(), functions);
			}
		}
		Utils::Utils::SleepForMilliseconds(10);
	}
}

/***************************
* Control                  *
***************************/

bool Manager::ControlSave(ControlID controlID,
	const HardwareType& hardwareType,
	const std::string& name,
	const std::string& arg1,
	const std::string& arg2,
	const std::string& arg3,
	const std::string& arg4,
	const std::string& arg5,
	string& result)
{
	if (controlID != ControlIdNone && controlID < ControlIdFirstHardware)
	{
		result = Languages::GetText(Languages::TextInvalidControlID);
		return false;
	}

	HardwareParams* params = GetHardware(controlID);
	bool newControl = false;
	if (params == nullptr)
	{
		params = CreateAndAddControl();
		newControl = true;
	}

	if (params == nullptr)
	{
		result = Languages::GetText(Languages::TextUnableToAddControl);
		return false;
	}

	// if we have a new object we have to update controlID
	controlID = params->GetControlID();

	params->SetName(CheckObjectName(hardwareParams, hardwareMutex, controlID, name.size() == 0 ? "C" : name));
	params->SetHardwareType(hardwareType);
	params->SetArg1(arg1);
	params->SetArg2(arg2);
	params->SetArg3(arg3);
	params->SetArg4(arg4);
	params->SetArg5(arg5);

	if (storage != nullptr)
	{
		storage->Save(*params);
	}

	if (newControl == true)
	{
		std::lock_guard<std::mutex> Guard(controlMutex);
		ControlInterface* control = new HardwareHandler(params);
		if (control == nullptr)
		{
			return false;
		}
		controls[controlID] = control;
		return true;
	}

	ControlInterface* control = GetControl(controlID);
	if (control == nullptr)
	{
		return false;
	}

	control->ReInit(params);
	return true;
}

bool Manager::ControlDelete(ControlID controlID)
{
	{
		std::lock_guard<std::mutex> guard(hardwareMutex);
		if (controlID < ControlIdFirstHardware || hardwareParams.count(controlID) != 1)
		{
			return false;
		}

		HardwareParams* params = hardwareParams.at(controlID);
		if (params == nullptr)
		{
			return false;
		}
		hardwareParams.erase(controlID);
		delete params;
	}
	{
		std::lock_guard<std::mutex> guard(controlMutex);
		if (controls.count(controlID) != 1)
		{
			return false;
		}
		ControlInterface* control = controls.at(controlID);
		if (control == nullptr)
		{
			return false;
		}
		controls.erase(controlID);
		delete control;
	}

	if (storage)
	{
		storage->DeleteHardwareParams(controlID);
	}
	return true;
}

HardwareParams* Manager::GetHardware(const ControlID controlID)
{
	std::lock_guard<std::mutex> guard(hardwareMutex);
	if (hardwareParams.count(controlID) != 1)
	{
		return nullptr;
	}
	return hardwareParams.at(controlID);
}

unsigned int Manager::ControlsOfHardwareType(const HardwareType hardwareType)
{
	std::lock_guard<std::mutex> guard(hardwareMutex);
	unsigned int counter = 0;
	for (auto& hardwareParam : hardwareParams)
	{
		if (hardwareParam.second->GetHardwareType() == hardwareType)
		{
			++counter;
		}
	}
	return counter;
}

bool Manager::ControlIsOfHardwareType(const ControlID controlID, const HardwareType hardwareType)
{
	std::lock_guard<std::mutex> guard(hardwareMutex);
	for (auto& hardwareParam : hardwareParams)
	{
		if (hardwareParam.second->GetControlID() != controlID)
		{
			continue;
		}
		return hardwareParam.second->GetHardwareType() == hardwareType;
	}
	return false;
}

ControlInterface* Manager::GetControl(const ControlID controlID) const
{
	std::lock_guard<std::mutex> guard(controlMutex);
	if (controls.count(controlID) != 1)
	{
		return nullptr;
	}
	return controls.at(controlID);
}

const std::string Manager::GetControlName(const ControlID controlID)
{
	std::lock_guard<std::mutex> guard(controlMutex);
	if (controls.count(controlID) != 1)
	{
		return unknownControl;
	}
	ControlInterface* control = controls.at(controlID);
	return control->GetName();
}

const std::map<ControlID,std::string> Manager::ControlListNames(const Hardware::Capabilities capability) const
{
	std::map<ControlID,std::string> ret;
	std::lock_guard<std::mutex> guard(hardwareMutex);
	for (auto& hardware : hardwareParams)
	{
		std::lock_guard<std::mutex> guard2(controlMutex);
		if (controls.count(hardware.second->GetControlID()) != 1)
		{
			continue;
		}
		ControlInterface* c = controls.at(hardware.second->GetControlID());
		if (c->CanHandle(capability) == false)
		{
			continue;
		}
		ret[hardware.first] = hardware.second->GetName();
	}
	return ret;
}

const map<string,Hardware::HardwareParams*> Manager::ControlListByName() const
{
	map<string,Hardware::HardwareParams*> out;
	std::lock_guard<std::mutex> guard(hardwareMutex);
	for (auto& hardware : hardwareParams)
	{
		out[hardware.second->GetName()] = hardware.second;
	}
	return out;
}

const std::map<std::string, Protocol> Manager::ProtocolsOfControl(const AddressType type, const ControlID controlID) const
{
	std::map<std::string,Protocol> ret;
	{
		const ControlInterface* control = GetControl(controlID);
		if (control == nullptr || control->GetControlType() != ControlTypeHardware)
		{
			ret[ProtocolSymbols[ProtocolNone]] = ProtocolNone;
			return ret;
		}

		const HardwareHandler* hardware = static_cast<const HardwareHandler*>(control);
		if (hardware->GetControlID() != controlID)
		{
			ret[ProtocolSymbols[ProtocolNone]] = ProtocolNone;
			return ret;
		}

		std::vector<Protocol> protocols;
		if (type == AddressTypeLoco)
		{
			hardware->LocoProtocols(protocols);
		}
		else
		{
			hardware->AccessoryProtocols(protocols);
		}
		for (auto protocol : protocols)
		{
			ret[ProtocolSymbols[protocol]] = protocol;
		}
	}
	return ret;
}

/***************************
* Loco                     *
***************************/

string Manager::GetLocoList() const
{
	string out;
	std::lock_guard<std::mutex> guard(locoMutex);
	for (auto& loco : locos)
	{
		Loco* l = loco.second;
		out += to_string(l->GetID()) + ";";
		out += to_string(l->GetSpeed()) + ";";
		out += to_string(l->GetOrientation()) + ";";
		out += to_string(l->GetTrackId()) + ";";
		out += l->GetName() + "\n";
	}
	return out;
}

Loco* Manager::GetLoco(const LocoID locoID) const
{
	std::lock_guard<std::mutex> guard(locoMutex);
	if (locos.count(locoID) != 1)
	{
		return nullptr;
	}
	return locos.at(locoID);
}

LocoConfig Manager::GetLocoOfConfigByMatchKey(const ControlID controlId, const string& matchKey) const
{
	ControlInterface* control = GetControl(controlId);
	if (control == nullptr)
	{
		return LocoConfig(LocoTypeLoco);
	}
	return control->GetLocoByMatchKey(matchKey);
}

Loco* Manager::GetLocoByMatchKey(const ControlID controlId, const string& matchKey) const
{
	std::lock_guard<std::mutex> guard(locoMutex);
	for (auto& loco : locos)
	{
		Loco* locoConfig = loco.second;
		if (locoConfig->GetControlID() == controlId && locoConfig->GetMatchKey().compare(matchKey) == 0)
		{
			return loco.second;
		}
	}
	return nullptr;
}

void Manager::LocoRemoveMatchKey(const LocoID locoId)
{
	Loco* loco = GetLoco(locoId);
	if (loco == nullptr)
	{
		return;
	}
	loco->ClearMatchKey();
}

void Manager::LocoReplaceMatchKey(const LocoID locoId, const std::string& newMatchKey)
{
	Loco* loco = GetLoco(locoId);
	if (loco == nullptr)
	{
		return;
	}
	loco->SetMatchKey(newMatchKey);
}

const map<string,LocoConfig> Manager::GetUnmatchedLocosOfControl(const ControlID controlId,
	const std::string& matchKey) const
{
	ControlInterface* control = GetControl(controlId);
	map<string,LocoConfig> out;
	if (control == nullptr || !control->CanHandle(Hardware::CapabilityLocoDatabase))
	{
		return out;
	}
	out = control->GetUnmatchedLocos(matchKey);
	out[""].SetName("");
	return out;
}

Loco* Manager::GetLoco(const ControlID controlID, const Protocol protocol, const Address address) const
{
	std::lock_guard<std::mutex> guard(locoMutex);
	for (auto& loco : locos)
	{
		if (loco.second->GetControlID() == controlID
			&& loco.second->GetProtocol() == protocol
			&& loco.second->GetAddress() == address)
		{
			return loco.second;
		}
	}
	return nullptr;
}

const map<string,LocoID> Manager::LocoBaseListFree() const
{
	map<string,LocoID> out;
	{
		std::lock_guard<std::mutex> guard(locoMutex);
		for (auto& loco : locos)
		{
			if (loco.second->IsInUse() == false)
			{
				out[loco.second->GetName()] = loco.second->GetID();
			}
		}
	}
	{
		std::lock_guard<std::mutex> guard(multipleUnitMutex);
		for (auto& multipleUnit : multipleUnits)
		{
			if (multipleUnit.second->IsInUse() == false)
			{
				out[multipleUnit.second->GetName()] = multipleUnit.second->GetID() + MultipleUnitIdPrefix;
			}
		}
	}
	return out;
}

const map<string,DataModel::LocoConfig> Manager::LocoConfigByName() const
{
	map<string,DataModel::LocoConfig> out;
	{
		std::lock_guard<std::mutex> guard(locoMutex);
		for (auto& loco : locos)
		{
			out[loco.second->GetName()] = *(loco.second);
		}
	}
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->AddUnmatchedLocos(out);
	}

	return out;
}

const map<string,LocoID> Manager::LocoIdsByName() const
{
	map<string,LocoID> out;
	{
		std::lock_guard<std::mutex> guard(locoMutex);
		for (auto& loco : locos)
		{
			Loco* locoEntry = loco.second;
			out[locoEntry->GetName()] = locoEntry->GetID();
		}
	}
	return out;
}

void Manager::LocoBaseSave(const DataModel::LocoBase* locoBase) const
{
	const DataModel::Loco* loco = dynamic_cast<const DataModel::Loco*>(locoBase);
	if (loco)
	{
		LocoSave(loco);
	}
	const DataModel::MultipleUnit* multipleUnit = dynamic_cast<const DataModel::MultipleUnit*>(locoBase);
	if (multipleUnit)
	{
		MultipleUnitSave(multipleUnit);
	}
}

bool Manager::LocoSave(LocoID locoID,
	const string& name,
	const ControlID controlID,
	const std::string& matchKey,
	const Protocol protocol,
	const Address address,
	const Address serverAddress,
	const Length length,
	const bool pushpull,
	const Speed maxSpeed,
	const Speed travelSpeed,
	const Speed reducedSpeed,
	const Speed creepingSpeed,
	const Propulsion propulsion,
	const TrainType type,
	const std::vector<DataModel::LocoFunctionEntry>& locoFunctions,
	string& result)
{
	if (!CheckControlLocoProtocolAddress(controlID, protocol, address, result))
	{
		return false;
	}

	Loco* loco = GetLoco(locoID);
	if (loco == nullptr)
	{
		loco = CreateAndAddObject(locos, locoMutex);
	}

	if (loco == nullptr)
	{
		result = Languages::GetText(Languages::TextUnableToAddLoco);
		return false;
	}

	// if we have a new object we have to update locoID
	locoID = loco->GetID();

	loco->SetName(CheckObjectName(multipleUnits, multipleUnitMutex, MultipleUnitNone, CheckObjectName(locos, locoMutex, locoID, name.size() == 0 ? "L" : name)));
	loco->SetControlID(controlID);
	loco->SetMatchKey(matchKey);
	loco->SetProtocol(protocol);
	loco->SetAddress(address);
	loco->SetServerAddress(serverAddress);
	loco->SetLength(length);
	loco->SetPushpull(pushpull);
	loco->SetMaxSpeed(maxSpeed);
	loco->SetTravelSpeed(travelSpeed);
	loco->SetReducedSpeed(reducedSpeed);
	loco->SetCreepingSpeed(creepingSpeed);
	loco->SetPropulsion(propulsion);
	loco->SetTrainType(type);
	loco->ConfigureFunctions(locoFunctions);

	// save in db
	LocoSave(loco);

	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->LocoSettings(locoID, name, matchKey);
	}
	return true;
}

bool Manager::LocoDelete(const LocoID locoID, string& result)
{
	Loco* loco = nullptr;
	{
		std::lock_guard<std::mutex> guard(locoMutex);
		if (locoID == LocoNone || locos.count(locoID) != 1)
		{
			result = Languages::GetText(Languages::TextLocoDoesNotExist);
			return false;
		}

		loco = locos.at(locoID);
		if (loco == nullptr)
		{
			result = Languages::GetText(Languages::TextLocoDoesNotExist);
			return false;
		}

		if (loco->IsInUse())
		{
			result = Logger::Logger::Format(Languages::GetText(Languages::TextLocoIsInUse), loco->GetName());
			return false;
		}

		locos.erase(locoID);
	}

	if (storage)
	{
		storage->DeleteLoco(locoID);
	}
	const string& name = loco->GetName();
	const string& matchKey = loco->GetMatchKey();
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->LocoDelete(locoID, name, matchKey);
	}
	delete loco;
	return true;
}

bool Manager::LocoProtocolAddress(const LocoID locoID, ControlID& controlID, Protocol& protocol, Address& address) const
{
	std::lock_guard<std::mutex> guard(locoMutex);
	if (locos.count(locoID) != 1)
	{
		controlID = 0;
		protocol = ProtocolNone;
		address = 0;
		return false;
	}
	Loco* loco = locos.at(locoID);
	if (loco == nullptr)
	{
		return false;
	}
	controlID = loco->GetControlID();
	protocol = loco->GetProtocol();
	address = loco->GetAddress();
	return true;
}

bool Manager::LocoBaseSpeed(const ControlType controlType,
	LocoBase* loco,
	const Speed speed)
{
	if (loco == nullptr)
	{
		return false;
	}
	Speed newSpeed = speed;
	if (speed > MaxSpeed)
	{
		newSpeed = MaxSpeed;
	}
	const string& locoName = loco->GetName();
	logger->Info(Languages::TextLocoSpeedIs, locoName, newSpeed);
	const Speed oldSpeed = loco->GetSpeed();
	if (oldSpeed == newSpeed)
	{
		return true;
	}
	loco->SetSpeed(newSpeed);
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->LocoBaseSpeed(controlType, loco, newSpeed);
	}
	return true;
}

Speed Manager::LocoSpeed(const LocoID locoID) const
{
	Loco* loco = GetLoco(locoID);
	if (loco == nullptr)
	{
		return MinSpeed;
	}
	return loco->GetSpeed();
}

void Manager::LocoBaseOrientation(const ControlType controlType,
	LocoBase* loco,
	Orientation orientation)
{
	if (loco == nullptr)
	{
		return;
	}

	if (orientation == OrientationChange)
	{
		const Orientation oldOrientation = loco->GetOrientation();
		orientation = (oldOrientation == OrientationLeft ? OrientationRight : OrientationLeft);
	}

	logger->Info(orientation ? Languages::TextLocoDirectionOfTravelIsRight : Languages::TextLocoDirectionOfTravelIsLeft, loco->GetName());
	const Orientation oldOrientation = loco->GetOrientation();
	if (oldOrientation == orientation)
	{
		return;
	}
	loco->SetOrientation(orientation);
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->LocoBaseOrientation(controlType, loco, orientation);
	}
}

void Manager::LocoBaseFunctionState(const ControlType controlType,
	const ObjectIdentifier& locoBaseIdentifier,
	const DataModel::LocoFunctionNr function,
	const DataModel::LocoFunctionState on)
{
	LocoBase* loco = GetLocoBase(locoBaseIdentifier);
	if (loco == nullptr)
	{
		return;
	}

	DataModel::LocoFunctionNr functionInternal = function;
	if (function >= NumberOfLocoFunctions)
	{
		functionInternal = loco->GetFunctionNumberFromFunctionIcon(static_cast<DataModel::LocoFunctionIcon>(function - 256));
	}
	LocoBaseFunctionState(controlType, loco, functionInternal, on);
}

void Manager::LocoBaseFunctionState(const ControlType controlType,
	LocoBase* loco,
	const DataModel::LocoFunctionNr function,
	const DataModel::LocoFunctionState on)
{
	if (loco == nullptr)
	{
		return;
	}

	logger->Info(on ? Languages::TextLocoFunctionIsOn : Languages::TextLocoFunctionIsOff, loco->GetName(), function);
	const DataModel::LocoFunctionState oldOn = loco->GetFunctionState(function);
	if (oldOn == on)
	{
		return;
	}
	loco->SetFunctionState(function, on);
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->LocoBaseFunction(controlType, loco, function, on);
	}
}

/***************************
* MultipleUnit             *
***************************/

MultipleUnit* Manager::GetMultipleUnit(const MultipleUnitID multipleUnitId) const
{
	std::lock_guard<std::mutex> guard(multipleUnitMutex);
	if (multipleUnits.count(multipleUnitId) != 1)
	{
		return nullptr;
	}
	return multipleUnits.at(multipleUnitId);
}

LocoConfig Manager::GetMultipleUnitOfConfigByMatchKey(const ControlID controlId, const string& matchKey) const
{
	ControlInterface* control = GetControl(controlId);
	if (control == nullptr)
	{
		return LocoConfig(LocoTypeMultipleUnit);
	}
	return control->GetMultipleUnitByMatchKey(matchKey);
}

const map<string,DataModel::LocoConfig> Manager::MultipleUnitConfigByName() const
{
	map<string,DataModel::LocoConfig> out;
	{
		std::lock_guard<std::mutex> guard(multipleUnitMutex);
		for (auto& multipleUnit : multipleUnits)
		{
			out[multipleUnit.second->GetName()] = *(multipleUnit.second);
		}
	}
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->AddUnmatchedMultipleUnits(out);
	}

	return out;
}

bool Manager::MultipleUnitSave(MultipleUnitID multipleUnitID,
	const string& name,
	const ControlID controlID,
	const std::string& matchKey,
	const Address address,
	const Address serverAddress,
	const Length length,
	const bool pushpull,
	const Speed maxSpeed,
	const Speed travelSpeed,
	const Speed reducedSpeed,
	const Speed creepingSpeed,
	const TrainType type,
	const std::vector<DataModel::LocoFunctionEntry>& locoFunctions,
	const std::vector<DataModel::Relation*>& slaves,
	string& result)
{
	if (!CheckControlMultipleUnitProtocolAddress(controlID, address, result))
	{
		return false;
	}

	MultipleUnit* multipleUnit = GetMultipleUnit(multipleUnitID);
	if (multipleUnit == nullptr)
	{
		multipleUnit = CreateAndAddObject(multipleUnits, multipleUnitMutex);
	}

	if (multipleUnit == nullptr)
	{
		result = Languages::GetText(Languages::TextUnableToAddMultipleUnit);
		return false;
	}

	// if we have a new object we have to update multipleUnitID
	if (multipleUnitID == 0)
	{
		multipleUnitID = multipleUnit->GetID();
		for (auto slave : slaves)
		{
			slave->ObjectID1(multipleUnitID);
		}
	}

	// FIXME: replace "M" with language dependent word
	multipleUnit->SetName(CheckObjectName(locos, locoMutex, LocoNone, CheckObjectName(multipleUnits, multipleUnitMutex, multipleUnitID, name.size() == 0 ? "M" : name)));
	multipleUnit->SetControlID(controlID);
	multipleUnit->SetMatchKey(matchKey);
	multipleUnit->SetProtocol(ProtocolNone);
	multipleUnit->SetAddress(address);
	multipleUnit->SetServerAddress(serverAddress);
	multipleUnit->SetLength(length);
	multipleUnit->SetPushpull(pushpull);
	multipleUnit->SetMaxSpeed(maxSpeed);
	multipleUnit->SetTravelSpeed(travelSpeed);
	multipleUnit->SetReducedSpeed(reducedSpeed);
	multipleUnit->SetCreepingSpeed(creepingSpeed);
	multipleUnit->SetTrainType(type);
	multipleUnit->ConfigureFunctions(locoFunctions);
	multipleUnit->AssignSlaves(slaves);

	// save in db
	MultipleUnitSave(multipleUnit);

	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->MultipleUnitSettings(multipleUnitID, name, matchKey);
	}
	return true;
}

bool Manager::MultipleUnitDelete(const MultipleUnitID multipleUnitID, string& result)
{
	MultipleUnit* multipleUnit = nullptr;
	{
		std::lock_guard<std::mutex> guard(multipleUnitMutex);
		if (multipleUnitID == MultipleUnitNone || multipleUnits.count(multipleUnitID) != 1)
		{
			result = Languages::GetText(Languages::TextMultipleUnitDoesNotExist);
			return false;
		}

		multipleUnit = multipleUnits.at(multipleUnitID);
		if (multipleUnit == nullptr)
		{
			result = Languages::GetText(Languages::TextMultipleUnitDoesNotExist);
			return false;
		}

		if (multipleUnit->IsInUse())
		{
			result = Logger::Logger::Format(Languages::GetText(Languages::TextMultipleUnitIsInUse), multipleUnit->GetName());
			return false;
		}

		multipleUnits.erase(multipleUnitID);
	}

	if (storage)
	{
		storage->DeleteMultipleUnit(multipleUnitID);
	}
	const string& name = multipleUnit->GetName();
	const string& matchKey = multipleUnit->GetMatchKey();
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->MultipleUnitDelete(multipleUnitID, name, matchKey);
	}
	delete multipleUnit;
	return true;
}

const map<string,LocoID> Manager::LocoBaseIdsByName() const
{
	map<string,LocoID> out = LocoIdsByName();
	{
		std::lock_guard<std::mutex> guard(multipleUnitMutex);
		for (auto& multipleUnit : multipleUnits)
		{
			MultipleUnit* multipleUnitEntry = multipleUnit.second;
			out[multipleUnitEntry->GetName()] = multipleUnitEntry->GetID() + MultipleUnitIdPrefix;
		}
	}
	return out;
}

const std::string& Manager::GetLocoBaseName(const ObjectIdentifier& locoBaseIdentifier) const
{
	const LocoBase* locoBase = GetLocoBase(locoBaseIdentifier);
	if (!locoBase)
	{
		return unknownLoco;
	}
	return locoBase->GetName();
}

/***************************
* Accessory Base           *
***************************/

void Manager::AccessoryBaseState(const ControlType controlType,
	const ControlID controlID,
	const Protocol protocol,
	const Address address,
	const DataModel::AccessoryState state)
{
	Accessory* accessory = GetAccessory(controlID, protocol, address);
	if (accessory != nullptr)
	{
		AccessoryState(controlType, accessory, accessory->CalculateInvertedAccessoryState(state), true);
		return;
	}

	Switch* mySwitch = GetSwitch(controlID, protocol, address);
	if (mySwitch != nullptr)
	{
		SwitchState(controlType, mySwitch, mySwitch->CalculateInvertedSwitchState(address, state), true);
		return;
	}

	Signal* signal = GetSignal(controlID, protocol, address);
	if (signal != nullptr)
	{
		SignalState(controlType, signal, signal->CalculateMappedSignalState(address, state), true);
		return;
	}

	logger->Warning(Languages::TextAccessoryControlProtocolAddressDoesNotExist, controlID, Utils::Utils::ProtocolToString(protocol), address);
}

void Manager::AccessoryBaseState(const ControlType controlType,
	const ObjectIdentifier& identifier,
	const DataModel::AccessoryState state)
{
	switch (identifier.GetObjectType())
	{
		case ObjectTypeAccessory:
			AccessoryState(controlType, identifier.GetObjectID(), state);
			return;

		case ObjectTypeSwitch:
			SwitchState(controlType, identifier.GetObjectID(), state);
			return;

		case ObjectTypeSignal:
			SignalState(controlType, identifier.GetObjectID(), state);
			return;

		default:
			return;
	}
}

/***************************
* Accessory                *
***************************/

bool Manager::AccessoryState(const ControlType controlType, const AccessoryID accessoryID, const DataModel::AccessoryState state, const bool force)
{
	if (boosterState == BoosterStateStop)
	{
		return false;
	}
	Accessory* accessory = GetAccessory(accessoryID);
	AccessoryState(controlType, accessory, state, force);
	return true;
}

void Manager::AccessoryState(const ControlType controlType, Accessory* accessory, const DataModel::AccessoryState state, const bool force)
{
	if (accessory == nullptr)
	{
		return;
	}

	if (force == false && accessory->IsInUse())
	{
		logger->Warning(Languages::TextAccessoryIsLocked, accessory->GetName());
		return;
	}

	if (!GetExecuteAccessory())
	{
		const DataModel::AccessoryState oldState = accessory->GetAccessoryState();
		if (oldState == state)
		{
			return;
		}
	}
	accessory->SetAccessoryState(state);

	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->AccessoryState(controlType, accessory);
	}
}

const map<string,AccessoryConfig> Manager::GetUnmatchedAccessoriesOfControl(const ControlID controlId,
	const std::string& matchKey) const
{
	ControlInterface* control = GetControl(controlId);
	map<string,AccessoryConfig> out;
	if (control == nullptr || !control->CanHandle(Hardware::CapabilityAccessoryDatabase))
	{
		return out;
	}
	out = control->GetUnmatchedAccessories(matchKey);
	out[""].SetName("");
	return out;
}

Accessory* Manager::GetAccessory(const AccessoryID accessoryID) const
{
	std::lock_guard<std::mutex> guard(accessoryMutex);
	if (accessories.count(accessoryID) != 1)
	{
		return nullptr;
	}
	return accessories.at(accessoryID);
}

Accessory* Manager::GetAccessory(const ControlID controlID, const Protocol protocol, const Address address) const
{
	std::lock_guard<std::mutex> guard(accessoryMutex);
	for (auto& accessory : accessories)
	{
		if (accessory.second->GetControlID() == controlID
			&& accessory.second->GetProtocol() == protocol
			&& accessory.second->GetAddress() == address)
		{
			return accessory.second;
		}
	}
	return nullptr;
}

const std::string& Manager::GetAccessoryName(const AccessoryID accessoryID) const
{
	std::lock_guard<std::mutex> guard(accessoryMutex);
	if (accessories.count(accessoryID) != 1)
	{
		return unknownAccessory;
	}
	return accessories.at(accessoryID)->GetName();
}

bool Manager::AccessorySave(AccessoryID accessoryID,
	const string& name,
	const LayoutPosition posX,
	const LayoutPosition posY,
	const LayoutPosition posZ,
	const LayoutRotation rotation,
	const ControlID controlID,
	const std::string& matchKey,
	const Protocol protocol,
	const Address address,
	const Address serverAddress,
	const DataModel::AccessoryType type,
	const DataModel::AccessoryPulseDuration duration,
	const bool inverted,
	string& result)
{
	if (!CheckControlAccessoryProtocolAddress(controlID, protocol, address, result))
	{
		return false;
	}

	Accessory* accessory = GetAccessory(accessoryID);
	if (!CheckLayoutItemPosition(accessory, posX, posY, posZ, result))
	{
		return false;
	}

	if (accessory == nullptr)
	{
		accessory = CreateAndAddObject(accessories, accessoryMutex);
	}

	if (accessory == nullptr)
	{
		result = Languages::GetText(Languages::TextUnableToAddAccessory);
		return false;
	}

	// if we have a new object we have to update accessoryID
	accessoryID = accessory->GetID();

	// update existing accessory
	accessory->SetName(CheckObjectName(accessories, accessoryMutex, accessoryID, name.size() == 0 ? "A" : name));
	accessory->SetPosX(posX);
	accessory->SetPosY(posY);
	accessory->SetPosZ(posZ);
	accessory->SetRotation(rotation);
	accessory->SetControlID(controlID);
	accessory->SetMatchKey(matchKey);
	accessory->SetProtocol(protocol);
	accessory->SetAddress(address);
	accessory->SetServerAddress(serverAddress);
	accessory->SetAccessoryType(type);
	accessory->SetAccessoryPulseDuration(duration);
	accessory->SetInverted(inverted);

	AccessorySaveAndPublishSettings(accessory);
	return true;
}

bool Manager::AccessoryPosition(const AccessoryID accessoryID,
	const LayoutPosition posX,
	const LayoutPosition posY,
	string& result)
{
	Accessory* accessory = GetAccessory(accessoryID);
	if (accessory == nullptr)
	{
		result = Languages::GetText(Languages::TextAccessoryDoesNotExist);
		return false;
	}

	if (!CheckLayoutItemPosition(accessory, posX, posY, accessory->GetPosZ(), result))
	{
		return false;
	}

	accessory->SetPosX(posX);
	accessory->SetPosY(posY);

	AccessorySaveAndPublishSettings(accessory);
	return true;
}

bool Manager::AccessoryRotate(const AccessoryID accessoryID,
	string& result)
{
	Accessory* accessory = GetAccessory(accessoryID);
	if (accessory == nullptr)
	{
		result = Languages::GetText(Languages::TextAccessoryDoesNotExist);
		return false;
	}

	LayoutRotation newRotation = accessory->GetRotation();
	++newRotation;
	if (!CheckLayoutItemPosition(accessory, accessory->GetPosX(), accessory->GetPosY(), accessory->GetPosZ(), LayoutItem::Width1, LayoutItem::Height1, newRotation, result))
	{
		return false;
	}

	accessory->SetRotation(newRotation);

	AccessorySaveAndPublishSettings(accessory);
	return true;
}

void Manager::AccessorySaveAndPublishSettings(const Accessory* const accessory)
{
	// save in db
	if (storage)
	{
		storage->Save(*accessory);
	}

	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->AccessorySettings(accessory->GetID(), accessory->GetName(), accessory->GetMatchKey());
	}
}

const map<string,DataModel::AccessoryConfig> Manager::AccessoryConfigByName() const
{
	map<string,DataModel::AccessoryConfig> out;
	{
		std::lock_guard<std::mutex> guard(accessoryMutex);
		for (auto& accessory : accessories)
		{
			out[accessory.second->GetName()] = *(accessory.second);
		}
	}

	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->AddUnmatchedAccessories(out);
	}
	return out;
}

bool Manager::AccessoryDelete(const AccessoryID accessoryID,
	string& result)
{
	Accessory* accessory = nullptr;
	{
		std::lock_guard<std::mutex> guard(accessoryMutex);
		if (accessoryID == AccessoryNone || accessories.count(accessoryID) != 1)
		{
			result = Languages::GetText(Languages::TextAccessoryDoesNotExist);
			return false;
		}

		accessory = accessories.at(accessoryID);
		if (accessory == nullptr)
		{
			result = Languages::GetText(Languages::TextAccessoryDoesNotExist);
			return false;
		}

		ObjectIdentifier accessoryIdentifier(ObjectTypeAccessory, accessoryID);
		if (ObjectIsPartOfRoute(accessoryIdentifier, accessory, result))
		{
			return false;
		}

		accessories.erase(accessoryID);
	}

	if (storage)
	{
		storage->DeleteAccessory(accessoryID);
	}
	const string& name = accessory->GetName();
	const string& matchKey = accessory->GetMatchKey();
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->AccessoryDelete(accessoryID, name, matchKey);
	}
	delete accessory;
	return true;
}

bool Manager::AccessoryRelease(const AccessoryID accessoryID)
{
	Accessory* accessory = GetAccessory(accessoryID);
	if (accessory == nullptr)
	{
		return false;
	}
	return accessory->Release(logger, accessory->GetLocoBase());
}

AccessoryConfig Manager::GetAccessoryOfConfigByMatchKey(const ControlID controlId, const string& matchKey) const
{
	ControlInterface* control = GetControl(controlId);
	if (control == nullptr)
	{
		return AccessoryConfig();
	}
	return control->GetAccessoryByMatchKey(matchKey);
}

Accessory* Manager::GetAccessoryByMatchKey(const ControlID controlId, const string& matchKey) const
{
	std::lock_guard<std::mutex> guard(accessoryMutex);
	for (auto& accessory : accessories)
	{
		Accessory* accessoryConfig = accessory.second;
		if (accessoryConfig->GetControlID() == controlId && accessoryConfig->GetMatchKey().compare(matchKey) == 0)
		{
			return accessoryConfig;
		}
	}
	return nullptr;
}

void Manager::AccessoryRemoveMatchKey(const AccessoryID accessoryId)
{
	Accessory* accessory = GetAccessory(accessoryId);
	if (accessory == nullptr)
	{
		return;
	}
	accessory->ClearMatchKey();
}

/***************************
* Feedback                 *
***************************/

void Manager::FeedbackState(const ControlID controlID, const FeedbackPin pin, const DataModel::Feedback::FeedbackState state)
{
	Feedback* feedback = GetFeedback(controlID, pin);
	if (feedback != nullptr)
	{
		FeedbackState(feedback, state);
		return;
	}

	if (GetAutoAddFeedback() == false)
	{
		return;
	}

	string name = "Feedback auto added " + std::to_string(controlID) + "/" + std::to_string(pin);
	logger->Info(Languages::TextAddingFeedback, name);
	string result;

	FeedbackSave(FeedbackNone, name, DataModel::LayoutItem::VisibleNo, 0, 0, 0, DataModel::LayoutItem::Rotation0, controlID, "", pin, false, FeedbackTypeDefault, RouteNone, result);
}

void Manager::FeedbackState(const FeedbackID feedbackID, const DataModel::Feedback::FeedbackState state)
{
	Feedback* feedback = GetFeedback(feedbackID);
	if (feedback == nullptr)
	{
		return;
	}
	FeedbackState(feedback, state);
}

void Manager::FeedbackPublishState(const Feedback* feedback)
{
	if (feedback == nullptr)
	{
		return;
	}
	DataModel::Feedback::FeedbackState state = feedback->GetState();
	const string& feedbackName = feedback->GetName();
	logger->Info(state ? Languages::TextFeedbackStateIsOn : Languages::TextFeedbackStateIsOff, feedbackName);
	const FeedbackID feedbackID = feedback->GetID();
	{
		std::lock_guard<std::mutex> guard(controlMutex);
		for (auto& control : controls)
		{
			control.second->FeedbackState(feedbackName, feedbackID, state);
		}
	}
}

Feedback* Manager::GetFeedback(const FeedbackID feedbackID) const
{
	std::lock_guard<std::mutex> guard(feedbackMutex);
	return GetFeedbackUnlocked(feedbackID);
}

Feedback* Manager::GetFeedbackUnlocked(const FeedbackID feedbackID) const
{
	if (feedbacks.count(feedbackID) != 1)
	{
		return nullptr;
	}
	return feedbacks.at(feedbackID);
}

Feedback* Manager::GetFeedback(const ControlID controlID, const FeedbackPin pin) const
{
	std::lock_guard<std::mutex> guard(feedbackMutex);
	for (auto& feedback : feedbacks)
	{
		if (feedback.second->GetControlID() == controlID
			&& feedback.second->GetPin() == pin)
		{
			return feedback.second;
		}
	}
	return nullptr;
}

const std::string& Manager::GetFeedbackName(const FeedbackID feedbackID) const
{
	std::lock_guard<std::mutex> guard(feedbackMutex);
	if (feedbacks.count(feedbackID) != 1)
	{
		return unknownFeedback;
	}
	return feedbacks.at(feedbackID)->GetName();
}

const map<string,DataModel::FeedbackConfig> Manager::FeedbackConfigByName() const
{
	map<string,DataModel::FeedbackConfig> out;
	{
		std::lock_guard<std::mutex> guard(feedbackMutex);
		for (auto& feedback : feedbacks)
		{
			out[feedback.second->GetName()] = *(feedback.second);
		}
	}
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->AddUnmatchedFeedbacks(out);
	}

	return out;
}

bool Manager::FeedbackSave(FeedbackID feedbackID,
	const std::string& name,
	const Visible visible,
	const LayoutPosition posX,
	const LayoutPosition posY,
	const LayoutPosition posZ,
	const LayoutRotation rotation,
	const ControlID controlID,
	const string& matchKey,
	const FeedbackPin pin,
	const bool inverted,
	const FeedbackType feedbackType,
	const RouteID routeId,
	string& result)
{
	Feedback* feedback = GetFeedback(feedbackID);
	if (visible && !CheckLayoutItemPosition(feedback, posX, posY, posZ, result))
	{
		return false;
	}

	if (feedback == nullptr)
	{
		feedback = CreateAndAddObject(feedbacks, feedbackMutex);
	}

	if (feedback == nullptr)
	{
		result = Languages::GetText(Languages::TextUnableToAddFeedback);
		return false;
	}

	// if we have a new object we have to update feedbackID
	feedbackID = feedback->GetID();

	feedback->SetName(CheckObjectName(feedbacks, feedbackMutex, feedbackID, name.size() == 0 ? "F" : name));
	feedback->SetVisible(visible);
	feedback->SetPosX(posX);
	feedback->SetPosY(posY);
	feedback->SetPosZ(posZ);
	feedback->SetRotation(rotation);
	feedback->SetControlID(controlID);
	feedback->SetMatchKey(matchKey);
	feedback->SetPin(pin);
	feedback->SetInverted(inverted);
	feedback->SetFeedbackType(feedbackType);
	feedback->SetRouteId(routeId);

	FeedbackSaveAndPublishSettings(feedback);
	return true;
}

bool Manager::FeedbackPosition(const FeedbackID feedbackID,
	const LayoutPosition posX,
	const LayoutPosition posY,
	string& result)
{
	Feedback* feedback = GetFeedback(feedbackID);
	if (feedback == nullptr)
	{
		result = Languages::GetText(Languages::TextFeedbackDoesNotExist);
		return false;
	}

	if (!feedback->GetVisible())
	{
		return false;
	}

	if (!CheckLayoutItemPosition(feedback, posX, posY, feedback->GetPosZ(), result))
	{
		return false;
	}

	feedback->SetPosX(posX);
	feedback->SetPosY(posY);

	FeedbackSaveAndPublishSettings(feedback);
	return true;
}

bool Manager::FeedbackRotate(const FeedbackID feedbackID,
	string& result)
{
	Feedback* feedback = GetFeedback(feedbackID);
	if (feedback == nullptr)
	{
		result = Languages::GetText(Languages::TextFeedbackDoesNotExist);
		return false;
	}

	LayoutRotation newRotation = feedback->GetRotation();
	++newRotation;
	if (!CheckLayoutItemPosition(feedback, feedback->GetPosX(), feedback->GetPosY(), feedback->GetPosZ(), LayoutItem::Width1, LayoutItem::Height1, newRotation, result))
	{
		return false;
	}

	feedback->SetRotation(newRotation);

	FeedbackSaveAndPublishSettings(feedback);
	return true;
}

void Manager::FeedbackSaveAndPublishSettings(const Feedback* const feedback)
{
	// save in db
	if (storage)
	{
		storage->Save(*feedback);
	}
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->FeedbackSettings(feedback->GetID(), feedback->GetName());
	}
}

const map<string,DataModel::Feedback*> Manager::FeedbackListByName() const
{
	map<string,DataModel::Feedback*> out;
	std::lock_guard<std::mutex> guard(feedbackMutex);
	for (auto& feedback : feedbacks)
	{
		out[feedback.second->GetName()] = feedback.second;
	}
	return out;
}

const map<RouteID,string> Manager::RoutesOfTrack(const TrackID trackID) const
{
	map<RouteID,string> out;
	const Track* track = GetTrack(trackID);
	if (track == nullptr)
	{
		return out;
	}
	const vector<const Route*> routesOfTrack = track->GetRoutes();
	for (auto route : routesOfTrack)
	{
		out[route->GetID()] = route->GetName();
	}
	return out;
}

const map<string,FeedbackID> Manager::FeedbacksOfTrack(const TrackID trackID) const
{
	map<string,FeedbackID> out;
	const Track* track = GetTrack(trackID);
	if (track == nullptr)
	{
		return out;
	}
	const vector<Relation*> feedbacksOfTrack = track->GetFeedbacks();
	for (auto feedbackRelation: feedbacksOfTrack)
	{
		const FeedbackID feedbackID = feedbackRelation->ObjectID2();
		Feedback* feedback = GetFeedback(feedbackID);
		if (feedback == nullptr)
		{
			continue;
		}
		out[feedback->GetName()] = feedbackID;
	}
	return out;
}

bool Manager::FeedbackDelete(const FeedbackID feedbackID,
	string& result)
{
	Feedback* feedback = nullptr;
	{
		std::lock_guard<std::mutex> guard(feedbackMutex);
		if (feedbackID == FeedbackNone || feedbacks.count(feedbackID) != 1)
		{
			result = Languages::GetText(Languages::TextFeedbackDoesNotExist);
			return false;
		}

		feedback = feedbacks.at(feedbackID);
		Track* track = feedback->GetTrack();
		if (track != nullptr)
		{
			result = Logger::Logger::Format(Languages::GetText(Languages::TextFeedbackIsUsedByTrack), feedback->GetName(), track->GetName());
			return false;
		}

		feedbacks.erase(feedbackID);
	}

	if (storage)
	{
		storage->DeleteFeedback(feedbackID);
	}
	const string& name = feedback->GetName();
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->FeedbackDelete(feedbackID, name);
	}
	delete feedback;
	return true;
}

FeedbackConfig Manager::GetFeedbackOfConfigByMatchKey(const ControlID controlId, const string& matchKey) const
{
	ControlInterface* control = GetControl(controlId);
	if (control == nullptr)
	{
		return FeedbackConfig();
	}
	return control->GetFeedbackByMatchKey(matchKey);
}

Feedback* Manager::GetFeedbackByMatchKey(const ControlID controlId, const string& matchKey) const
{
	std::lock_guard<std::mutex> guard(feedbackMutex);
	for (auto& feedback : feedbacks)
	{
		Feedback* feedbackConfig = feedback.second;
		if (feedbackConfig->GetControlID() == controlId && feedbackConfig->GetMatchKey().compare(matchKey) == 0)
		{
			return feedback.second;
		}
	}
	return nullptr;
}

void Manager::FeedbackRemoveMatchKey(const FeedbackID feedbackId)
{
	Feedback* feedback = GetFeedback(feedbackId);
	if (feedback == nullptr)
	{
		return;
	}
	feedback->ClearMatchKey();
}

void Manager::FeedbackReplaceMatchKey(const FeedbackID feedbackId, const std::string& newMatchKey)
{
	Feedback* feedback = GetFeedback(feedbackId);
	if (feedback == nullptr)
	{
		return;
	}
	feedback->SetMatchKey(newMatchKey);
}

const map<string,FeedbackConfig> Manager::GetUnmatchedFeedbacksOfControl(const ControlID controlId,
	const std::string& matchKey) const
{
	ControlInterface* control = GetControl(controlId);
	map<string,FeedbackConfig> out;
	if (control == nullptr || !control->CanHandle(Hardware::CapabilityFeedbackDatabase))
	{
		return out;
	}
	out = control->GetUnmatchedFeedbacks(matchKey);
	out[""].SetName("");
	return out;
}

/***************************
* Track                    *
***************************/

Track* Manager::GetTrack(const TrackID trackID) const
{
	std::lock_guard<std::mutex> guard(trackMutex);
	if (tracks.count(trackID) != 1)
	{
		return nullptr;
	}
	return tracks.at(trackID);
}

const std::string& Manager::GetTrackName(const TrackID trackID) const
{
	if (tracks.count(trackID) != 1)
	{
		return unknownTrack;
	}
	return tracks.at(trackID)->GetName();
}

const map<string,DataModel::Track*> Manager::TrackListByName() const
{
	map<string,DataModel::Track*> out;
	std::lock_guard<std::mutex> guard(trackMutex);
	for (auto& track : tracks)
	{
		out[track.second->GetName()] = track.second;
	}
	return out;
}

const map<string,TrackID> Manager::TrackListIdByName() const
{
	map<string,TrackID> out;
	std::lock_guard<std::mutex> guard(trackMutex);
	for (auto& track : tracks)
	{
		out[track.second->GetName()] = track.second->GetID();
	}
	return out;
}

bool Manager::TrackSave(TrackID trackID,
	const std::string& name,
	const bool showName,
	const LayoutPosition posX,
	const LayoutPosition posY,
	const LayoutPosition posZ,
	const LayoutItemSize height,
	const LayoutRotation rotation,
	const DataModel::TrackType trackType,
	const vector<Relation*>& newFeedbacks,
	const vector<Relation*>& newSignals,
	const DataModel::SelectRouteApproach selectRouteApproach,
	const bool allowLocoTurn,
	const bool releaseWhenFree,
	string& result)
{
	Track* track = GetTrack(trackID);
	if (!CheckLayoutItemPosition(track, posX, posY, posZ, LayoutItem::Width1, height, rotation, result))
	{
		return false;
	}

	if (track == nullptr)
	{
		track = CreateAndAddObject(tracks, trackMutex);
	}

	if (track == nullptr)
	{
		result = Languages::GetText(Languages::TextUnableToAddTrack);
		return false;
	}

	// if we have a new object we have to update trackID
	if (trackID == TrackNone)
	{
		trackID = track->GetID();
		for (auto feedback : newFeedbacks)
		{
			feedback->ObjectID1(trackID);
		}
		for (auto signal : newSignals)
		{
			signal->ObjectID1(trackID);
		}
	}

	// update existing track
	track->SetName(CheckObjectName(tracks, trackMutex, trackID, name.size() == 0 ? "T" : name));
	track->SetShowName(showName);
	track->SetHeight(height);
	track->SetRotation(rotation);
	track->SetPosX(posX);
	track->SetPosY(posY);
	track->SetPosZ(posZ);
	track->SetTrackType(trackType);
	track->AssignFeedbacks(newFeedbacks);
	track->AssignSignals(newSignals);
	track->SetSelectRouteApproach(selectRouteApproach);
	track->SetAllowLocoTurn(allowLocoTurn);
	track->SetReleaseWhenFree(releaseWhenFree);

	TrackSaveAndPublishSettings(track);
	return true;
}

bool Manager::TrackPosition(const TrackID trackID,
	const LayoutPosition posX,
	const LayoutPosition posY,
	string& result)
{
	Track* track = GetTrack(trackID);
	if (track == nullptr)
	{
		result = Languages::GetText(Languages::TextTrackDoesNotExist);
		return false;
	}

	if (!CheckLayoutItemPosition(track, posX, posY, track->GetPosZ(), LayoutItem::Width1, track->GetHeight(), track->GetRotation(), result))
	{
		return false;
	}

	track->SetPosX(posX);
	track->SetPosY(posY);

	TrackSaveAndPublishSettings(track);
	return true;
}

bool Manager::TrackRotate(const TrackID trackID,
	string& result)
{
	Track* track = GetTrack(trackID);
	if (track == nullptr)
	{
		result = Languages::GetText(Languages::TextTrackDoesNotExist);
		return false;
	}

	LayoutRotation newRotation = track->GetRotation();
	++newRotation;
	if (!CheckLayoutItemPosition(track, track->GetPosX(), track->GetPosY(), track->GetPosZ(), LayoutItem::Width1, track->GetHeight(), newRotation, result))
	{
		return false;
	}

	track->SetRotation(newRotation);

	TrackSaveAndPublishSettings(track);
	return true;
}

void Manager::TrackSaveAndPublishSettings(const Track* const track)
{
	TrackSave(track);

	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->TrackSettings(track->GetID(), track->GetName());
	}
}

bool Manager::TrackDelete(const TrackID trackID,
	string& result)
{
	Track* track = nullptr;
	{
		std::lock_guard<std::mutex> guard(trackMutex);
		if (trackID == TrackNone || tracks.count(trackID) != 1)
		{
			result = Languages::GetText(Languages::TextTrackDoesNotExist);
			return false;
		}

		track = tracks.at(trackID);
		if (track == nullptr)
		{
			result = Languages::GetText(Languages::TextTrackDoesNotExist);
			return false;
		}

		if (track->IsInUse())
		{
			result = Logger::Logger::Format(Languages::GetText(Languages::TextTrackIsUsedByLoco), track->GetName(), GetLocoBaseName(track->GetLocoBase()));
			return false;
		}

		Route* route = GetFirstRouteFromOrToTrack(trackID);
		if (route != nullptr)
		{
			result = Logger::Logger::Format(Languages::GetText(Languages::TextTrackIsUsedByRoute), track->GetName(), route->GetName());
			return false;
		}

		ObjectIdentifier trackIdentifier(ObjectTypeTrack, trackID);
		if (ObjectIsPartOfRoute(trackIdentifier, track, result))
		{
			return false;
		}

		tracks.erase(trackID);
	}

	if (storage)
	{
		storage->DeleteTrack(trackID);
	}
	const string& name = track->GetName();
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->TrackDelete(trackID, name);
	}

	Cluster* cluster = track->GetCluster();
	if (cluster != nullptr)
	{
		cluster->DeleteTrack(track);
	}

	delete track;
	return true;
}

/***************************
* Switch                   *
***************************/

bool Manager::SwitchState(const ControlType controlType, const SwitchID switchID, const DataModel::AccessoryState state, const bool force)
{
	if (boosterState == BoosterStateStop)
	{
		return false;
	}

	Switch* mySwitch = GetSwitch(switchID);
	SwitchState(controlType, mySwitch, state, force);
	return true;
}

void Manager::SwitchState(const ControlType controlType, Switch* mySwitch, const DataModel::AccessoryState state, const bool force)
{
	if (mySwitch == nullptr)
	{
		return;
	}

	if (force == false && mySwitch->IsInUse())
	{
		logger->Warning(Languages::TextSwitchIsLocked, mySwitch->GetName());
		return;
	}

	if (!GetExecuteAccessory())
	{
		const DataModel::AccessoryState oldState = mySwitch->GetAccessoryState();
		if (oldState == state)
		{
			return;
		}
	}
	mySwitch->SetAccessoryState(state);

	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->SwitchState(controlType, mySwitch);
	}
}

Switch* Manager::GetSwitch(const SwitchID switchID) const
{
	std::lock_guard<std::mutex> guard(switchMutex);
	if (switches.count(switchID) != 1)
	{
		return nullptr;
	}
	return switches.at(switchID);
}

Switch* Manager::GetSwitch(const ControlID controlID, const Protocol protocol, const Address address) const
{
	std::lock_guard<std::mutex> guard(switchMutex);
	for (auto& mySwitch : switches)
	{
		Switch* s = mySwitch.second;
		if ((s->GetControlID() == controlID)
			&& (s->GetProtocol() == protocol)
			&& (s->UsesAddress(address)))
		{
			return s;
		}
	}
	return nullptr;
}

const std::string& Manager::GetSwitchName(const SwitchID switchID) const
{
	if (switches.count(switchID) != 1)
	{
		return unknownSwitch;
	}
	return switches.at(switchID)->GetName();
}

bool Manager::SwitchSave(SwitchID switchID,
	const string& name,
	const LayoutPosition posX,
	const LayoutPosition posY,
	const LayoutPosition posZ,
	const LayoutRotation rotation,
	const ControlID controlID,
	const string& matchKey,
	const Protocol protocol,
	const Address address,
	const Address serverAddress,
	const DataModel::AccessoryType type,
	const DataModel::AccessoryPulseDuration duration,
	const bool inverted,
	string& result)
{
	if (!CheckControlAccessoryProtocolAddress(controlID, protocol, address, result))
	{
		return false;
	}

	Switch* mySwitch = GetSwitch(switchID);
	if (!CheckLayoutItemPosition(mySwitch, posX, posY, posZ, LayoutItem::Width1, Switch::CalculateHeightFromType(type), rotation, result))
	{
		return false;
	}

	if (mySwitch == nullptr)
	{
		mySwitch = CreateAndAddObject(switches, switchMutex);
	}

	if (mySwitch == nullptr)
	{
		result = Languages::GetText(Languages::TextUnableToAddSwitch);
		return false;
	}

	// if we have a new object we have to update switchID
	switchID = mySwitch->GetID();

	// update existing switch
	mySwitch->SetName(CheckObjectName(switches, switchMutex, switchID, name.size() == 0 ? "S" : name));
	mySwitch->SetPosX(posX);
	mySwitch->SetPosY(posY);
	mySwitch->SetPosZ(posZ);
	mySwitch->SetRotation(rotation);
	mySwitch->SetControlID(controlID);
	mySwitch->SetMatchKey(matchKey);
	mySwitch->SetProtocol(protocol);
	mySwitch->SetAddress(address);
	mySwitch->SetServerAddress(serverAddress);
	mySwitch->SetAccessoryType(type);
	mySwitch->SetAccessoryPulseDuration(duration);
	mySwitch->SetInverted(inverted);

	SwitchSaveAndPublishSettings(mySwitch);
	return true;
}

bool Manager::SwitchPosition(const SwitchID switchID,
	const LayoutPosition posX,
	const LayoutPosition posY,
	string& result)
{
	Switch* mySwitch = GetSwitch(switchID);
	if (mySwitch == nullptr)
	{
		result = Languages::GetText(Languages::TextSwitchDoesNotExist);
		return false;
	}

	if (!CheckLayoutItemPosition(mySwitch, posX, posY, mySwitch->GetPosZ(), LayoutItem::Width1, mySwitch->GetHeight(), mySwitch->GetRotation(), result))
	{
		return false;
	}

	mySwitch->SetPosX(posX);
	mySwitch->SetPosY(posY);

	SwitchSaveAndPublishSettings(mySwitch);
	return true;
}

bool Manager::SwitchRotate(const SwitchID switchID,
	string& result)
{
	Switch* mySwitch = GetSwitch(switchID);
	if (mySwitch == nullptr)
	{
		result = Languages::GetText(Languages::TextSwitchDoesNotExist);
		return false;
	}

	LayoutRotation newRotation = mySwitch->GetRotation();
	++newRotation;
	if (!CheckLayoutItemPosition(mySwitch, mySwitch->GetPosX(), mySwitch->GetPosY(), mySwitch->GetPosZ(), LayoutItem::Width1, mySwitch->GetHeight(), newRotation, result))
	{
		return false;
	}

	mySwitch->Rotate();

	SwitchSaveAndPublishSettings(mySwitch);
	return true;
}

void Manager::SwitchSaveAndPublishSettings(const Switch* const mySwitch)
{
	if (storage)
	{
		storage->Save(*mySwitch);
	}

	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->SwitchSettings(mySwitch->GetID(), mySwitch->GetName(), mySwitch->GetMatchKey());
	}
}

bool Manager::SwitchDelete(const SwitchID switchID,
	string& result)
{
	Switch* mySwitch = nullptr;
	{
		std::lock_guard<std::mutex> guard(switchMutex);
		if (switchID == SwitchNone || switches.count(switchID) != 1)
		{
			result = Languages::GetText(Languages::TextSwitchDoesNotExist);
			return false;
		}

		mySwitch = switches.at(switchID);
		if (mySwitch == nullptr)
		{
			result = Languages::GetText(Languages::TextSwitchDoesNotExist);
			return false;
		}

		ObjectIdentifier switchIdentifier(ObjectTypeSwitch, switchID);
		if (ObjectIsPartOfRoute(switchIdentifier, mySwitch, result))
		{
			return false;
		}
		switches.erase(switchID);
	}

	if (storage)
	{
		storage->DeleteSwitch(switchID);
	}

	const string& name = mySwitch->GetName();
	const string& matchKey = mySwitch->GetMatchKey();
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->SwitchDelete(switchID, name, matchKey);
	}
	delete mySwitch;
	return true;
}

const map<string,DataModel::AccessoryConfig> Manager::SwitchConfigByName() const
{
	map<string,DataModel::AccessoryConfig> out;
	{
		std::lock_guard<std::mutex> guard(switchMutex);
		for (auto& mySwitch : switches)
		{
			out[mySwitch.second->GetName()] = *(mySwitch.second);
		}
	}

	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->AddUnmatchedAccessories(out);
	}
	return out;
}

bool Manager::SwitchRelease(const RouteID switchID)
{
	Switch* mySwitch = GetSwitch(switchID);
	if (mySwitch == nullptr)
	{
		return false;
	}
	return mySwitch->Release(logger, mySwitch->GetLocoBase());
}

Switch* Manager::GetSwitchByMatchKey(const ControlID controlId, const string& matchKey) const
{
	std::lock_guard<std::mutex> guard(switchMutex);
	for (auto& mySwitch : switches)
	{
		Switch* switchConfig = mySwitch.second;
		if (switchConfig->GetControlID() == controlId && switchConfig->GetMatchKey().compare(matchKey) == 0)
		{
			return switchConfig;
		}
	}
	return nullptr;
}

void Manager::SwitchRemoveMatchKey(const SwitchID switchId)
{
	Switch* mySwitch = GetSwitch(switchId);
	if (mySwitch == nullptr)
	{
		return;
	}
	mySwitch->ClearMatchKey();
}

/***************************
* Route                   *
***************************/

bool Manager::RouteExecute(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier, const RouteID routeID)
{
	Route* route = GetRoute(routeID);
	if (route == nullptr)
	{
		return false;
	}
	return route->Execute(logger, locoBaseIdentifier);
}

void Manager::RouteExecuteAsync(Logger::Logger* logger, const RouteID routeID)
{
	Route* route = GetRoute(routeID);
	if (route == nullptr)
	{
		return;
	}
	__attribute__((unused)) auto r = std::async(std::launch::async, Route::ExecuteStatic, logger, route);
}

string Manager::GetRouteList() const
{
	string out;
	std::lock_guard<std::mutex> guard(routeMutex);
	for (auto& route : routes)
	{
		Route* r = route.second;
		out += to_string(r->GetID()) + ";";
		out += to_string(r->GetFromTrack()) + ";";
		out += to_string(r->GetFromOrientation()) + ";";
		out += to_string(r->GetToTrack()) + ";";
		out += to_string(r->GetToOrientation()) + ";";
		out += r->GetName() + "\n";
	}
	return out;
}

Route* Manager::GetRoute(const RouteID routeID) const
{
	std::lock_guard<std::mutex> guard(routeMutex);
	if (routes.count(routeID) != 1)
	{
		return nullptr;
	}
	return routes.at(routeID);
}

const string& Manager::GetRouteName(const RouteID routeID) const
{
	std::lock_guard<std::mutex> guard(routeMutex);
	if (routes.count(routeID) != 1)
	{
		return unknownRoute;
	}
	return routes.at(routeID)->GetName();
}

bool Manager::RouteSave(RouteID routeID,
	const std::string& name,
	const Delay delay,
	const Route::PushpullType pushpull,
	const Propulsion propulsion,
	const TrainType trainType,
	const Length minTrainLength,
	const Length maxTrainLength,
	const std::vector<DataModel::Relation*>& relationsAtLock,
	const std::vector<DataModel::Relation*>& relationsAtUnlock,
	const Visible visible,
	const LayoutPosition posX,
	const LayoutPosition posY,
	const LayoutPosition posZ,
	const Automode automode,
	const TrackID fromTrack,
	const Orientation fromOrientation,
	const TrackID toTrack,
	const Orientation toOrientation,
	const Route::Speed speed,
	const FeedbackID feedbackIdReduced,
	const FeedbackID feedbackIdCreep,
	const FeedbackID feedbackIdStop,
	const FeedbackID feedbackIdOver,
	const Pause waitAfterRelease,
	const RouteID followUpRoute,
	string& result)
{

	Route* route = GetRoute(routeID);
	if (visible && !CheckLayoutItemPosition(route, posX, posY, posZ, result))
	{
		return false;
	}

	if (route == nullptr)
	{
		route = CreateAndAddObject(routes, routeMutex);
	}
	if (route == nullptr)
	{
		result = Languages::GetText(Languages::TextUnableToAddRoute);
		return false;
	}

	// remove route from old track
	Track* oldTrack = GetTrack(route->GetFromTrack());
	if (oldTrack != nullptr)
	{
		oldTrack->RemoveRoute(route);
		TrackSave(oldTrack);
	}

	// if we have a new object we have to update routeID
	if (routeID == 0)
	{
		routeID = route->GetID();
		for (auto atLock : relationsAtLock)
		{
			atLock->ObjectID1(routeID);
		}
		for (auto atUnlock : relationsAtUnlock)
		{
			atUnlock->ObjectID1(routeID);
		}
	}

	// update existing route
	route->SetName(CheckObjectName(routes, routeMutex, routeID, name.size() == 0 ? "S" : name));
	route->SetDelay(delay);
	route->AssignRelationsAtLock(relationsAtLock);
	route->AssignRelationsAtUnlock(relationsAtUnlock);
	route->SetVisible(visible);
	route->SetPosX(posX);
	route->SetPosY(posY);
	route->SetPosZ(posZ);
	route->SetAutomode(automode);
	if (automode == AutomodeYes)
	{
		route->SetFromTrack(fromTrack);
		route->SetFromOrientation(fromOrientation);
		route->SetToTrack(toTrack);
		route->SetToOrientation(toOrientation);
		route->SetSpeed(speed);
		route->SetFeedbackIdReduced(feedbackIdReduced);
		route->SetFeedbackIdCreep(feedbackIdCreep);
		route->SetFeedbackIdStop(feedbackIdStop);
		route->SetFeedbackIdOver(feedbackIdOver);
		route->SetPushpull(pushpull);
		route->SetPropulsion(propulsion);
		route->SetTrainType(trainType);
		route->SetMinTrainLength(minTrainLength);
		route->SetMaxTrainLength(maxTrainLength);
		route->SetWaitAfterRelease(waitAfterRelease);
		route->SetFollowUpRoute(followUpRoute);
	}
	else
	{
		route->SetFromTrack(TrackNone);
		route->SetFromOrientation(OrientationRight);
		route->SetToTrack(TrackNone);
		route->SetToOrientation(OrientationLeft);
		route->SetSpeed(Route::SpeedTravel);
		route->SetFeedbackIdReduced(FeedbackNone);
		route->SetFeedbackIdCreep(FeedbackNone);
		route->SetFeedbackIdStop(FeedbackNone);
		route->SetFeedbackIdOver(FeedbackNone);
		route->SetPushpull(Route::PushpullTypeBoth);
		route->SetPropulsion(PropulsionAll);
		route->SetTrainType(TrainTypeAll);
		route->SetMinTrainLength(0);
		route->SetMaxTrainLength(0);
		route->SetWaitAfterRelease(0);
		route->SetFollowUpRoute(RouteNone);
	}

	//Add new route
	Track* newTrack = GetTrack(route->GetFromTrack());
	if (newTrack != nullptr)
	{
		newTrack->AddRoute(route);
		TrackSave(newTrack);
	}

	RouteSaveAndPublishSettings(route);
	return true;
}

bool Manager::RoutePosition(const RouteID routeID,
	const LayoutPosition posX,
	const LayoutPosition posY,
	string& result)
{
	Route* route = GetRoute(routeID);
	if (route == nullptr)
	{
		result = Languages::GetText(Languages::TextRouteDoesNotExist);
		return false;
	}

	if (!route->GetVisible())
	{
		return false;
	}

	if (!CheckLayoutItemPosition(route, posX, posY, route->GetPosZ(), result))
	{
		return false;
	}

	route->SetPosX(posX);
	route->SetPosY(posY);

	RouteSaveAndPublishSettings(route);
	return true;
}

void Manager::RouteSaveAndPublishSettings(const Route* const route)
{
	RouteSave(route);

	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->RouteSettings(route->GetID(), route->GetName());
	}
}

const map<string,DataModel::Route*> Manager::RouteListByName() const
{
	map<string,DataModel::Route*> out;
	std::lock_guard<std::mutex> guard(routeMutex);
	for (auto& route : routes)
	{
		out[route.second->GetName()] = route.second;
	}
	return out;
}

bool Manager::RouteDelete(const RouteID routeID,
	string& result)
{
	Route* route = nullptr;
	{
		{
			std::lock_guard<std::mutex> guard(routeMutex);
			if (routeID == RouteNone || routes.count(routeID) != 1)
			{
				result = Languages::GetText(Languages::TextRouteDoesNotExist);
				return false;
			}

			route = routes.at(routeID);
			if (route == nullptr)
			{
				result = Languages::GetText(Languages::TextRouteDoesNotExist);
				return false;
			}

			if (route->IsInUse())
			{
				result = Logger::Logger::Format(Languages::GetText(Languages::TextRouteIsInUse), route->GetName());
				return false;
			}
		}

		// this must not be in the lock_guard, it would be a deadlock.
		ObjectIdentifier routeIdentifier(ObjectTypeRoute, routeID);
		if (ObjectIsPartOfRoute(routeIdentifier, route, result))
		{
			return false;
		}

		{
			std::lock_guard<std::mutex> guard(routeMutex);
			routes.erase(routeID);
		}
	}

	if (storage)
	{
		storage->DeleteRoute(routeID);
	}

	const string& routeName = route->GetName();
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->RouteDelete(routeID, routeName);
	}
	delete route;
	return true;
}

Route* Manager::GetFirstRouteFromOrToTrack(const TrackID trackID) const
{
	std::lock_guard<std::mutex> guard(routeMutex);
	for (auto& route : routes)
	{
		if (route.second->GetToTrack() == trackID || route.second->GetFromTrack() == trackID)
		{
			return route.second;
		}
	}
	return nullptr;
}

Layer* Manager::GetLayer(const LayerID layerID) const
{
	std::lock_guard<std::mutex> guard(layerMutex);
	if (layers.count(layerID) != 1)
	{
		return nullptr;
	}
	return layers.at(layerID);
}

const map<string,LayerID> Manager::LayerListByName() const
{
	map<string,LayerID> list;
	std::lock_guard<std::mutex> guard(layerMutex);
	for (auto& layer : layers)
	{
		list[layer.second->GetName()] = layer.first;
	}
	return list;
}

const map<string,LayerID> Manager::LayerListByNameWithFeedback() const
{
	map<string,LayerID> list = LayerListByName();
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		if (!control.second->CanHandle(Hardware::CapabilityFeedback))
		{
			continue;
		}
		list["| Feedbacks of " + control.second->GetShortName()] = -control.first;
	}
	return list;
}

bool Manager::LayerSave(LayerID layerID, const std::string&name, std::string& result)
{
	Layer* layer = GetLayer(layerID);
	if (layer == nullptr)
	{
		layer = CreateAndAddObject(layers, layerMutex);
	}

	if (layer == nullptr)
	{
		result = Languages::GetText(Languages::TextUnableToAddLayer);
		return false;
	}

	// if we have a new object we have to update layerID
	layerID = layer->GetID();

	// update existing layer
	layer->SetName(CheckObjectName(layers, layerMutex, layerID, name.size() == 0 ? "L" : name));

	// save in db
	if (storage)
	{
		storage->Save(*layer);
	}
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->LayerSettings(layerID, name);
	}
	return true;
}

bool Manager::LayerHasElements(const Layer* layer,
	string& result)
{
	LayerID layerId = layer->GetID();
	for (auto& track : tracks)
	{
		if (track.second->IsVisibleOnLayer(layerId))
		{
			result = Logger::Logger::Format(Languages::GetText(Languages::TextLayerIsUsedByTrack), layer->GetName(), track.second->GetName());
			return true;
		}
	}
	for (auto& mySwitch : switches)
	{
		if (mySwitch.second->IsVisibleOnLayer(layerId))
		{
			result = Logger::Logger::Format(Languages::GetText(Languages::TextLayerIsUsedBySwitch), layer->GetName(), mySwitch.second->GetName());
			return true;
		}
	}
	for (auto& signal : signals)
	{
		if (signal.second->IsVisibleOnLayer(layerId))
		{
			result = Logger::Logger::Format(Languages::GetText(Languages::TextLayerIsUsedBySignal), layer->GetName(), signal.second->GetName());
			return true;
		}
	}
	for (auto& accessory : accessories)
	{
		if (accessory.second->IsVisibleOnLayer(layerId))
		{
			result = Logger::Logger::Format(Languages::GetText(Languages::TextLayerIsUsedByAccessory), layer->GetName(), accessory.second->GetName());
			return true;
		}
	}
	for (auto& route : routes)
	{
		if (route.second->IsVisibleOnLayer(layerId))
		{
			result = Logger::Logger::Format(Languages::GetText(Languages::TextLayerIsUsedByRoute), layer->GetName(), route.second->GetName());
			return true;
		}
	}
	for (auto& feedback : feedbacks)
	{
		if (feedback.second->IsVisibleOnLayer(layerId))
		{
			result = Logger::Logger::Format(Languages::GetText(Languages::TextLayerIsUsedByFeedback), layer->GetName(), feedback.second->GetName());
			return true;
		}
	}
	for (auto& text : texts)
	{
		if (text.second->IsVisibleOnLayer(layerId))
		{
			result = Logger::Logger::Format(Languages::GetText(Languages::TextLayerIsUsedByText), layer->GetName(), text.second->GetName());
			return true;
		}
	}
	return false;
}

bool Manager::LayerDelete(const LayerID layerID,
	string& result)
{
	if (layerID == LayerUndeletable)
	{
		result = Languages::GetText(Languages::TextLayer1IsUndeletable);
		return false;
	}

	if (layerID == LayerNone)
	{
		result = Languages::GetText(Languages::TextLayerDoesNotExist);
		return false;
	}

	Layer* layer = nullptr;
	{
		std::lock_guard<std::mutex> guard(layerMutex);
		if (layers.count(layerID) != 1)
		{
			result = Languages::GetText(Languages::TextLayerDoesNotExist);
			return false;
		}

		layer = layers.at(layerID);
		if (layer == nullptr)
		{
			result = Languages::GetText(Languages::TextLayerDoesNotExist);
			return false;
		}

		if (LayerHasElements(layer, result))
		{
			return false;
		}

		layers.erase(layerID);
	}

	if (storage)
	{
		storage->DeleteLayer(layerID);
	}

	const string& layerName = layer->GetName();
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->LayerDelete(layerID, layerName);
	}
	delete layer;
	return true;
}

/***************************
* Signal                   *
***************************/

bool Manager::SignalState(const ControlType controlType, const SignalID signalID, const DataModel::AccessoryState state, const bool force)
{
	if (boosterState == BoosterStateStop)
	{
		return false;
	}
	Signal* signal = GetSignal(signalID);
	if (signal == nullptr)
	{
		return false;
	}
	return SignalState(controlType, signal, state, force);
}

bool Manager::SignalState(const ControlType controlType, Signal* signal, const DataModel::AccessoryState state, const bool force)
{
	if (signal == nullptr)
	{
		return false;
	}

	if (force == false && signal->IsInUse())
	{
		logger->Warning(Languages::TextSignalIsLocked, signal->GetName());
		return false;
	}

	const DataModel::AccessoryState oldState = signal->GetAccessoryState();
	if (oldState == state)
	{
		return true;
	}
	signal->SetAccessoryState(state);

	SignalPublishState(controlType, signal);
	return true;
}

void Manager::SignalPublishState(const ControlType controlType, const DataModel::Signal* signal)
{
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->SignalState(controlType, signal);
	}
}

Signal* Manager::GetSignal(const SignalID signalID) const
{
	std::lock_guard<std::mutex> guard(signalMutex);
	if (signals.count(signalID) != 1)
	{
		return nullptr;
	}
	return signals.at(signalID);
}

Signal* Manager::GetSignal(const ControlID controlID, const Protocol protocol, const Address address) const
{
	std::lock_guard<std::mutex> guard(signalMutex);
	for (auto& signal : signals)
	{
		Signal* s = signal.second;
		if ((s->GetControlID() == controlID)
			&& (s->GetProtocol() == protocol)
			&& (s->UsesAddress(address)))
		{
			return s;
		}
	}
	return nullptr;
}

const std::string& Manager::GetSignalName(const SignalID signalID) const
{
	if (signals.count(signalID) != 1)
	{
		return unknownSignal;
	}
	return signals.at(signalID)->GetName();
}

bool Manager::SignalSave(SignalID signalID,
	const string& name,
	const LayoutPosition posX,
	const LayoutPosition posY,
	const LayoutPosition posZ,
	const LayoutItemSize height,
	const LayoutRotation rotation,
	const ControlID controlID,
	const string& matchKey,
	const Protocol protocol,
	const Address address,
	const Address serverAddress,
	const DataModel::AccessoryType type,
	const std::map<DataModel::AccessoryState,DataModel::AddressOffset>& offsets,
	const DataModel::AccessoryPulseDuration duration,
	const bool inverted,
	string& result)
{
	if (!CheckControlAccessoryProtocolAddress(controlID, protocol, address, result))
	{
		return false;
	}

	Signal* signal = GetSignal(signalID);
	if (!CheckLayoutItemPosition(signal, posX, posY, posZ, result))
	{
		return false;
	}

	if (signal == nullptr)
	{
		signal = CreateAndAddObject(signals, signalMutex);
	}

	if (signal == nullptr)
	{
		result = Languages::GetText(Languages::TextUnableToAddSignal);
		return false;
	}

	// if we have a new object we have to update locoID
	signalID = signal->GetID();

	signal->SetName(CheckObjectName(signals, signalMutex, signalID, name.size() == 0 ? "S" : name));
	signal->SetPosX(posX);
	signal->SetPosY(posY);
	signal->SetPosZ(posZ);
	signal->SetHeight(height);
	signal->SetRotation(rotation);
	signal->SetControlID(controlID);
	signal->SetMatchKey(matchKey);
	signal->SetProtocol(protocol);
	signal->SetAddress(address);
	signal->SetServerAddress(serverAddress);
	signal->SetAccessoryType(type);
	signal->SetStateAddressOffsets(offsets);
	signal->SetAccessoryPulseDuration(duration);
	signal->SetInverted(inverted);

	SignalSaveAndPublishSettings(signal);
	return true;
}

bool Manager::SignalPosition(const SignalID signalID,
	const LayoutPosition posX,
	const LayoutPosition posY,
	string& result)
{
	Signal* signal = GetSignal(signalID);
	if (signal == nullptr)
	{
		result = Languages::GetText(Languages::TextSignalDoesNotExist);
		return false;
	}

	if (!CheckLayoutItemPosition(signal, posX, posY, signal->GetPosZ(), result))
	{
		return false;
	}

	signal->SetPosX(posX);
	signal->SetPosY(posY);

	SignalSaveAndPublishSettings(signal);
	return true;
}

bool Manager::SignalRotate(const SignalID signalID,
	string& result)
{
	Signal* signal = GetSignal(signalID);
	if (signal == nullptr)
	{
		result = Languages::GetText(Languages::TextSignalDoesNotExist);
		return false;
	}

	signal->Rotate();

	SignalSaveAndPublishSettings(signal);
	return true;
}

void Manager::SignalSaveAndPublishSettings(const Signal* const signal)
{
	// save in db
	if (storage)
	{
		storage->Save(*signal);
	}

	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->SignalSettings(signal->GetID(), signal->GetName(), signal->GetMatchKey());
	}
}

bool Manager::SignalDelete(const SignalID signalID,
	string& result)
{
	Signal* signal = nullptr;
	{
		std::lock_guard<std::mutex> guard(signalMutex);
		if (signalID == SignalNone || signals.count(signalID) != 1)
		{
			result = Languages::GetText(Languages::TextSignalDoesNotExist);
			return false;
		}

		signal = signals.at(signalID);
		if (signal == nullptr)
		{
			result = Languages::GetText(Languages::TextSignalDoesNotExist);
			return false;
		}

		if (signal->IsInUse())
		{
			result = Logger::Logger::Format(Languages::GetText(Languages::TextSignalIsUsedByLoco), signal->GetName(), GetLocoBaseName(signal->GetLocoBase()));
			return false;
		}

		Track* track = signal->GetTrack();
		if (track)
		{
			result = Logger::Logger::Format(Languages::GetText(Languages::TextSignalIsUsedByTrack), signal->GetName(), track->GetName());
			return false;
		}

		ObjectIdentifier signalIdentifier(ObjectTypeSignal, signalID);
		if (ObjectIsPartOfRoute(signalIdentifier, signal, result))
		{
			return false;
		}

		signals.erase(signalID);
	}

	if (storage)
	{
		storage->DeleteSignal(signalID);
	}

	const string& name = signal->GetName();
	const string& matchKey = signal->GetMatchKey();
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->SignalDelete(signalID, name, matchKey);
	}

	delete signal;
	return true;
}

const map<string,DataModel::Signal*> Manager::SignalListByName() const
{
	map<string,DataModel::Signal*> out;
	std::lock_guard<std::mutex> guard(signalMutex);
	for (auto& signal : signals)
	{
		out[signal.second->GetName()] = signal.second;
	}
	return out;
}

const map<string,DataModel::AccessoryConfig> Manager::SignalConfigByName() const
{
	map<string,DataModel::AccessoryConfig> out;
	{
		std::lock_guard<std::mutex> guard(signalMutex);
		for (auto& signal : signals)
		{
			out[signal.second->GetName()] = *(signal.second);
		}
	}

	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->AddUnmatchedAccessories(out);
	}
	return out;
}

Signal* Manager::GetSignalByMatchKey(const ControlID controlId, const string& matchKey) const
{
	std::lock_guard<std::mutex> guard(signalMutex);
	for (auto& signal : signals)
	{
		Signal* signalConfig = signal.second;
		if (signalConfig->GetControlID() == controlId && signalConfig->GetMatchKey().compare(matchKey) == 0)
		{
			return signalConfig;
		}
	}
	return nullptr;
}

void Manager::SignalRemoveMatchKey(const SignalID signalId)
{
	Signal* signal = GetSignal(signalId);
	if (signal == nullptr)
	{
		return;
	}
	signal->ClearMatchKey();
}

bool Manager::SignalRelease(const SignalID signalID)
{
	Signal* signal = GetSignal(signalID);
	if (signal == nullptr)
	{
		return false;
	}
	return signal->Release(logger, ObjectIdentifier());
}

/***************************
* Cluster                  *
***************************/

Cluster* Manager::GetCluster(const ClusterID clusterID) const
{
	std::lock_guard<std::mutex> guard(clusterMutex);
	if (clusters.count(clusterID) != 1)
	{
		return nullptr;
	}
	return clusters.at(clusterID);
}

const map<string,DataModel::Cluster*> Manager::ClusterListByName() const
{
	map<string,DataModel::Cluster*> out;
	std::lock_guard<std::mutex> guard(clusterMutex);
	for (auto& cluster : clusters)
	{
		out[cluster.second->GetName()] = cluster.second;
	}
	return out;
}

bool Manager::ClusterSave(ClusterID clusterID,
	const string& name,
	const vector<Relation*>& newTracks,
	string& result)
{
	Cluster* cluster = GetCluster(clusterID);
	if (cluster == nullptr)
	{
		cluster = CreateAndAddObject(clusters, clusterMutex);
	}

	if (cluster == nullptr)
	{
		result = Languages::GetText(Languages::TextUnableToAddCluster);
		return false;
	}

	// if we have a new object we have to update clusterID
	if (clusterID == 0)
	{
		clusterID = cluster->GetID();
		for (auto track : newTracks)
		{
			track->ObjectID1(clusterID);
		}
	}

	// update existing cluster
	cluster->SetName(CheckObjectName(clusters, clusterMutex, clusterID, name.size() == 0 ? "C" : name));
	cluster->AssignTracks(newTracks);

	// save in db
	if (storage)
	{
		storage->Save(*cluster);
	}
	return true;
}

bool Manager::ClusterDelete(const ClusterID clusterID)
{
	if (clusterID == ClusterNone)
	{
		return false;
	}
	Cluster* cluster = nullptr;
	{
		std::lock_guard<std::mutex> guard(clusterMutex);
		if (clusters.count(clusterID) != 1)
		{
			return false;
		}

		cluster = clusters.at(clusterID);
		clusters.erase(clusterID);
	}

	cluster->DeleteTracks();

	if (storage)
	{
		storage->DeleteCluster(clusterID);
	}

	const string& clusterName = cluster->GetName();
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->ClusterDelete(clusterID, clusterName);
	}
	delete cluster;
	return true;
}

/***************************
* Text                  *
***************************/

Text* Manager::GetText(const TextID textID) const
{
	std::lock_guard<std::mutex> guard(textMutex);
	if (texts.count(textID) != 1)
	{
		return nullptr;
	}
	return texts.at(textID);
}

const map<string,DataModel::Text*> Manager::TextListByName() const
{
	map<string,DataModel::Text*> out;
	std::lock_guard<std::mutex> guard(textMutex);
	for (auto& text : texts)
	{
		out[text.second->GetName()] = text.second;
	}
	return out;
}

bool Manager::TextSave(TextID textID,
	const string& name,
	const LayoutPosition posX,
	const LayoutPosition posY,
	const LayoutPosition posZ,
	const LayoutItemSize width,
	const LayoutRotation rotation,
	string& result)
{
	Text* text = GetText(textID);
	if (!CheckLayoutItemPosition(text, posX, posY, posZ, width, LayoutItem::Height1, rotation, result))
	{
		return false;
	}

	if (text == nullptr)
	{
		text = CreateAndAddObject(texts, textMutex);
	}

	if (text == nullptr)
	{
		result = Languages::GetText(Languages::TextUnableToAddText);
		return false;
	}

	// if we have a new object we have to update textID
	textID = text->GetID();

	// update existing text
	text->SetName(CheckObjectName(texts, textMutex, textID, name.size() == 0 ? "T" : name));
	text->SetPosX(posX);
	text->SetPosY(posY);
	text->SetPosZ(posZ);
	text->SetWidth(width);
	text->SetHeight(LayoutItem::Height1);
	text->SetRotation(rotation);

	TextSaveAndPublishSettings(text);
	return true;
}

bool Manager::TextPosition(TextID textID,
	const LayoutPosition posX,
	const LayoutPosition posY,
	string& result)
{
	Text* text = GetText(textID);
	if (!text)
	{
		result = Languages::GetText(Languages::TextTextDoesNotExist);
		return false;
	}

	if (!CheckLayoutItemPosition(text, posX, posY, text->GetPosZ(), text->GetWidth(), LayoutItem::Height1, text->GetRotation(), result))
	{
		return false;
	}

	text->SetPosX(posX);
	text->SetPosY(posY);

	TextSaveAndPublishSettings(text);
	return true;
}

bool Manager::TextRotate(TextID textID,
	string& result)
{
	Text* text = GetText(textID);
	if (!text)
	{
		result = Languages::GetText(Languages::TextTextDoesNotExist);
		return false;
	}

	LayoutRotation newRotation = text->GetRotation();
	++newRotation;
	if (!CheckLayoutItemPosition(text, text->GetPosX(), text->GetPosY(), text->GetPosZ(), text->GetWidth(), LayoutItem::Height1, newRotation, result))
	{
		return false;
	}

	text->SetRotation(newRotation);

	TextSaveAndPublishSettings(text);
	return true;
}

void Manager::TextSaveAndPublishSettings(const Text* const text)
{
	// save in db
	if (storage)
	{
		storage->Save(*text);
	}

	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->TextSettings(text->GetID(), text->GetName());
	}
}

bool Manager::TextDelete(const TextID textID, string& result)
{
	if (textID == TextNone)
	{
		result = Languages::GetText(Languages::TextTextDoesNotExist);
		return false;
	}
	Text* text = nullptr;
	{
		std::lock_guard<std::mutex> guard(textMutex);
		if (texts.count(textID) != 1)
		{
			result = Languages::GetText(Languages::TextTextDoesNotExist);
			return false;
		}

		text = texts.at(textID);
		texts.erase(textID);
	}

	if (storage)
	{
		storage->DeleteText(textID);
	}

	const string& textName = text->GetName();
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->TextDelete(textID, textName);
	}
	delete text;
	return true;
}

/***************************
* Automode                 *
***************************/

bool Manager::LocoBaseIntoTrack(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier, const TrackID trackID)
{
	Track* track = GetTrack(trackID);
	if (!track)
	{
		return false;
	}

	LocoBase* locoBase = GetLocoBase(locoBaseIdentifier);
	if (!locoBase)
	{
		return false;
	}

	bool reserved = track->ReserveForce(logger, locoBaseIdentifier);
	if (!reserved)
	{
		return false;
	}

	reserved = locoBase->SetTrack(trackID);
	if (!reserved)
	{
		track->Release(logger, locoBaseIdentifier);
		return false;
	}

	reserved = track->Lock(logger, locoBaseIdentifier);
	if (!reserved)
	{
		locoBase->Release();
		track->Release(logger, locoBaseIdentifier);
		return false;
	}

	const string& locoName = locoBase->GetName();
	const string& trackName = track->GetName();
	logger->Info(Languages::TextLocoIsOnTrack, locoName, trackName);

	TrackPublishState(track);
	return true;
}

bool Manager::LocoRelease(const LocoID locoID)
{
	Loco* loco = GetLoco(locoID);
	if (loco == nullptr)
	{
		return false;
	}
	return LocoBaseReleaseInternal(loco);
}

bool Manager::MultipleUnitRelease(const MultipleUnitID multipleUnitID)
{
	MultipleUnit* multipleUnit = GetMultipleUnit(multipleUnitID);
	if (multipleUnit == nullptr)
	{
		return false;
	}
	return LocoBaseReleaseInternal(multipleUnit);
}

bool Manager::LocoBaseReleaseInternal(LocoBase* locoBase)
{
	LocoBaseSpeed(ControlTypeInternal, locoBase, MinSpeed);

	bool ret = locoBase->Release();
	if (ret == false)
	{
		return false;
	}
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->LocoBaseRelease(locoBase);
	}
	return true;
}

bool Manager::TrackRelease(const TrackID trackID)
{
	Track* track = GetTrack(trackID);
	if (track == nullptr)
	{
		return false;
	}
	return track->ReleaseForce(logger, ObjectIdentifier());
}

bool Manager::LocoBaseReleaseOnTrack(const TrackID trackID)
{
	Track* track = GetTrack(trackID);
	if (track == nullptr)
	{
		return false;
	}
	ObjectIdentifier locoBaseIdentifier = track->GetLocoBase();
	track->ReleaseForce(logger, locoBaseIdentifier);
	LocoBase* locoBase = GetLocoBase(locoBaseIdentifier);
	if (locoBase == nullptr)
	{
		return false;
	}
	return LocoBaseReleaseInternal(locoBase);
}

bool Manager::TrackStartLocoBase(const TrackID trackID)
{
	Track* track = GetTrack(trackID);
	if (track == nullptr)
	{
		return false;
	}
	return LocoBaseStart(track->GetLocoBase());
}

bool Manager::TrackStopLocoBase(const TrackID trackID)
{
	Track* track = GetTrack(trackID);
	if (track == nullptr)
	{
		return false;
	}
	return LocoBaseStop(track->GetLocoBase());
}

void Manager::TrackBlock(const TrackID trackID, const bool blocked)
{
	Track* track = GetTrack(trackID);
	if (track == nullptr)
	{
		return;
	}
	track->SetBlocked(blocked);
	TrackPublishState(track);
}

void Manager::TrackSetLocoOrientation(const TrackID trackID, const Orientation orientation)
{
	Track* track = GetTrack(trackID);
	if (track == nullptr)
	{
		return;
	}
	track->SetLocoOrientation(orientation);
	TrackPublishState(track);
}

void Manager::TrackPublishState(const DataModel::Track* track)
{
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->TrackState(track);
	}
}

bool Manager::RouteRelease(const RouteID routeID)
{
	Route* route = GetRoute(routeID);
	if (route == nullptr)
	{
		return false;
	}
	return route->Release(logger, route->GetLocoBase());
}

bool Manager::LocoDestinationReached(const LocoBase* loco,
	const Route* route,
	const Track* track)
{
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->LocoBaseDestinationReached(loco, route, track);
	}
	return true;
}

bool Manager::LocoBaseStart(const ObjectIdentifier& locoBaseIdentifier)
{
	LocoBase* locoBase = GetLocoBase(locoBaseIdentifier);
	if (locoBase == nullptr)
	{
		return false;
	}
	bool ret = locoBase->GoToAutoMode();
	if (ret == false)
	{
		return false;
	}
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->LocoBaseStart(locoBase);
	}
	return true;
}

bool Manager::LocoBaseStartAll()
{
	{
		std::lock_guard<std::mutex> guard(locoMutex);
		for (auto& loco : locos)
		{
			const bool started = loco.second->GoToAutoMode();
			if (!started)
			{
				continue;
			}
			Utils::Utils::SleepForMilliseconds(50);
		}
	}
	{
		std::lock_guard<std::mutex> guard(multipleUnitMutex);
		for (auto& multipleUnit : multipleUnits)
		{
			const bool started = multipleUnit.second->GoToAutoMode();
			if (!started)
			{
				continue;
			}
			Utils::Utils::SleepForMilliseconds(50);
		}
	}
	return true;
}

bool Manager::LocoBaseStop(const ObjectIdentifier& locoBaseIdentifier)
{
	LocoBase* locoBase = GetLocoBase(locoBaseIdentifier);
	if (locoBase == nullptr)
	{
		return false;
	}
	if (locoBase->IsInManualMode())
	{
		return true;
	}
	locoBase->RequestManualMode();
	while (locoBase->GoToManualMode() == false)
	{
		Utils::Utils::SleepForSeconds(1);
	}
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->LocoBaseStop(locoBase);
	}
	return true;
}

bool Manager::LocoBaseStopAll()
{
	{
		std::lock_guard<std::mutex> guard(multipleUnitMutex);
		for (auto& multipleUnit : multipleUnits)
		{
			if (multipleUnit.second->IsInManualMode())
			{
				continue;
			}
			multipleUnit.second->RequestManualMode();
		}
	}
	{
		std::lock_guard<std::mutex> guard(locoMutex);
		for (auto& loco : locos)
		{
			if (!loco.second->IsInManualMode())
			{
				continue;
			}
			loco.second->RequestManualMode();
		}
	}
	bool anyLocosInAutoMode = true;
	while (anyLocosInAutoMode && !isKillRunning())
	{
		Utils::Utils::SleepForSeconds(1);
		anyLocosInAutoMode = false;
		{
			std::lock_guard<std::mutex> guard(multipleUnitMutex);
			for (auto& multipleUnit : multipleUnits)
			{
				if (multipleUnit.second->IsInManualMode())
				{
					continue;
				}
				const bool multipleUnitInManualMode = multipleUnit.second->GoToManualMode();
				if (!multipleUnitInManualMode)
				{
					multipleUnit.second->RequestManualMode();
				}
				anyLocosInAutoMode |= !multipleUnitInManualMode;
			}
		}
		{
			std::lock_guard<std::mutex> guard(locoMutex);
			for (auto& loco : locos)
			{
				if (loco.second->IsInManualMode())
				{
					continue;
				}
				const bool locoInManualMode = loco.second->GoToManualMode();
				if (!locoInManualMode)
				{
					loco.second->RequestManualMode();
				}
				anyLocosInAutoMode |= !locoInManualMode;
			}
		}
	}

	for (auto& multipleUnit : multipleUnits)
	{
		if (multipleUnit.second->IsInManualMode())
		{
			continue;
		}
		multipleUnit.second->GetLogger()->Info(Languages::Languages::TextReleasingMultipleUnit);
		multipleUnit.second->Release();
	}
	for (auto& loco : locos)
	{
		if (loco.second->IsInManualMode())
		{
			continue;
		}
		loco.second->GetLogger()->Info(Languages::Languages::TextReleasingLoco);
		loco.second->Release();
	}
	return true;
}

void Manager::LocoBaseStopAllImmediately(const ControlType controlType)
{
	{
		std::lock_guard<std::mutex> guard(locoMutex);
		for (auto& loco : locos)
		{
			LocoBaseSpeed(controlType, loco.second, MinSpeed);
		}
	}
	{
		std::lock_guard<std::mutex> guard(multipleUnitMutex);
		for (auto& multipleUnit : multipleUnits)
		{
			LocoBaseSpeed(controlType, multipleUnit.second, MinSpeed);
		}
	}
}

bool Manager::LocoBaseAddTimeTable(const ObjectIdentifier& locoBaseIdentifier, const RouteID routeID)
{
	LocoBase* locoBase = GetLocoBase(locoBaseIdentifier);
	if (locoBase == nullptr)
	{
		return false;
	}
	Route* route = GetRoute(routeID);
	if (!route)
	{
		return false;
	}
	locoBase->AddTimeTable(route, RouteStop);
	return true;
}

string Manager::GetLokomotiveCs2() const
{
	string out("[lokomotive]\nversion\n.major=0\n.minor=3\nsession\n.id=");
	out += "1"; // FIXME: replace with mfx-neuanmeldezähler
	{
		std::lock_guard<std::mutex> guard(locoMutex);
		for (auto& loco : locos)
		{
			out += "\nlokomotive";
			out += "\n.name=" + loco.second->GetName();
			out += "\n.typ=";
			const Address address = loco.second->GetAddress();
			uint32_t uid;
			switch(loco.second->GetProtocol())
			{
				case ProtocolMM:
					out += "mm2_prog";
					uid = address;
					break;

				case ProtocolDCC:
					out += "dcc";
					uid = address + 0xC000;
					break;

				case ProtocolMFX:
					out += "mfx";
					uid = address + 0x4000;
					break;

				default:
					out += "unknown";
					uid = 0;
					break;
			}
			out += "\n.uid=0x" + Utils::Integer::IntegerToHex(uid);
			out += "\n.adresse=0x" + Utils::Integer::IntegerToHex(address);
			out += "\n.icon=";
			out += "\n.symbol=";
			out += "\n.av=0";
			out += "\n.bv=0";
			out += "\n.velocity=" + to_string(loco.second->GetSpeed());
			out += "\n.richtung=" + to_string(loco.second->GetOrientation());
			out += "\n.tachomax=120";
			out += "\n.vmax=255";
			out += "\n.vmin=1";
			for (LocoFunctionNr nr = 0; nr < 32; ++nr)
			{
				out += "\n.funktionen";
				if (nr >= 16)
				{
					out += "_2";
				}
				out += "\n..nr=" + to_string(nr);
				out += "\n..typ=" + to_string(Hardware::Protocols::MaerklinCANCommon::MapLocofunctionRailControlToCs2(nr, loco.second->GetFunctionIcon(nr)));
				out += "\n..dauer=" + to_string(loco.second->GetFunctionType(nr));
				out += "\n..wert=" + to_string(loco.second->GetFunctionState(nr));;
			}
		}
	}
	return out;
}

/***************************
* Layout                   *
***************************/

bool Manager::CheckPositionFree(const LayoutPosition posX, const LayoutPosition posY, const LayoutPosition posZ, string& result) const
{
	return CheckLayoutPositionFree(posX, posY, posZ, result, accessories, accessoryMutex)
		&& CheckLayoutPositionFree(posX, posY, posZ, result, tracks, trackMutex)
		&& CheckLayoutPositionFree(posX, posY, posZ, result, feedbacks, feedbackMutex)
		&& CheckLayoutPositionFree(posX, posY, posZ, result, switches, switchMutex)
		&& CheckLayoutPositionFree(posX, posY, posZ, result, routes, routeMutex)
		&& CheckLayoutPositionFree(posX, posY, posZ, result, signals, signalMutex)
		&& CheckLayoutPositionFree(posX, posY, posZ, result, texts, textMutex);
}

bool Manager::CheckPositionFree(const LayoutPosition posX,
	const LayoutPosition posY,
	const LayoutPosition posZ,
	const LayoutItemSize width,
	const LayoutItemSize height,
	const LayoutRotation rotation,
	string& result) const
{
	if (width == 0)
	{
		result.assign(Languages::GetText(Languages::TextWidthIs0));
		return false;
	}
	if (height == 0)
	{
		result.assign(Languages::GetText(Languages::TextHeightIs0));
		return false;
	}
	LayoutPosition x;
	LayoutPosition y;
	LayoutPosition z = posZ;
	LayoutItemSize w;
	LayoutItemSize h;
	bool ret = DataModel::LayoutItem::MapPosition(posX, posY, width, height, rotation, x, y, w, h);
	if (ret == false)
	{
		return false;
	}
	for (LayoutPosition ix = x; ix < x + w; ix++)
	{
		for (LayoutPosition iy = y; iy < y + h; iy++)
		{
			bool ret = CheckPositionFree(ix, iy, z, result);
			if (ret == false)
			{
				return false;
			}
		}
	}
	return true;
}

bool Manager::CheckLayoutItemPosition(const LayoutItem* item,
	const LayoutPosition posX,
	const LayoutPosition posY,
	const LayoutPosition posZ,
	string& result) const
{
	if (item)
	{
		if (item->IsVisibleOnLayer(posZ) == DataModel::LayoutItem::VisibleNo ||
		    item->HasPosition(posX, posY, posZ))
		{
			return true;
		}
	}
	return CheckPositionFree(posX, posY, posZ, DataModel::LayoutItem::Width1, DataModel::LayoutItem::Height1, DataModel::LayoutItem::Rotation0, result);
}

bool Manager::CheckLayoutItemPosition(const LayoutItem* item,
	const LayoutPosition posX,
	const LayoutPosition posY,
	const LayoutPosition posZ,
	const LayoutItemSize width,
	const LayoutItemSize height,
	const LayoutRotation rotation,
	string& result) const
{
	LayoutPosition x1;
	LayoutPosition y1;
	LayoutPosition z1 = posZ;
	LayoutItemSize w1;
	LayoutItemSize h1;
	bool ret = DataModel::LayoutItem::MapPosition(posX, posY, width, height, rotation, x1, y1, w1, h1);
	if (ret == false)
	{
		result = Languages::GetText(Languages::TextUnableToCalculatePosition);
		return false;
	}

	LayoutPosition x2 = 0;
	LayoutPosition y2 = 0;
	LayoutPosition z2 = 0;
	LayoutItemSize w2 = 0;
	LayoutItemSize h2 = 0;

	if (item != nullptr)
	{
		z2 = item->GetPosZ();
		ret = DataModel::LayoutItem::MapPosition(item->GetPosX(), item->GetPosY(), width, item->GetHeight(), item->GetRotation(), x2, y2, w2, h2);
		if (ret == false)
		{
			result = Languages::GetText(Languages::TextUnableToCalculatePosition);
			return false;
		}
	}

	for (LayoutPosition ix = x1; ix < x1 + w1; ++ix)
	{
		for (LayoutPosition iy = y1; iy < y1 + h1; ++iy)
		{
			ret = (ix >= x2 && ix < x2 + w2 && iy >= y2 && iy < y2 + h2 && z1 == z2);
			if (ret == true)
			{
				continue;
			}

			ret = CheckPositionFree(ix, iy, z1, result);
			if (ret == false)
			{
				return false;
			}
		}
	}
	return true;
}

template<class Type>
bool Manager::CheckLayoutPositionFree(const LayoutPosition posX, const LayoutPosition posY, const LayoutPosition posZ, string& result, const map<ObjectID, Type*>& layoutVector, std::mutex& mutex) const
{
	std::lock_guard<std::mutex> guard(mutex);
	for (auto& layout : layoutVector)
	{
		if (layout.second->CheckPositionFree(posX, posY, posZ))
		{
			continue;
		}
		result.assign(Logger::Logger::Format(Languages::GetText(Languages::TextPositionAlreadyInUse), static_cast<int>(posX), static_cast<int>(posY), static_cast<int>(posZ), layout.second->GetLayoutType(), layout.second->GetName()));
		return false;
	}
	return true;
}

bool Manager::CheckAddressLoco(const Protocol protocol, const Address address, string& result)
{
	switch (protocol)
	{
		case ProtocolDCC:
		case ProtocolDCC14:
		case ProtocolDCC28:
		case ProtocolDCC128:
			if (address > 10239)
			{
				result.assign(Languages::GetText(Languages::TextLocoAddressDccTooHigh));
				return false;
			}
			return true;

		case ProtocolMM1:
		case ProtocolMM15:
			if (address > 80)
			{
				result.assign(Languages::GetText(Languages::TextLocoAddressMm1TooHigh));
				return false;
			}
			return true;

		case ProtocolMM:
		case ProtocolMM2:
			if (address > 255)
			{
				result.assign(Languages::GetText(Languages::TextLocoAddressMm2TooHigh));
				return false;
			}
			return true;

		default:
			return true;
	}
}

bool Manager::CheckAddressAccessory(const Protocol protocol, const Address address, string& result)
{
	switch (protocol)
	{
		case ProtocolDCC:
			if (address > 2044)
			{
				result.assign(Languages::GetText(Languages::TextAccessoryAddressDccTooHigh));
				return false;
			}
			return true;

		case ProtocolMM1:
		case ProtocolMM2:
			if (address > 320) {
				result.assign(Languages::GetText(Languages::TextAccessoryAddressMmTooHigh));
				return false;
			}
			return true;

		default:
			return true;
	}
}

bool Manager::CheckControlProtocolAddress(const AddressType type, const ControlID controlID, const Protocol protocol, const Address address, string& result)
{
	if (address == AddressNone)
	{
		result.assign(Logger::Logger::Format(Languages::GetText(Languages::TextAddressMustBeHigherThen0)));
		return false;
	}

	if ((type == AddressTypeMultipleUnit) && (controlID == ControlNone))
	{
		// multiple units do not need physical control
		return true;
	}

	{
		std::lock_guard<std::mutex> guard(controlMutex);
		if (controlID < ControlIdFirstHardware || controls.count(controlID) != 1)
		{
			result.assign(Languages::GetText(Languages::TextControlDoesNotExist));
			return false;
		}
		ControlInterface* control = controls.at(controlID);
		if (!control)
		{
			result.assign(Languages::GetText(Languages::TextControlDoesNotExist));
			return false;
		}
		bool protocolSupported;
		switch (type)
		{
			case AddressTypeLoco:
				protocolSupported = control->LocoProtocolSupported(protocol);
				break;

			case AddressTypeMultipleUnit:
				protocolSupported = true;
				break;

			case AddressTypeAccessory:
				protocolSupported = control->AccessoryProtocolSupported(protocol);
				break;

			default:
				protocolSupported = false;
				break;
		}

		if (!protocolSupported)
		{
			string protocolText;
			if (protocol <= ProtocolEnd)
			{
				protocolText = ProtocolSymbols[protocol] + " ";
			}
			std::vector<Protocol> protocols;
			switch (type)
			{
				case AddressTypeLoco:
					control->LocoProtocols(protocols);
					break;

				case AddressTypeAccessory:
					control->AccessoryProtocols(protocols);
					break;

				case AddressTypeMultipleUnit:
				default:
					break;
			}

			string protocolsText;
			for (auto p : protocols)
			{
				if (protocolsText.size() > 0)
				{
					protocolsText.append(", ");
				}
				protocolsText.append(ProtocolSymbols[p]);
			}
			result.assign(Logger::Logger::Format(Languages::GetText(Languages::TextProtocolNotSupported), protocolText, protocolsText));
			return false;
		}
	} // unlock controlMutex

	switch (type)
	{
		case AddressTypeLoco:
			return CheckAddressLoco(protocol, address, result);

		case AddressTypeMultipleUnit:
			return true;

		case AddressTypeAccessory:
			return CheckAddressAccessory(protocol, address, result);

		default:
			return false;
	}
}

bool Manager::SettingsSave(const Languages::Language language,
	const DataModel::AccessoryPulseDuration duration,
	const bool autoAddFeedback,
	const bool stopOnFeedbackInFreeTrack,
	const bool executeAccessoryAlways,
	const DataModel::SelectRouteApproach selectRouteApproach,
	const DataModel::Loco::NrOfTracksToReserve nrOfTracksToReserve,
	const Logger::Logger::Level logLevel
	)
{
	Languages::SetDefaultLanguage(language);
	this->defaultAccessoryDuration = duration;
	this->autoAddFeedback = autoAddFeedback;
	this->stopOnFeedbackInFreeTrack = stopOnFeedbackInFreeTrack;
	this->executeAccessory = executeAccessoryAlways;
	this->selectRouteApproach = selectRouteApproach;
	this->nrOfTracksToReserve = nrOfTracksToReserve;
	Logger::Logger::SetLogLevel(logLevel);

	if (storage == nullptr)
	{
		return false;
	}
	Storage::TransactionGuard guard(storage);
	storage->SaveSetting("Language", std::to_string(static_cast<int>(language)));
	storage->SaveSetting("DefaultAccessoryDuration", std::to_string(duration));
	storage->SaveSetting("AutoAddFeedback", std::to_string(autoAddFeedback));
	storage->SaveSetting("StopOnFeedbackInFreeTrack", std::to_string(stopOnFeedbackInFreeTrack));
	storage->SaveSetting("ExecuteAccessory", std::to_string(executeAccessory));
	storage->SaveSetting("SelectRouteApproach", std::to_string(static_cast<int>(selectRouteApproach)));
	storage->SaveSetting("NrOfTracksToReserve", std::to_string(static_cast<int>(nrOfTracksToReserve)));
	storage->SaveSetting("LogLevel", std::to_string(static_cast<int>(logLevel)));
	return true;
}

void Manager::DebounceWorker()
{
	Utils::Utils::SetThreadName(Languages::GetText(Languages::TextDebouncer));
	logger->Info(Languages::TextDebounceThreadStarted);
	while (debounceRun)
	{
		{
			std::lock_guard<std::mutex> guard(feedbackMutex);
			for (auto& feedback : feedbacks)
			{
				feedback.second->Debounce();
			}
		}
		Utils::Utils::SleepForMilliseconds(250);
	}
	logger->Info(Languages::TextDebounceThreadTerminated);
}

template<class ID, class T>
T* Manager::CreateAndAddObject(std::map<ID,T*>& objects, std::mutex& mutex)
{
	std::lock_guard<std::mutex> Guard(mutex);
	ID newObjectID = 0;
	for (auto& object : objects)
	{
		if (object.first > newObjectID)
		{
			newObjectID = object.first;
		}
	}
	++newObjectID;
	T* newObject = new T(this, newObjectID);
	if (newObject == nullptr)
	{
		return nullptr;
	}
	objects[newObjectID] = newObject;
	return newObject;
}

ControlID Manager::GetPossibleControlForLoco() const
{
	for (auto& control : controls)
	{
		if (control.second->CanHandle(Hardware::CapabilityLoco))
		{
			return control.first;
		}
	}
	return ControlIdNone;
}

ControlID Manager::GetPossibleControlForAccessory() const
{
	for (auto& control : controls)
	{
		if (control.second->CanHandle(Hardware::CapabilityAccessory))
		{
			return control.first;
		}
	}
	return ControlIdNone;
}

ControlID Manager::GetPossibleControlForFeedback() const
{
	for (auto& control : controls)
	{
		if (control.second->CanHandle(Hardware::CapabilityFeedback))
		{
			return control.first;
		}
	}
	return ControlIdNone;
}

void Manager::ProgramCheckBooster(const ProgramMode mode)
{
	switch (mode)
	{
		case ProgramModeDccPomLoco:
		case ProgramModeDccPomAccessory:
			if (boosterState == BoosterStateGo)
			{
				return;
			}
			Booster(ControlTypeInternal, BoosterStateGo);
			Utils::Utils::SleepForMilliseconds(100);
			return;

		default:
			return;
	}
}

void Manager::ProgramRead(const ControlID controlID, const ProgramMode mode, const Address address, const CvNumber cv)
{
	ControlInterface* control = GetControl(controlID);
	if (control == nullptr)
	{
		return;
	}
	ProgramCheckBooster(mode);
	control->ProgramRead(mode, address, cv);
}

void Manager::ProgramWrite(const ControlID controlID, const ProgramMode mode, const Address address, const CvNumber cv, const CvValue value)
{
	ControlInterface* control = GetControl(controlID);
	if (control == nullptr)
	{
		return;
	}
	ProgramCheckBooster(mode);
	control->ProgramWrite(mode, address, cv, value);
}

void Manager::ProgramValue(const CvNumber cv, const CvValue value)
{
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		control.second->ProgramValue(cv, value);;
	}
}

bool Manager::CanHandle(const Hardware::Capabilities capability) const
{
	std::lock_guard<std::mutex> guard(controlMutex);
	for (auto& control : controls)
	{
		bool ret = control.second->CanHandle(capability);
		if (ret == true)
		{
			return true;
		}
	}
	return false;
}

bool Manager::CanHandle(const ControlID controlId, const Hardware::Capabilities capability) const
{
	ControlInterface* control = GetControl(controlId);
	if (control == nullptr)
	{
		return false;
	}
	return control->CanHandle(capability);
}

Hardware::Capabilities Manager::GetCapabilities(const ControlID controlID) const
{
	ControlInterface* control = GetControl(controlID);
	if (control == nullptr)
	{
		return Hardware::CapabilityNone;
	}
	return control->GetCapabilities();
}

bool Manager::LayoutItemRotate(const DataModel::ObjectIdentifier& identifier,
	string& result)
{
	ObjectType type = identifier.GetObjectType();
	ObjectID id = identifier.GetObjectID();
	switch (type)
	{
		case ObjectTypeAccessory:
			return AccessoryRotate(id, result);

		case ObjectTypeFeedback:
			return FeedbackRotate(id, result);

		case ObjectTypeSignal:
			return SignalRotate(id, result);

		case ObjectTypeSwitch:
			return SwitchRotate(id, result);

		case ObjectTypeText:
			return TextRotate(id, result);

		case ObjectTypeTrack:
			return TrackRotate(id, result);

		case ObjectTypeRoute:
		default:
			return false;
	}
}

bool Manager::LayoutItemNewPosition(const DataModel::ObjectIdentifier& identifier,
	const DataModel::LayoutItem::LayoutItemSize posX,
	const DataModel::LayoutItem::LayoutItemSize posY,
	string& result)
{
	ObjectType type = identifier.GetObjectType();
	ObjectID id = identifier.GetObjectID();
	switch (type)
	{
		case ObjectTypeAccessory:
			return AccessoryPosition(id, posX, posY, result);

		case ObjectTypeFeedback:
			return FeedbackPosition(id, posX, posY, result);

		case ObjectTypeRoute:
			return RoutePosition(id, posX, posY, result);

		case ObjectTypeSignal:
			return SignalPosition(id, posX, posY, result);

		case ObjectTypeSwitch:
			return SwitchPosition(id, posX, posY, result);

		case ObjectTypeText:
			return TextPosition(id, posX, posY, result);

		case ObjectTypeTrack:
			return TrackPosition(id, posX, posY, result);

		default:
			return false;
	}
}

ObjectIdentifier Manager::GetIdentifierOfServerLocoAddress(const Address serverAddress) const
{
	{
		std::lock_guard<std::mutex> guard(locoMutex);
		for (auto& loco : locos)
		{
			if (loco.second->GetServerAddress() == serverAddress)
			{
				return ObjectIdentifier(ObjectTypeLoco, loco.second->GetID());
			}
		}
	}

	{
		std::lock_guard<std::mutex> guard(multipleUnitMutex);
		for (auto& multipleUnit : multipleUnits)
		{
			if (multipleUnit.second->GetServerAddress() == serverAddress)
			{
				return ObjectIdentifier(ObjectTypeMultipleUnit, multipleUnit.second->GetID());
			}
		}
	}

	return ObjectIdentifier(ObjectTypeNone, ObjectNone);
}

ObjectIdentifier Manager::GetIdentifierOfServerAccessoryAddress(const Address serverAddress) const
{
	{
		std::lock_guard<std::mutex> guard(accessoryMutex);
		for (auto& accessory : accessories)
		{
			if (accessory.second->GetServerAddress() == serverAddress)
			{
				return ObjectIdentifier(ObjectTypeAccessory, accessory.second->GetID());
			}
		}
	}

	{
		std::lock_guard<std::mutex> guard(switchMutex);
		for (auto& mySwitch : switches)
		{
			if (mySwitch.second->GetServerAddress() == serverAddress)
			{
				return ObjectIdentifier(ObjectTypeSwitch, mySwitch.second->GetID());
			}
		}
	}

	{
		std::lock_guard<std::mutex> guard(signalMutex);
		for (auto& signal : signals)
		{
			if (signal.second->GetServerAddress() == serverAddress)
			{
				return ObjectIdentifier(ObjectTypeSignal, signal.second->GetID());
			}
		}
	}

	return ObjectIdentifier(ObjectTypeNone, ObjectNone);
}

Hardware::HardwareParams* Manager::CreateAndAddControl()
{
	std::lock_guard<std::mutex> Guard(hardwareMutex);
	ControlID newObjectID = ControlIdFirstHardware - 1;
	for (auto& hardwareParam : hardwareParams)
	{
		if (hardwareParam.first > newObjectID)
		{
			newObjectID = hardwareParam.first;
		}
	}
	++newObjectID;
	Hardware::HardwareParams* newParams = new Hardware::HardwareParams(this, newObjectID);
	if (newParams == nullptr)
	{
		return nullptr;
	}
	hardwareParams[newObjectID] = newParams;
	return newParams;
}

bool Manager::ObjectIsPartOfRoute(const ObjectIdentifier& identifier,
	const Object* object,
	string& result)
{
	std::lock_guard<std::mutex> Guard(routeMutex);
	for (auto& route : routes)
	{
		if (!route.second->ObjectIsPartOfRoute(identifier))
		{
			continue;
		}

		Languages::TextSelector selector;
		switch (identifier.GetObjectType())
		{
			case ObjectTypeTrack:
				selector = Languages::TextTrackIsUsedByRoute;
				break;

			case ObjectTypeAccessory:
				selector = Languages::TextAccessoryIsUsedByRoute;
				break;

			case ObjectTypeSwitch:
				selector = Languages::TextSwitchIsUsedByRoute;
				break;

			case ObjectTypeRoute:
				selector = Languages::TextRouteIsUsedByRoute;
				break;

			case ObjectTypeSignal:
				selector = Languages::TextSignalIsUsedByRoute;
				break;

			default:
				selector = Languages::TextObjectIsUsedByRoute;
				break;
		}
		result = Logger::Logger::Format(Languages::GetText(selector), object->GetName(), route.second->GetName());
		return true;
	}
	return false;
}
