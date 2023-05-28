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

#include "DataModel/Loco.h"
#include "DataModel/LocoFunctions.h"
#include "DataTypes.h"
#include "Logger/Logger.h"
#include "Hardware/CS1.h"
#include "Hardware/CS2Udp.h"
#include "Hardware/CS2Tcp.h"
#include "Hardware/CcSchnitte.h"
#include "Hardware/DR5000.h"
#include "Hardware/DccPpExSerial.h"
#include "Hardware/DccPpExTcp.h"
#include "Hardware/Ecos.h"
#include "Hardware/HardwareHandler.h"
#include "Hardware/Hsi88.h"
#include "Hardware/Intellibox.h"
#include "Hardware/Intellibox2.h"
#include "Hardware/LocoNetAdapter63120.h"
#include "Hardware/LocoNetAdapter63820.h"
#include "Hardware/M6051.h"
#include "Hardware/MasterControl.h"
#include "Hardware/MasterControl2.h"
#include "Hardware/OpenDcc.h"
#include "Hardware/RedBox.h"
#include "Hardware/Rektor.h"
#include "Hardware/SystemControl7.h"
#include "Hardware/TwinCenter.h"
#include "Hardware/Virtual.h"
#include "Hardware/Z21.h"
#include "Utils/Utils.h"

using std::string;

namespace Hardware
{
	const std::string HardwareHandler::Unknown = Languages::GetText(Languages::TextUnknownHardware);

	void HardwareHandler::Init(const HardwareParams* params)
	{
		this->params = params;
		HardwareType type = params->GetHardwareType();

		switch(type)
		{
			case HardwareTypeNone:
				instance = nullptr;
				return;

			case HardwareTypeVirtual:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new Virtual(params));
				return;

			case HardwareTypeCS2Udp:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new CS2Udp(params));
				return;

