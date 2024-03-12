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

#include <algorithm>

#include "DataModel/DataModel.h"
#include "DataModel/LocoConfig.h"
#include "Hardware/HardwareHandler.h"
#include "Utils/Utils.h"
#include "Server/Web/HtmlTagButton.h"
#include "Server/Web/HtmlTagInputCheckboxWithLabel.h"
#include "Server/Web/HtmlTagInputHidden.h"
#include "Server/Web/HtmlTagInputIntegerWithLabel.h"
#include "Server/Web/HtmlTagInputTextWithLabel.h"
#include "Server/Web/WebClientStatic.h"

using namespace DataModel;
using LayoutRotation = DataModel::LayoutItem::LayoutRotation;
using std::map;
using std::string;
using std::to_string;
using std::vector;

namespace Server { namespace Web
{
	HtmlTag WebClientStatic::HtmlTagControlArgument(const unsigned char argNr, const ArgumentType type, const string& value)
	{
		Languages::TextSelector argumentName;
		string argumentNumber = "arg" + to_string(argNr);
		switch (type)
		{
			case ArgumentTypeIpAddress:
				argumentName = Languages::TextIPAddress;
				break;

			case ArgumentTypeSerialPort:
			{
				argumentName = Languages::TextSerialPort;
#ifdef __CYGWIN__
				vector<unsigned char> comPorts;
				bool ret = Utils::Utils::GetComPorts(comPorts);
				if (ret == false || comPorts.size() == 0)
				{
					break;
				}
				map<string,string> comPortOptions;
				for (auto comPort : comPorts)
				{
					comPortOptions["/dev/ttyS" + to_string(comPort)] = "COM" + to_string(comPort + 1);
				}
				return HtmlTagSelectWithLabel(argumentNumber, argumentName, comPortOptions, value);
#else
				break;
#endif
			}

			case ArgumentTypeS88Modules:
			{
				argumentName = Languages::TextNrOfS88Modules;
				const int valueInteger = Utils::Utils::StringToInteger(value, 0, 62);
				return HtmlTagInputIntegerWithLabel(argumentNumber, argumentName, valueInteger, 0, 62);
			}

			default:
				return HtmlTag();
		}
		return HtmlTagInputTextWithLabel(argumentNumber, argumentName, value);
	}

	HtmlTag WebClientStatic::HtmlTagControlArguments(const HardwareType hardwareType, const string& arg1, const string& arg2, const string& arg3, const string& arg4, const string& arg5)
	{
		HtmlTag div;
		std::map<unsigned char,ArgumentType> argumentTypes;
		std::string hint;
		Hardware::HardwareHandler::ArgumentTypesOfHardwareTypeAndHint(hardwareType, argumentTypes, hint);
		if (argumentTypes.count(1) == 1)
		{
			div.AddChildTag(HtmlTagControlArgument(1, argumentTypes.at(1), arg1));
		}
		if (argumentTypes.count(2) == 1)
		{
			div.AddChildTag(HtmlTagControlArgument(2, argumentTypes.at(2), arg2));
		}
		if (argumentTypes.count(3) == 1)
		{
			div.AddChildTag(HtmlTagControlArgument(3, argumentTypes.at(3), arg3));
		}
		if (argumentTypes.count(4) == 1)
		{
			div.AddChildTag(HtmlTagControlArgument(4, argumentTypes.at(4), arg4));
		}
		if (argumentTypes.count(5) == 1)
		{
			div.AddChildTag(HtmlTagControlArgument(5, argumentTypes.at(5), arg5));
		}
		if (hint.size() > 0)
		{
			div.AddChildTag(HtmlTag("div").AddContent(Languages::GetText(Languages::TextHint)).AddContent(HtmlTag("br")).AddContent(hint));
		}
		return div;
	}