			case HardwareTypeM6051:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new M6051(params));
				return;

			case HardwareTypeOpenDcc:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new OpenDcc(params));
				return;

			case HardwareTypeHsi88:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new Hsi88(params));
				return;

			case HardwareTypeZ21:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new Z21(params));
				return;

			case HardwareTypeCcSchnitte:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new CcSchnitte(params));
				return;

			case HardwareTypeEcos:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new Ecos(params));
				return;

			case HardwareTypeCS2Tcp:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new CS2Tcp(params));
				return;

			case HardwareTypeIntellibox:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new Intellibox(params));
				break;

			case HardwareTypeMasterControl:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new MasterControl(params));
				break;

			case HardwareTypeTwinCenter:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new TwinCenter(params));
				break;

			case HardwareTypeMasterControl2:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new MasterControl2(params));
				break;

			case HardwareTypeRedBox:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new RedBox(params));
				break;

			case HardwareTypeRektor:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new Rektor(params));
				return;

			case HardwareTypeDR5000:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new DR5000(params));
				return;

			case HardwareTypeCS1:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new CS1(params));
				return;

			case HardwareTypeDccPpExTcp:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new DccPpExTcp(params));
				return;

			case HardwareTypeDccPpExSerial:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new DccPpExSerial(params));
				return;

			case HardwareTypeIntellibox2:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new Intellibox2(params));
				break;

			case HardwareTypeLocoNetAdapter63120:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new LocoNetAdapter63120(params));
				break;

			case HardwareTypeLocoNetAdapter63820:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new LocoNetAdapter63820(params));
				break;

			case HardwareTypeSystemControl7:
				instance = reinterpret_cast<Hardware::HardwareInterface*>(new SystemControl7(params));
				break;
		}
	}

	void HardwareHandler::Close()
	{
		delete(instance);
		instance = nullptr;
		params = nullptr;
	}

	const std::string& HardwareHandler::GetName() const
	{
		if (instance == nullptr)
		{
			return Unknown;
		}
		return instance->GetFullName();
	}

	const std::string& HardwareHandler::GetShortName() const
	{
		if (instance == nullptr)
		{
			return Unknown;
		}
		return instance->GetShortName();
	}

	Hardware::Capabilities HardwareHandler::GetCapabilities() const
	{
		if (instance == nullptr)
		{
			return Hardware::CapabilityNone;
		}

		return instance->GetCapabilities();
	}

	void HardwareHandler::LocoProtocols(std::vector<Protocol>& protocols) const
	{
		if (instance == nullptr)
		{
			protocols.push_back(ProtocolNone);
			return;
		}

		instance->GetLocoProtocols(protocols);
	}

	bool HardwareHandler::LocoProtocolSupported(Protocol protocol) const
	{
		if (instance == nullptr)
		{
			return false;
		}
		return instance->LocoProtocolSupported(protocol);
	}

	void HardwareHandler::AccessoryProtocols(std::vector<Protocol>& protocols) const
	{
		if (instance == nullptr)
		{
			protocols.push_back(ProtocolNone);
			return;
		}

		instance->GetAccessoryProtocols(protocols);
	}

	bool HardwareHandler::AccessoryProtocolSupported(Protocol protocol) const
	{
		if (instance == nullptr)
		{
			return false;
		}
		return instance->AccessoryProtocolSupported(protocol);
	}

	void HardwareHandler::Booster(const ControlType controlType, const BoosterState status)
	{
		if (controlType == ControlTypeHardware || instance == nullptr)
		{
			return;
		}
		instance->Booster(status);
	}

	void HardwareHandler::LocoBaseSpeed(const ControlType controlType,
		const DataModel::LocoBase* loco,
		const Speed speed)
	{
		if (controlType == ControlTypeHardware || instance == nullptr || loco->GetControlID() != GetControlID())
		{
			return;
		}
		instance->LocoSpeed(loco->GetProtocol(), loco->GetAddress(), speed);
	}

	void HardwareHandler::LocoBaseOrientation(const ControlType controlType,
		const DataModel::LocoBase* loco,
		const Orientation orientation)
	{
		if (controlType == ControlTypeHardware || instance == nullptr || loco->GetControlID() != GetControlID())
		{
			return;
		}
		instance->LocoOrientation(loco->GetProtocol(), loco->GetAddress(), orientation);
	}

	void HardwareHandler::LocoBaseFunction(const ControlType controlType,
		const DataModel::LocoBase* loco,
		const DataModel::LocoFunctionNr function,
		const DataModel::LocoFunctionState on)
	{
		if (controlType == ControlTypeHardware || instance == nullptr || loco->GetControlID() != GetControlID())
		{
			return;
		}
		instance->LocoFunction(loco->GetProtocol(), loco->GetAddress(), function, on);
	}

	void HardwareHandler::LocoSpeedOrientationFunctions(const DataModel::LocoBase* loco,
		const Speed speed,
		const Orientation orientation,
		std::vector<DataModel::LocoFunctionEntry>& functions)
	{
		if (instance == nullptr || loco->GetControlID() != GetControlID())
		{
			return;
		}
		instance->LocoSpeedOrientationFunctions(loco->GetProtocol(), loco->GetAddress(), speed, orientation, functions);
	}

	void HardwareHandler::LocoSettings(const LocoID locoId,
		__attribute__((unused)) const std::string& name,
		const std::string& matchKey)
	{
		if (instance == nullptr)
		{
			return;
		}
		instance->SetLocoIdOfMatchKey(locoId, matchKey);
	}

	void HardwareHandler::LocoDelete(__attribute__((unused)) const LocoID locoID,
		__attribute__((unused)) const std::string& name,
		const std::string& matchKey)
	{
		if (instance == nullptr)
		{
			return;
		}
		instance->SetLocoIdOfMatchKey(LocoNone, matchKey);
	}

	void HardwareHandler::AccessorySettings(const AccessoryID accessoryId,
		__attribute__((unused)) const std::string& name,
		const std::string& matchKey)
	{
		if (instance == nullptr)
		{
			return;
		}
		instance->SetAccessoryIdentifierOfMatchKey(DataModel::ObjectIdentifier(ObjectTypeAccessory, accessoryId), matchKey);
	}

	void HardwareHandler::AccessoryDelete(__attribute__((unused)) const AccessoryID accessoryId,
		__attribute__((unused)) const std::string& name,
		const std::string& matchKey)
	{
		if (instance == nullptr)
		{
			return;
		}
		instance->SetAccessoryIdentifierOfMatchKey(DataModel::ObjectIdentifier(ObjectTypeNone, ObjectNone), matchKey);
	}

	void HardwareHandler::AccessoryState(const ControlType controlType, const DataModel::Accessory* accessory)
	{
		if (controlType == ControlTypeHardware
			|| instance == nullptr
			|| accessory == nullptr
			|| accessory->GetControlID() != this->GetControlID())
		{
			return;
		}
		instance->Accessory(accessory->GetProtocol(), accessory->GetAddress(), accessory->GetInvertedAccessoryState(), accessory->GetAccessoryPulseDuration());
	}

	void HardwareHandler::SwitchSettings(const SwitchID switchId,
		__attribute__((unused)) const std::string& name,
		const std::string& matchKey)
	{
		if (instance == nullptr)
		{
			return;
		}
		instance->SetAccessoryIdentifierOfMatchKey(DataModel::ObjectIdentifier(ObjectTypeSwitch, switchId), matchKey);
	}

	void HardwareHandler::SwitchDelete(__attribute__((unused)) const SwitchID switchId,
		__attribute__((unused)) const std::string& name,
		const std::string& matchKey)
	{
		if (instance == nullptr)
		{
			return;
		}
		instance->SetAccessoryIdentifierOfMatchKey(DataModel::ObjectIdentifier(ObjectTypeNone, ObjectNone), matchKey);
	}

	void HardwareHandler::SwitchState(const ControlType controlType, const DataModel::Switch* mySwitch)
	{
		if (controlType == ControlTypeHardware
			|| instance == nullptr
			|| mySwitch == nullptr
			|| mySwitch->GetControlID() != this->GetControlID())
		{
			return;
		}

		Protocol protocol =  mySwitch->GetProtocol();
		Address address =  mySwitch->GetAddress();
		DataModel::AccessoryPulseDuration duration = mySwitch->GetAccessoryPulseDuration();
		if (mySwitch->GetType() != DataModel::SwitchTypeThreeWay)
		{
			instance->Accessory(protocol, address, mySwitch->GetInvertedAccessoryState(), duration);
			return;
		}

		switch (mySwitch->GetAccessoryState())
		{
			case DataModel::SwitchStateTurnout:
				instance->Accessory(protocol, address + 1, mySwitch->CalculateInvertedAccessoryState(DataModel::SwitchStateStraight), duration);
				instance->Accessory(protocol, address, mySwitch->CalculateInvertedAccessoryState(DataModel::SwitchStateTurnout), duration);
				break;

			case DataModel::SwitchStateStraight:
				instance->Accessory(protocol, address, mySwitch->CalculateInvertedAccessoryState(DataModel::SwitchStateStraight), duration);
				instance->Accessory(protocol, address + 1, mySwitch->CalculateInvertedAccessoryState(DataModel::SwitchStateStraight), duration);
				break;

			case DataModel::SwitchStateThird:
				instance->Accessory(protocol, address, mySwitch->CalculateInvertedAccessoryState(DataModel::SwitchStateStraight), duration);
				instance->Accessory(protocol, address + 1, mySwitch->CalculateInvertedAccessoryState(DataModel::SwitchStateTurnout), duration);
				break;

			default:
				break;
		}
	}

	void HardwareHandler::SignalSettings(const SignalID signalId,
		__attribute__((unused)) const std::string& name,
		const std::string& matchKey)
	{
		if (instance == nullptr)
		{
			return;
		}
		instance->SetAccessoryIdentifierOfMatchKey(DataModel::ObjectIdentifier(ObjectTypeSignal, signalId), matchKey);
	}

	void HardwareHandler::SignalDelete(__attribute__((unused)) const SignalID signalId,
		__attribute__((unused)) const std::string& name,
		const std::string& matchKey)
	{
		if (instance == nullptr)
		{
			return;
		}
		instance->SetAccessoryIdentifierOfMatchKey(DataModel::ObjectIdentifier(ObjectTypeNone, ObjectNone), matchKey);
	}

	void HardwareHandler::SignalState(const ControlType controlType, const DataModel::Signal* signal)
	{
		if (controlType == ControlTypeHardware
			|| instance == nullptr
			|| signal == nullptr
			|| signal->GetControlID() != this->GetControlID())
		{
			return;
		}
		const Address address = signal->GetMappedAddress();
		if (address == 0)
		{
			return;
		}
		const DataModel::AccessoryState state = signal->GetMappedAccessoryState();
		instance->Accessory(signal->GetProtocol(), address, state, signal->GetAccessoryPulseDuration());
	}

	bool HardwareHandler::ProgramCheckValues(const ProgramMode mode, const CvNumber cv, const CvValue value)
	{
		if (cv == 0)
		{
			// CVs are one based, so CV zero is not allowed
			return false;
		}
		if (cv == 1 && value == 0)
		{
			// loco/accessory address zero is not allowed for DCC decoders and can not be undone.
			// It would destroy some DCC decoders. Therefore we do not allow this.
			return false;
		}
		CvNumber maxCv;
		switch (mode)
		{
			case ProgramModeMm:
			case ProgramModeMmPom:
				maxCv = 0x100;
				break;

			case ProgramModeDccRegister:
			case ProgramModeDccPage:
			case ProgramModeDccDirect:
			case ProgramModeDccPomLoco:
			case ProgramModeDccPomAccessory:
				maxCv = 0x400;
				break;

			case ProgramModeMfx:
				maxCv = 0x4000;
				break;

			default:
				return false;
		}
		return (cv <= maxCv);
	}

	void HardwareHandler::ProgramRead(const ProgramMode mode, const Address address, const CvNumber cv)
	{
		if (ProgramCheckValues(mode, cv) == false)
		{
			return;
		}
		if (instance == nullptr)
		{
			return;
		}

		instance->ProgramRead(mode, address, cv);
	}

	void HardwareHandler::ProgramWrite(const ProgramMode mode, const Address address, const CvNumber cv, const CvValue value)
	{
		if (ProgramCheckValues(mode, cv, value) == false)
		{
			return;
		}
		if (instance == nullptr)
		{
			return;
		}
		instance->ProgramWrite(mode, address, cv, value);
	}

	void HardwareHandler::FeedbackDelete(const FeedbackID feedbackID, const std::string& name)
	{
		if (instance == nullptr)
		{
			return;
		}
		instance->FeedbackDelete(feedbackID, name);
	}

	void HardwareHandler::FeedbackSettings(const FeedbackID feedbackID, const std::string& name)
	{
		if (instance == nullptr)
		{
			return;
		}
		instance->FeedbackSettings(feedbackID, name);
	}

	void HardwareHandler::AddUnmatchedLocos(std::map<std::string,DataModel::LocoConfig>& list) const
	{
		if (instance == nullptr)
		{
			return;
		}

		const std::map<std::string,Hardware::LocoCacheEntry>& database = instance->GetLocoDatabase();
		for (auto& entry : database)
		{
			const Hardware::LocoCacheEntry& loco = entry.second;
			if (loco.GetLocoID() != LocoNone)
			{
				continue;
			}
			if (loco.GetType() != LocoTypeLoco)
			{
				continue;
			}
			list[loco.GetName() + " (" + instance->GetShortName() + ")"] = loco;
		}
	}

	std::map<std::string,DataModel::LocoConfig> HardwareHandler::GetUnmatchedLocos(const std::string& matchKey) const
	{
		std::map<std::string,DataModel::LocoConfig> out;
		if (instance == nullptr)
		{
			return out;
		}

		const std::map<std::string,Hardware::LocoCacheEntry>& database = instance->GetLocoDatabase();
		for (auto& entry : database)
		{
			const Hardware::LocoCacheEntry& loco = entry.second;
			if (loco.GetLocoID() != LocoNone && loco.GetMatchKey().size() && loco.GetMatchKey() != matchKey)
			{
				continue;
			}
			out[loco.GetName()] = loco;
		}
		return out;
	}

	DataModel::LocoConfig HardwareHandler::GetLocoByMatchKey(const std::string& match) const
	{
		if (instance == nullptr)
		{
			return DataModel::LocoConfig(LocoTypeLoco);
		}
		return instance->GetLocoByMatchKey(match);
	}

	void HardwareHandler::AddUnmatchedAccessories(std::map<std::string,DataModel::AccessoryConfig>& list) const
	{
		if (instance == nullptr)
		{
			return;
		}

		const std::map<std::string,Hardware::AccessoryCacheEntry>& database = instance->GetAccessoryDatabase();
		for (auto& entry : database)
		{
			const Hardware::AccessoryCacheEntry& accessory = entry.second;
			const DataModel::ObjectIdentifier objectIdentifier = accessory.GetObjectIdentifier();
			if (objectIdentifier.GetObjectType() != ObjectTypeNone)
			{
				continue;
			}
			list[accessory.GetName() + " (" + instance->GetShortName() + ")"] = accessory;
		}
	}

	std::map<std::string,DataModel::AccessoryConfig> HardwareHandler::GetUnmatchedAccessories(const std::string& matchKey) const
	{
		std::map<std::string,DataModel::AccessoryConfig> out;
		if (instance == nullptr)
		{
			return out;
		}

		const std::map<std::string,Hardware::AccessoryCacheEntry>& database = instance->GetAccessoryDatabase();
		for (auto& entry : database)
		{
			const Hardware::AccessoryCacheEntry& accessory = entry.second;
			if (accessory.GetObjectIdentifier().GetObjectID() != ObjectNone
				&& accessory.GetMatchKey().size()
				&& accessory.GetMatchKey() != matchKey)
			{
				continue;
			}
			out[accessory.GetName()] = accessory;
		}
		return out;
	}

	DataModel::AccessoryConfig HardwareHandler::GetAccessoryByMatchKey(const std::string& match) const
	{
		if (instance == nullptr)
		{
			return DataModel::AccessoryConfig();
		}
		return instance->GetAccessoryByMatchKey(match);
	}

	void HardwareHandler::AddUnmatchedFeedbacks(std::map<std::string,DataModel::FeedbackConfig>& list) const
	{
		if (instance == nullptr)
		{
			return;
		}

		const std::map<std::string,Hardware::FeedbackCacheEntry>& database = instance->GetFeedbackDatabase();
		for (auto& entry : database)
		{
			const Hardware::FeedbackCacheEntry& feedback = entry.second;
			if (feedback.GetFeedbackID() != FeedbackNone)
			{
				continue;
			}
			list[feedback.GetName() + " (" + instance->GetShortName() + ")"] = feedback;
		}
	}

	std::map<std::string,DataModel::FeedbackConfig> HardwareHandler::GetUnmatchedFeedbacks(const std::string& matchKey) const
	{
		std::map<std::string,DataModel::FeedbackConfig> out;
		if (instance == nullptr)
		{
			return out;
		}

		const std::map<std::string,Hardware::FeedbackCacheEntry>& database = instance->GetFeedbackDatabase();
		for (auto& entry : database)
		{
			const Hardware::FeedbackCacheEntry& feedback = entry.second;
			if (feedback.GetFeedbackID() != FeedbackNone && feedback.GetMatchKey().size() && feedback.GetMatchKey() != matchKey)
			{
				continue;
			}
			out[feedback.GetName()] = feedback;
		}
		return out;
	}

	DataModel::FeedbackConfig HardwareHandler::GetFeedbackByMatchKey(const std::string& match) const
	{
		if (instance == nullptr)
		{
			return DataModel::FeedbackConfig();
		}
		return instance->GetFeedbackByMatchKey(match);
	}

	void HardwareHandler::ArgumentTypesOfHardwareTypeAndHint(const HardwareType hardwareType, std::map<unsigned char,ArgumentType>& arguments, std::string& hint)
	{
		switch (hardwareType)
		{
			case HardwareTypeNone:
				hint = "";
				return;

			case HardwareTypeVirtual:
				Hardware::Virtual::GetHint(hint);
				return;

			case HardwareTypeCS2Udp:
				Hardware::CS2Udp::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeM6051:
				Hardware::M6051::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeOpenDcc:
				Hardware::OpenDcc::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeHsi88:
				Hardware::Hsi88::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeDR5000:
			case HardwareTypeRektor:
			case HardwareTypeZ21:
				Hardware::Protocols::Z21::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeCcSchnitte:
				Hardware::CcSchnitte::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeEcos:
				Hardware::Protocols::EsuCAN::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeCS2Tcp:
				Hardware::CS2Tcp::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeIntellibox:
				Hardware::Intellibox::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeMasterControl:
				Hardware::MasterControl::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeTwinCenter:
				Hardware::TwinCenter::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeMasterControl2:
				Hardware::MasterControl2::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeRedBox:
				Hardware::RedBox::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeCS1:
				Hardware::Protocols::EsuCAN::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeDccPpExTcp:
				Hardware::DccPpExTcp::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeDccPpExSerial:
				Hardware::DccPpExSerial::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeIntellibox2:
				Hardware::Intellibox2::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeLocoNetAdapter63120:
				Hardware::LocoNetAdapter63120::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeLocoNetAdapter63820:
				Hardware::LocoNetAdapter63820::GetArgumentTypesAndHint(arguments, hint);
				return;

			case HardwareTypeSystemControl7:
				Hardware::SystemControl7::GetArgumentTypesAndHint(arguments, hint);
				return;
		}
	}
} // namespace Hardware