	const std::map<string,HardwareType> WebClientStatic::ListHardwareNames()
	{
		std::map<string,HardwareType> hardwareList;
		// Keys should not contain more then 30 characters
		hardwareList["CAN-Digital-Bahn CC-Schnitte"] = HardwareTypeCcSchnitte;
		hardwareList["Digikeijs DR5000"] = HardwareTypeDR5000;
		hardwareList["DCC-EX Serial"] = HardwareTypeDccPpExSerial;
		hardwareList["DCC-EX TCP"] = HardwareTypeDccPpExTcp;
		hardwareList["ESU Ecos (beta)"] = HardwareTypeEcos;
		hardwareList["Fleischmann TwinCenter"] = HardwareTypeTwinCenter;
		hardwareList["KM1 System Control 7"] = HardwareTypeSystemControl7;
		hardwareList["LDT HSI-88 RS-232"] = HardwareTypeHsi88;
		hardwareList["LokStoreDigital LoDi-Rektor (beta)"] = HardwareTypeRektor;
		hardwareList["Märklin Central Station 1 (beta)"] = HardwareTypeCS1;
		hardwareList["Märklin Central Station 2/3 TCP"] = HardwareTypeCS2Tcp;
		hardwareList["Märklin Central Station 2/3 UDP"] = HardwareTypeCS2Udp;
		hardwareList["Märklin Interface 6050/6051"] = HardwareTypeM6051;
		hardwareList["OpenDCC Z1"] = HardwareTypeOpenDcc;
		hardwareList["Roco Z21"] = HardwareTypeZ21;
		hardwareList["Tams MasterControl"] = HardwareTypeMasterControl;
		hardwareList["Tams MasterControl 2"] = HardwareTypeMasterControl2;
		hardwareList["Tams RedBox"] = HardwareTypeRedBox;
		hardwareList["Uhlenbrock Adapter 63120"] = HardwareTypeLocoNetAdapter63120;
		hardwareList["Uhlenbrock Adapter 63820"] = HardwareTypeLocoNetAdapter63820;
		hardwareList["Uhlenbrock Intellibox"] = HardwareTypeIntellibox;
		hardwareList["Uhlenbrock Intellibox II"] = HardwareTypeIntellibox2;
		hardwareList["Virtual Command Station"] = HardwareTypeVirtual;
		return hardwareList;
	}

	HtmlTag WebClientStatic::HtmlTagProtocol(const map<string,Protocol>& protocolMap, const Protocol selectedProtocol)
	{
		size_t mapSize = protocolMap.size();
		switch (mapSize)
		{
			case 0:
				return HtmlTagInputHidden("protocol", std::to_string(ProtocolNone));

			case 1:
				return HtmlTagInputHidden("protocol", std::to_string(protocolMap.begin()->second));

			default:
				HtmlTag content;
				content.AddChildTag(HtmlTagLabel(Languages::TextProtocol, "protocol"));
				content.AddChildTag(HtmlTagSelect("protocol", protocolMap, selectedProtocol));
				return content;
		}
	}

	HtmlTag WebClientStatic::HtmlTagDuration(const DataModel::AccessoryPulseDuration duration, const Languages::TextSelector label)
	{
		std::map<string,string> durationOptions;
		durationOptions["0000"] = "0";
		durationOptions["0100"] = "100";
		durationOptions["0250"] = "250";
		durationOptions["1000"] = "1000";
		return HtmlTagSelectWithLabel("duration", label, durationOptions, Utils::Utils::ToStringWithLeadingZeros(duration, 4));
	}

	HtmlTag WebClientStatic::HtmlTagSlaveEntry(const string& prefix,
		const string& priority,
		const ObjectID objectId,
		const map<string,ObjectID>& options)
	{
		HtmlTag content("div");
		content.AddId(prefix + "_priority_" + priority);
		HtmlTagButton deleteButton(Languages::TextDelete, prefix + "_delete_" + priority);
		deleteButton.AddAttribute("onclick", "deleteElement('" + prefix + "_priority_" + priority + "');return false;");
		deleteButton.AddClass("wide_button");
		content.AddChildTag(deleteButton);

		HtmlTag contentObject("div");
		contentObject.AddId(prefix + "_object_" + priority);
		contentObject.AddClass("inline-block");

		contentObject.AddChildTag(HtmlTagSelect(prefix + "_id_" + priority, options, objectId).AddClass("select_slave_id"));
		content.AddChildTag(contentObject);
		return content;
	}

	HtmlTag WebClientStatic::HtmlTagRotation(const LayoutRotation rotation)
	{
		HtmlTag content;
		std::map<LayoutRotation, Languages::TextSelector> rotationOptions;
		rotationOptions[DataModel::LayoutItem::Rotation0] = Languages::TextNoRotation;
		rotationOptions[DataModel::LayoutItem::Rotation90] = Languages::Text90DegClockwise;
		rotationOptions[DataModel::LayoutItem::Rotation180] = Languages::Text180Deg;
		rotationOptions[DataModel::LayoutItem::Rotation270] = Languages::Text90DegAntiClockwise;
		content.AddChildTag(HtmlTagSelectWithLabel("rotation", Languages::TextRotation, rotationOptions, rotation));
		content.AddChildTag(HtmlTag("p").AddContent(Languages::GetText(Languages::TextHint)).AddContent(HtmlTag("br")).AddContent(Languages::GetText(Languages::TextHintPositionRotate)));
		return content;
	}

	HtmlTag WebClientStatic::HtmlTagSelectExecuteAccessory(const bool executeAccessory)
	{
		map<unsigned char,Languages::TextSelector> options;
		options[0] = Languages::TextWhenWrongPosition;
		options[1] = Languages::TextAlways;
		return HtmlTagSelectWithLabel("executeaccessory", Languages::TextExecuteAccessory, options, static_cast<unsigned char>(executeAccessory));
	}

	HtmlTag WebClientStatic::HtmlTagSelectSelectRouteApproach(const DataModel::SelectRouteApproach selectRouteApproach, const bool addDefault)
	{
		map<DataModel::SelectRouteApproach,Languages::TextSelector> options;
		if (addDefault)
		{
			options[DataModel::SelectRouteSystemDefault] = Languages::TextSystemDefault;
		}
		options[DataModel::SelectRouteDoNotCare] = Languages::TextDoNotCare;
		options[DataModel::SelectRouteRandom] = Languages::TextRandom;
		options[DataModel::SelectRouteMinTrackLength] = Languages::TextMinTrackLength;
		options[DataModel::SelectRouteLongestUnused] = Languages::TextLongestUnused;
		return HtmlTagSelectWithLabel("selectrouteapproach", Languages::TextSelectRouteBy, options, selectRouteApproach);
	}

	HtmlTag WebClientStatic::HtmlTagNrOfTracksToReserve(const DataModel::Loco::NrOfTracksToReserve nrOfTracksToReserve)
	{
		map<DataModel::Loco::NrOfTracksToReserve,string> options;
		options[DataModel::Loco::ReserveOne] = "1";
		options[DataModel::Loco::ReserveTwo] = "2";
		return HtmlTagSelectWithLabel("nroftrackstoreserve", Languages::TextNrOfTracksToReserve, options, nrOfTracksToReserve);
	}

	HtmlTag WebClientStatic::HtmlTagLogLevel()
	{
		map<Logger::Logger::Level,Languages::TextSelector> options;
		options[Logger::Logger::LevelOff] = Languages::TextOff;
		options[Logger::Logger::LevelError] = Languages::TextError;
		options[Logger::Logger::LevelWarning] = Languages::TextWarning;
		options[Logger::Logger::LevelInfo] = Languages::TextInfo;
		options[Logger::Logger::LevelDebug] = Languages::TextDebug;
		return HtmlTagSelectWithLabel("loglevel", Languages::TextLogLevel, options, Logger::Logger::GetLogLevel());
	}

	HtmlTag WebClientStatic::HtmlTagLanguage()
	{
		map<Languages::Language,Languages::TextSelector> options;
		options[Languages::EN] = Languages::TextEnglish;
		options[Languages::DE] = Languages::TextGerman;
		options[Languages::ES] = Languages::TextSpanish;
		return HtmlTagSelectWithLabel("language", Languages::TextLanguage, options, Languages::GetDefaultLanguage());
	}

	HtmlTag WebClientStatic::HtmlTagControl(const std::map<ControlID,string>& controls, ControlID& controlId, const string& objectType, const ObjectID objectID)
	{
		if (controls.size() == 0)
		{
			return HtmlTagInputTextWithLabel("control", Languages::TextControl, Languages::GetText(Languages::TextConfigureControlFirst));
		}
		bool controlIdValid = false;
		if (controlId != ControlIdNone)
		{
			for (auto& control : controls)
			{
				if (control.first != controlId)
				{
					continue;
				}
				controlIdValid = true;
				break;
			}
		}
		if (!controlIdValid)
		{
			controlId = controls.begin()->first;
		}
		if (controls.size() == 1)
		{
			return HtmlTagInputHidden("control", to_string(controlId));
		}
		std::map<string, string> controlOptions;
		for (auto& control : controls)
		{
			controlOptions[to_string(control.first)] = control.second;
		}
		return HtmlTagSelectWithLabel("control", Languages::TextControl, controlOptions, to_string(controlId)).AddAttribute("onchange", "loadProtocol('" + objectType + "', " + to_string(objectID) + ")");
	}

	HtmlTag WebClientStatic::HtmlTagControl(const string& name, const std::map<ControlID,string>& controls)
	{
		ControlID controlIdFirst = controls.begin()->first;
		if (controls.size() == 1)
		{
			return HtmlTagInputHidden("s_" + name, to_string(controlIdFirst));
		}
		return HtmlTagSelectWithLabel(name, Languages::TextControl, controls, controlIdFirst).AddAttribute("onchange", "loadProgramModeSelector();");
	}

	vector<ObjectID> WebClientStatic::InterpretSlaveData(const string& prefix, const map<string, string>& arguments)
	{
		vector<ObjectID> ids;
		unsigned int count = Utils::Utils::GetIntegerMapEntry(arguments, prefix + "counter", 0);
		for (unsigned int index = 1; index <= count; ++index)
		{
			string indexAsString = to_string(index);
			ObjectID id = Utils::Utils::GetIntegerMapEntry(arguments, prefix + "_id_" + indexAsString, TrackNone);
			if (id == TrackNone)
			{
				continue;
			}
			ids.push_back(id);
		}

		std::sort(ids.begin(), ids.end());
		ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
		return ids;
	}

	HtmlTag WebClientStatic::HtmlTagTabMenuItem(const std::string& tabName,
		const Languages::TextSelector buttonValue,
		const bool selected,
		const bool hidden)
	{
		HtmlTag button("button");
		button.AddClass("tab_button");
		button.AddId("tab_button_" + tabName);
		button.AddAttribute("onclick", "ShowTab('" + tabName + "');");
		button.AddContent(buttonValue);
		if (selected)
		{
			button.AddClass("tab_button_selected");
		}
		if (hidden)
		{
			button.AddClass("hidden");
		}
		return button;
	}

	HtmlTag WebClientStatic::HtmlTagSelectPropulsion(const Propulsion propulsion)
	{
		map<Propulsion,Languages::TextSelector> propulsions;
		propulsions[PropulsionUnknown] = Languages::TextPropulsionUnknown;
		propulsions[PropulsionSteam] = Languages::TextPropulsionSteam;
		propulsions[PropulsionDiesel] = Languages::TextPropulsionDiesel;
		propulsions[PropulsionGas] = Languages::TextPropulsionGas;
		propulsions[PropulsionElectric] = Languages::TextPropulsionElectric;
		propulsions[PropulsionHydrogen] = Languages::TextPropulsionHydrogen;
		propulsions[PropulsionAccu] = Languages::TextPropulsionAccu;
		propulsions[PropulsionOther] = Languages::TextPropulsionOther;
		return HtmlTagSelectWithLabel("propulsion", Languages::TextPropulsion, propulsions, propulsion);
	}

	HtmlTag WebClientStatic::HtmlTagSelectTrainType(const TrainType trainType)
	{
		map<TrainType,Languages::TextSelector> trainTypes;
		trainTypes[TrainTypeUnknown] = Languages::TextTrainTypeUnknown;
		trainTypes[TrainTypeInternationalHighSpeed] = Languages::TextTrainTypeInternationalHighSpeed;
		trainTypes[TrainTypeNationalHighSpeed] = Languages::TextTrainTypeNationalHighSpeed;
		trainTypes[TrainTypeInternationalLongDistance] = Languages::TextTrainTypeInternationalLongDistance;
		trainTypes[TrainTypeNationalLongDistance] = Languages::TextTrainTypeNationalLongDistance;
		trainTypes[TrainTypeInternationalNight] = Languages::TextTrainTypeInternationalNight;
		trainTypes[TrainTypeNationalNight] = Languages::TextTrainTypeNationalNight;
		trainTypes[TrainTypeLongDistanceFastLocal] = Languages::TextTrainTypeLongDistanceFastLocal;
		trainTypes[TrainTypeFastLocal] = Languages::TextTrainTypeFastLocal;
		trainTypes[TrainTypeLocal] = Languages::TextTrainTypeLocal;
		trainTypes[TrainTypeSuburban] = Languages::TextTrainTypeSuburban;
		trainTypes[TrainTypeUnderground] = Languages::TextTrainTypeUnderground;
		trainTypes[TrainTypeHistoric] = Languages::TextTrainTypeHistoric;
		trainTypes[TrainTypeExtra] = Languages::TextTrainTypeExtra;
		trainTypes[TrainTypePassengerWithCargo] = Languages::TextTrainTypePassengerWithCargo;
		trainTypes[TrainTypeCargoLongDistance] = Languages::TextTrainTypeCargoLongDistance;
		trainTypes[TrainTypeCargoLocal] = Languages::TextTrainTypeCargoLocal;
		trainTypes[TrainTypeCargoBlock] = Languages::TextTrainTypeCargoBlock;
		trainTypes[TrainTypeCargoTractor] = Languages::TextTrainTypeCargoTractor;
		trainTypes[TrainTypeCargoExpress] = Languages::TextTrainTypeCargoExpress;
		trainTypes[TrainTypeCargoWithPassenger] = Languages::TextTrainTypeCargoWithPassenger;
		trainTypes[TrainTypeRescue] = Languages::TextTrainTypeRescue;
		trainTypes[TrainTypeConstruction] = Languages::TextTrainTypeConstruction;
		trainTypes[TrainTypeEmpty] = Languages::TextTrainTypeEmpty;
		trainTypes[TrainTypeLoco] = Languages::TextTrainTypeLoco;
		trainTypes[TrainTypeCleaning] = Languages::TextTrainTypeCleaning;
		trainTypes[TrainTypeOther] = Languages::TextTrainTypeOther;
		return HtmlTagSelectWithLabel("type", Languages::TextTrainType, trainTypes, trainType);
	}

	HtmlTag WebClientStatic::HtmlTagTabFunctions(const LocoFunctionEntry* locoFunctions)
	{
		HtmlTag functionsContent("div");
		functionsContent.AddId("tab_functions");
		functionsContent.AddClass("tab_content");
		functionsContent.AddClass("hidden");
		map<DataModel::LocoFunctionType,Languages::TextSelector> functionTypes;
		functionTypes[DataModel::LocoFunctionTypeNone] = Languages::TextLocoFunctionTypeNone;
		functionTypes[DataModel::LocoFunctionTypePermanent] = Languages::TextLocoFunctionTypePermanent;
		functionTypes[DataModel::LocoFunctionTypeMoment] = Languages::TextLocoFunctionTypeMoment;
//		functionTypes[DataModel::LocoFunctionTypeFlashing] = Languages::TextLocoFunctionTypeFlashing;
//		functionTypes[DataModel::LocoFunctionTypeTimer] = Languages::TextLocoFunctionTypeTimer;

		map<DataModel::LocoFunctionIcon,Languages::TextSelector> functionIcons;
		functionIcons[DataModel::LocoFunctionIconDefault] = Languages::TextLocoFunctionIconDefault;
		functionIcons[DataModel::LocoFunctionIconShuntingMode] = Languages::TextLocoFunctionIconShuntingMode;
		functionIcons[DataModel::LocoFunctionIconInertia] = Languages::TextLocoFunctionIconInertia;
		functionIcons[DataModel::LocoFunctionIconLight] = Languages::TextLocoFunctionIconLight;
		functionIcons[DataModel::LocoFunctionIconHeadlightLowBeamForward] = Languages::TextLocoFunctionIconHeadlightLowBeamForward;
		functionIcons[DataModel::LocoFunctionIconHeadlightLowBeamReverse] = Languages::TextLocoFunctionIconHeadlightLowBeamReverse;
		functionIcons[DataModel::LocoFunctionIconHeadlightHighBeamForward] = Languages::TextLocoFunctionIconHeadlightHighBeamForward;
		functionIcons[DataModel::LocoFunctionIconHeadlightHighBeamReverse] = Languages::TextLocoFunctionIconHeadlightHighBeamReverse;
		functionIcons[DataModel::LocoFunctionIconBacklightForward] = Languages::TextLocoFunctionIconBacklightForward;
		functionIcons[DataModel::LocoFunctionIconBacklightReverse] = Languages::TextLocoFunctionIconBacklightReverse;
		functionIcons[DataModel::LocoFunctionIconShuntingLight] = Languages::TextLocoFunctionIconShuntingLight;
		functionIcons[DataModel::LocoFunctionIconBlinkingLight] = Languages::TextLocoFunctionIconBlinkingLight;
		functionIcons[DataModel::LocoFunctionIconInteriorLight1] = Languages::TextLocoFunctionIconInteriorLight1;
		functionIcons[DataModel::LocoFunctionIconInteriorLight2] = Languages::TextLocoFunctionIconInteriorLight2;
		functionIcons[DataModel::LocoFunctionIconTableLight1] = Languages::TextLocoFunctionIconTableLight1;
		functionIcons[DataModel::LocoFunctionIconTableLight2] = Languages::TextLocoFunctionIconTableLight2;
		functionIcons[DataModel::LocoFunctionIconTableLight3] = Languages::TextLocoFunctionIconTableLight3;
		functionIcons[DataModel::LocoFunctionIconCabLight1] = Languages::TextLocoFunctionIconCabLight1;
		functionIcons[DataModel::LocoFunctionIconCabLight2] = Languages::TextLocoFunctionIconCabLight2;
		functionIcons[DataModel::LocoFunctionIconCabLight12] = Languages::TextLocoFunctionIconCabLight12;
		functionIcons[DataModel::LocoFunctionIconDriversDeskLight] = Languages::TextLocoFunctionIconDriversDeskLight;
		functionIcons[DataModel::LocoFunctionIconTrainDestinationIndicator] = Languages::TextLocoFunctionIconTrainDestinationIndicator;
		functionIcons[DataModel::LocoFunctionIconLocomotiveNumberIndicator] = Languages::TextLocoFunctionIconLocomotiveNumberIndicator;
		functionIcons[DataModel::LocoFunctionIconEngineLight] = Languages::TextLocoFunctionIconEngineLight;
		functionIcons[DataModel::LocoFunctionIconFireBox] = Languages::TextLocoFunctionIconFireBox;
		functionIcons[DataModel::LocoFunctionIconStairsLight] = Languages::TextLocoFunctionIconStairsLight;
		functionIcons[DataModel::LocoFunctionIconSmokeGenerator] = Languages::TextLocoFunctionIconSmokeGenerator;
		functionIcons[DataModel::LocoFunctionIconTelex1] = Languages::TextLocoFunctionIconTelex1;
		functionIcons[DataModel::LocoFunctionIconTelex2] = Languages::TextLocoFunctionIconTelex2;
		functionIcons[DataModel::LocoFunctionIconTelex12] = Languages::TextLocoFunctionIconTelex12;
		functionIcons[DataModel::LocoFunctionIconPanto1] = Languages::TextLocoFunctionIconPanto1;
		functionIcons[DataModel::LocoFunctionIconPanto2] = Languages::TextLocoFunctionIconPanto2;
		functionIcons[DataModel::LocoFunctionIconPanto12] = Languages::TextLocoFunctionIconPanto12;
		functionIcons[DataModel::LocoFunctionIconUp] = Languages::TextLocoFunctionIconUp;
		functionIcons[DataModel::LocoFunctionIconDown] = Languages::TextLocoFunctionIconDown;
		functionIcons[DataModel::LocoFunctionIconUpDown1] = Languages::TextLocoFunctionIconUpDown1;
		functionIcons[DataModel::LocoFunctionIconUpDown2] = Languages::TextLocoFunctionIconUpDown2;
		functionIcons[DataModel::LocoFunctionIconLeft] = Languages::TextLocoFunctionIconLeft;
		functionIcons[DataModel::LocoFunctionIconRight] = Languages::TextLocoFunctionIconRight;
		functionIcons[DataModel::LocoFunctionIconLeftRight] = Languages::TextLocoFunctionIconLeftRight;
		functionIcons[DataModel::LocoFunctionIconTurnLeft] = Languages::TextLocoFunctionIconTurnLeft;
		functionIcons[DataModel::LocoFunctionIconTurnRight] = Languages::TextLocoFunctionIconTurnRight;
		functionIcons[DataModel::LocoFunctionIconTurn] = Languages::TextLocoFunctionIconTurn;
		functionIcons[DataModel::LocoFunctionIconCrane] = Languages::TextLocoFunctionIconCrane;
		functionIcons[DataModel::LocoFunctionIconMagnet] = Languages::TextLocoFunctionIconMagnet;
		functionIcons[DataModel::LocoFunctionIconCraneHook] = Languages::TextLocoFunctionIconCraneHook;
		functionIcons[DataModel::LocoFunctionIconFan] = Languages::TextLocoFunctionIconFan;
		functionIcons[DataModel::LocoFunctionIconBreak] = Languages::TextLocoFunctionIconBreak;
		functionIcons[DataModel::LocoFunctionIconNoSound] = Languages::TextLocoFunctionIconNoSound;
		functionIcons[DataModel::LocoFunctionIconSoundGeneral] = Languages::TextLocoFunctionIconSoundGeneral;
		functionIcons[DataModel::LocoFunctionIconRunning1] = Languages::TextLocoFunctionIconRunning1;
		functionIcons[DataModel::LocoFunctionIconRunning2] = Languages::TextLocoFunctionIconRunning2;
		functionIcons[DataModel::LocoFunctionIconEngine1] = Languages::TextLocoFunctionIconEngine1;
		functionIcons[DataModel::LocoFunctionIconEngine2] = Languages::TextLocoFunctionIconEngine2;
		functionIcons[DataModel::LocoFunctionIconBreak1] = Languages::TextLocoFunctionIconBreak1;
		functionIcons[DataModel::LocoFunctionIconBreak2] = Languages::TextLocoFunctionIconBreak2;
		functionIcons[DataModel::LocoFunctionIconCurve] = Languages::TextLocoFunctionIconCurve;
		functionIcons[DataModel::LocoFunctionIconHorn1] = Languages::TextLocoFunctionIconHorn1;
		functionIcons[DataModel::LocoFunctionIconHorn2] = Languages::TextLocoFunctionIconHorn2;
		functionIcons[DataModel::LocoFunctionIconWhistle1] = Languages::TextLocoFunctionIconWhistle1;
		functionIcons[DataModel::LocoFunctionIconWhistle2] = Languages::TextLocoFunctionIconWhistle2;
		functionIcons[DataModel::LocoFunctionIconBell] = Languages::TextLocoFunctionIconBell;
		functionIcons[DataModel::LocoFunctionIconStationAnnouncement1] = Languages::TextLocoFunctionIconStationAnnouncement1;
		functionIcons[DataModel::LocoFunctionIconStationAnnouncement2] = Languages::TextLocoFunctionIconStationAnnouncement2;
		functionIcons[DataModel::LocoFunctionIconStationAnnouncement3] = Languages::TextLocoFunctionIconStationAnnouncement3;
		functionIcons[DataModel::LocoFunctionIconSpeak] = Languages::TextLocoFunctionIconSpeak;
		functionIcons[DataModel::LocoFunctionIconRadio] = Languages::TextLocoFunctionIconRadio;
		functionIcons[DataModel::LocoFunctionIconMusic1] = Languages::TextLocoFunctionIconMusic1;
		functionIcons[DataModel::LocoFunctionIconMusic2] = Languages::TextLocoFunctionIconMusic2;
		functionIcons[DataModel::LocoFunctionIconOpenDoor] = Languages::TextLocoFunctionIconOpenDoor;
		functionIcons[DataModel::LocoFunctionIconCloseDoor] = Languages::TextLocoFunctionIconCloseDoor;
		functionIcons[DataModel::LocoFunctionIconFan1] = Languages::TextLocoFunctionIconFan1;
		functionIcons[DataModel::LocoFunctionIconFan2] = Languages::TextLocoFunctionIconFan2;
		functionIcons[DataModel::LocoFunctionIconFan3] = Languages::TextLocoFunctionIconFan3;
		functionIcons[DataModel::LocoFunctionIconShovelCoal] = Languages::TextLocoFunctionIconShovelCoal;
		functionIcons[DataModel::LocoFunctionIconCompressedAir] = Languages::TextLocoFunctionIconCompressedAir;
		functionIcons[DataModel::LocoFunctionIconReliefValve] = Languages::TextLocoFunctionIconReliefValve;
		functionIcons[DataModel::LocoFunctionIconSteamBlowOut] = Languages::TextLocoFunctionIconSteamBlowOut;
		functionIcons[DataModel::LocoFunctionIconSteamBlow] = Languages::TextLocoFunctionIconSteamBlow;
		functionIcons[DataModel::LocoFunctionIconDrainValve] = Languages::TextLocoFunctionIconDrainValve;
		functionIcons[DataModel::LocoFunctionIconShakingRust] = Languages::TextLocoFunctionIconShakingRust;
		functionIcons[DataModel::LocoFunctionIconAirPump] = Languages::TextLocoFunctionIconAirPump;
		functionIcons[DataModel::LocoFunctionIconWaterPump] = Languages::TextLocoFunctionIconWaterPump;
		functionIcons[DataModel::LocoFunctionIconBufferPush] = Languages::TextLocoFunctionIconBufferPush;
		functionIcons[DataModel::LocoFunctionIconGenerator] = Languages::TextLocoFunctionIconGenerator;
		functionIcons[DataModel::LocoFunctionIconGearBox] = Languages::TextLocoFunctionIconGearBox;
		functionIcons[DataModel::LocoFunctionIconGearUp] = Languages::TextLocoFunctionIconGearUp;
		functionIcons[DataModel::LocoFunctionIconGearDown] = Languages::TextLocoFunctionIconGearDown;
		functionIcons[DataModel::LocoFunctionIconFillWater] = Languages::TextLocoFunctionIconFillWater;
		functionIcons[DataModel::LocoFunctionIconFillDiesel] = Languages::TextLocoFunctionIconFillDiesel;
		functionIcons[DataModel::LocoFunctionIconFillGas] = Languages::TextLocoFunctionIconFillGas;
		functionIcons[DataModel::LocoFunctionIconSand] = Languages::TextLocoFunctionIconSand;
		functionIcons[DataModel::LocoFunctionIconRailJoint] = Languages::TextLocoFunctionIconRailJoint;
		functionIcons[DataModel::LocoFunctionIconCoupler] = Languages::TextLocoFunctionIconCoupler;
		functionIcons[DataModel::LocoFunctionIconPanto] = Languages::TextLocoFunctionIconPanto;
		functionIcons[DataModel::LocoFunctionIconMainSwitch] = Languages::TextLocoFunctionIconMainSwitch;
		functionIcons[DataModel::LocoFunctionIconSoundLouder] = Languages::TextLocoFunctionIconSoundLouder;
		functionIcons[DataModel::LocoFunctionIconSoundLower] = Languages::TextLocoFunctionIconSoundLower;
		functionIcons[DataModel::LocoFunctionIconNoBreak] = Languages::TextLocoFunctionIconNoBreak;
		for (unsigned int nr = 0; nr < NumberOfLocoFunctions; ++nr)
		{
			HtmlTag fDiv("div");
			fDiv.AddClass("function_line");
			string nrString = to_string(nr);
			string fNrString = "f" + nrString;
			fDiv.AddChildTag(HtmlTagLabel("F" + nrString, fNrString + "_type"));

			const DataModel::LocoFunctionType functionType = locoFunctions[nr].type;
			DataModel::LocoFunctionIcon icon = locoFunctions[nr].icon;
			DataModel::LocoFunctionTimer timer = locoFunctions[nr].timer;

			fDiv.AddChildTag(HtmlTagSelect(fNrString + "_type", functionTypes, functionType).AddAttribute("onchange", "onChangeLocoFunctionType(" + nrString + ");return false;"));
			HtmlTagSelect selectIcon(fNrString + "_icon", functionIcons, icon);
			HtmlTagInputInteger inputTimer(fNrString + "_timer", timer, 1, 255);
			if (functionType == LocoFunctionTypeNone)
			{
				selectIcon.AddClass("hidden");
			}
			if (functionType != LocoFunctionTypeTimer)
			{
				inputTimer.AddClass("hidden");
			}
			inputTimer.AddClass("function_line_integer");

			fDiv.AddChildTag(selectIcon);
			fDiv.AddChildTag(inputTimer);
			functionsContent.AddChildTag(fDiv);
		}
		return functionsContent;
	}

	HtmlTag WebClientStatic::HtmlTagTabAutomode(const bool pushpull,
		const Speed maxSpeed,
		const Speed travelSpeed,
		const Speed reducedSpeed,
		const Speed creepingSpeed)
	{
		HtmlTag automodeContent("div");
		automodeContent.AddId("tab_automode");
		automodeContent.AddClass("tab_content");
		automodeContent.AddClass("hidden");
		automodeContent.AddChildTag(HtmlTagInputCheckboxWithLabel("pushpull", Languages::TextPushPullTrain, "pushpull", pushpull));
		automodeContent.AddChildTag(HtmlTagInputIntegerWithLabel("maxspeed", Languages::TextMaxSpeed, maxSpeed, 0, MaxSpeed));
		automodeContent.AddChildTag(HtmlTagInputIntegerWithLabel("travelspeed", Languages::TextTravelSpeed, travelSpeed, 0, MaxSpeed));
		automodeContent.AddChildTag(HtmlTagInputIntegerWithLabel("reducedspeed", Languages::TextReducedSpeed, reducedSpeed, 0, MaxSpeed));
		automodeContent.AddChildTag(HtmlTagInputIntegerWithLabel("creepingspeed", Languages::TextCreepingSpeed, creepingSpeed, 0, MaxSpeed));
		return automodeContent;
	}

	vector<DataModel::Relation*> WebClientStatic::ConvertSlaveIDVectorToRelation(Manager& manager,
		const MultipleUnitID multipleUnitID,
		const vector<LocoID>& slaveIDs)
	{
		vector<DataModel::Relation*> slaves;
		for (auto const & slaveID : slaveIDs)
		{
			slaves.push_back(new Relation(&manager,
				ObjectIdentifier(ObjectTypeMultipleUnit, multipleUnitID),
				ObjectIdentifier(ObjectTypeLoco, slaveID),
				Relation::TypeMultipleUnitSlave));
		}
		return slaves;
	}
}} // namespace Server::Web
