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

#include <cstdint>
#include <string>

// common
typedef uint8_t ControlID;

// objects in db
typedef uint16_t ObjectID;

// loco
typedef ObjectID LocoID;
typedef uint16_t Address;
typedef uint16_t Speed;
typedef uint32_t Length;

// multiple unit
typedef ObjectID MultipleUnitID;

// accessory
typedef ObjectID AccessoryID;

// feedback
typedef ObjectID FeedbackID;
typedef uint32_t FeedbackPin;

// track
typedef ObjectID TrackID;
typedef ObjectID ClusterID;

// switch
typedef AccessoryID SwitchID;

// signal
typedef AccessoryID SignalID;

// route
typedef ObjectID RouteID;
typedef uint16_t Delay;

// layer
typedef int16_t LayerID;

// relations
typedef uint16_t Priority;

typedef uint8_t Pause;

// text
typedef ObjectID TextID;

static const Address AddressNone = 0;
static const Address AddressDefault = 3;
static const LocoID LocoNone = 0;
static const MultipleUnitID MultipleUnitNone = 0;
static const MultipleUnitID MultipleUnitIdPrefix = 0x8000;
static const ObjectID ObjectNone = 0;
static const AccessoryID AccessoryNone = 0;
static const FeedbackID FeedbackNone = 0;
static const FeedbackPin FeedbackPinNone = 0;
static const TrackID TrackNone = 0;
static const SwitchID SwitchNone = 0;
static const RouteID RouteNone = 0;
static const RouteID RouteAuto = 0;
static const RouteID RouteStop = 0xFFFF;
static const ControlID ControlNone = 0;
static const LayerID LayerNone = 0;
static const LayerID LayerUndeletable = 1;
static const SignalID SignalNone = 0;
static const ClusterID ClusterNone = 0;
static const TextID TextNone = 0;

static const Speed MaxSpeed = 1023;
static const Speed DefaultTravelSpeed = 700;
static const Speed DefaultReducedSpeed = 400;
static const Speed DefaultCreepingSpeed = 100;
static const Speed MinSpeed = 0;

enum ControlType : uint8_t
{
	ControlTypeHardware = 0,
	ControlTypeInternal,
	ControlTypeWebServer,
	ControlTypeZ21Server,
	ControlTypeCS2Server
};

enum ControlIDs : ControlID
{
	ControlIdNone = 0,
	ControlIdWebServer,
	ControlIdZ21Server,
	ControlIdCS2Server,
	ControlIdFirstHardware = 10
};

enum BoosterState : bool
{
	BoosterStateStop = false,
	BoosterStateGo = true
};

enum Protocol : uint8_t
{
	ProtocolNone = 0,
	ProtocolServer = 1,
	ProtocolMM1 = 2,
	ProtocolMM2 = 3,
	ProtocolMFX = 4,
	ProtocolDCC = 5,
	ProtocolDCC14 = 6,
	ProtocolDCC28 = 7,
	ProtocolDCC128 = 8,
	ProtocolMM = 9,
	ProtocolMM15 = 10,
	ProtocolSX1 = 11,
	ProtocolSX2 = 12,
	ProtocolFMZ = 13,
	ProtocolMultipleUnit = 14,
	ProtocolEnd = ProtocolMultipleUnit
};

static const std::string ProtocolSymbols[] =
{
	"none",
	"all",
	"MM 1",
	"MM 2",
	"mfx",
	"DCC",
	"DCC 14",
	"DCC 28",
	"DCC 128",
	"MM",
	"MM 1.5",
	"SX1",
	"SX2",
	"FMZ",
	"Multi"
};

enum AddressType : uint8_t
{
	AddressTypeLoco = 0,
	AddressTypeMultipleUnit,
	AddressTypeAccessory
};

enum LocoType : uint8_t
{
	LocoTypeLoco = 0,
	LocoTypeMultipleUnit
};

enum ArgumentType : uint8_t
{
	ArgumentTypeIpAddress = 1,
	ArgumentTypeSerialPort = 2,
	ArgumentTypeS88Modules = 3,
	ArgumentTypeMasterSlave = 4
};

enum HardwareType : uint8_t
{
	HardwareTypeNone = 0,
	HardwareTypeVirtual = 1,
	HardwareTypeCS2Udp = 2,
	HardwareTypeM6051 = 3,
	// HardwareTypeRM485 = 4, // not used anymore, but still reserved for old configurations
	HardwareTypeOpenDcc = 5,
	HardwareTypeHsi88 = 6,
	HardwareTypeZ21 = 7,
	HardwareTypeCcSchnitte = 8,
	HardwareTypeEcos = 9,
	HardwareTypeCS2Tcp = 10,
	HardwareTypeIntellibox = 11,
	HardwareTypeMasterControl = 12,
	HardwareTypeTwinCenter = 13,
	HardwareTypeMasterControl2 = 14,
	HardwareTypeRedBox = 15,
	HardwareTypeRektor = 16,
	HardwareTypeDR5000 = 17,
	HardwareTypeCS1 = 18,
	HardwareTypeDccPpExTcp = 19,
	HardwareTypeDccPpExSerial = 20,
	HardwareTypeIntellibox2 = 21,
	HardwareTypeLocoNetAdapter63120 = 22,
	HardwareTypeLocoNetAdapter63820 = 23,
	HardwareTypeSystemControl7 = 24
};

enum Automode : bool
{
	AutomodeNo = false,
	AutomodeYes = true
};

enum ObjectType : uint8_t
{
	ObjectTypeNone = 0,
	ObjectTypeLoco = 1,
	ObjectTypeTrack = 2,
	ObjectTypeFeedback = 3,
	ObjectTypeAccessory = 4,
	ObjectTypeSwitch = 5,
	ObjectTypeRoute = 6,
	ObjectTypeLayer = 7,
	ObjectTypeSignal = 8,
	ObjectTypeCluster = 9,
	ObjectTypeTimeTable = 10,
	ObjectTypeText = 11,
	ObjectTypePause = 12,
	ObjectTypeMultipleUnit = 13,
	ObjectTypeBooster = 14
};

enum Orientation : uint8_t
{
	OrientationLeft = 0,
	OrientationRight = 1,
	OrientationChange = 2
};

enum StartupInitLocos : uint8_t
{
	StartupInitLocosNone  = 0,
	StartupInitLocosSpeed = 1,
	StartupInitLocosAll   = 2,
};

enum ProgramMode : uint8_t
{
	ProgramModeNone,
	ProgramModeDccDirect,
	ProgramModeDccPomLoco,
	ProgramModeDccPomAccessory,
	ProgramModeMm,
	ProgramModeMmPom,
	ProgramModeMfx,
	ProgramModeDccPage,
	ProgramModeDccRegister
};

typedef uint16_t CvNumber;
typedef uint8_t CvValue;

enum Propulsion : uint8_t
{
	PropulsionUnknown  = 0x00,
	PropulsionSteam    = 0x01,
	PropulsionDiesel   = 0x02,
	PropulsionGas      = 0x04,
	PropulsionElectric = 0x08,
	PropulsionHydrogen = 0x10,
	PropulsionAccu     = 0x20,
	PropulsionOther    = 0x80,
	PropulsionAll      = PropulsionSteam
							| PropulsionDiesel
							| PropulsionGas
							| PropulsionElectric
							| PropulsionHydrogen
							| PropulsionAccu
							| PropulsionOther
};

enum TrainType : uint32_t
{
	TrainTypeUnknown                   = 0x00000000,

	TrainTypeInternationalHighSpeed    = 0x00000001,
	TrainTypeNationalHighSpeed         = 0x00000002,
	TrainTypeInternationalLongDistance = 0x00000004,
	TrainTypeNationalLongDistance      = 0x00000008,
	TrainTypeInternationalNight        = 0x00000010,
	TrainTypeNationalNight             = 0x00000020,
	TrainTypeLongDistanceFastLocal     = 0x00000040,
	TrainTypeFastLocal                 = 0x00000080,
	TrainTypeLocal                     = 0x00000100,
	TrainTypeSuburban                  = 0x00000200,
	TrainTypeUnderground               = 0x00000400,
	TrainTypeHistoric                  = 0x00001000,
	TrainTypeExtra                     = 0x00002000,

 	TrainTypePassengerWithCargo        = 0x00008000,

	TrainTypePassenger                 = TrainTypeInternationalHighSpeed
											| TrainTypeNationalHighSpeed
											| TrainTypeInternationalLongDistance
											| TrainTypeNationalLongDistance
											| TrainTypeInternationalNight
											| TrainTypeNationalNight
											| TrainTypeLongDistanceFastLocal
											| TrainTypeFastLocal
											| TrainTypeLocal
											| TrainTypeSuburban
											| TrainTypeUnderground
											| TrainTypeHistoric
											| TrainTypeExtra
											| TrainTypePassengerWithCargo,

	TrainTypeCargoLongDistance         = 0x00010000,
	TrainTypeCargoLocal                = 0x00020000,
	TrainTypeCargoBlock                = 0x00040000,
	TrainTypeCargoTractor              = 0x00080000,
	TrainTypeCargoExpress              = 0x00100000,

	TrainTypeCargoWithPassenger        = 0x00800000,

 	TrainTypeCargo                     = TrainTypeCargoLongDistance
 											| TrainTypeCargoLocal
 											| TrainTypeCargoBlock
 											| TrainTypeCargoTractor
 											| TrainTypeCargoExpress
 											| TrainTypeCargoWithPassenger,

	TrainTypeRescue                    = 0x01000000,
	TrainTypeConstruction              = 0x02000000,
	TrainTypeEmpty                     = 0x04000000,
	TrainTypeLoco                      = 0x08000000,
	TrainTypeCleaning                  = 0x10000000,

	TrainTypeOther                     = 0x40000000,
	// Bit 32 (0x80000000) can not be used because of conversion functions that handle with signed int32_t

	TrainTypeAll                       = TrainTypePassenger
											| TrainTypeCargo
											| TrainTypeRescue
											| TrainTypeConstruction
											| TrainTypeEmpty
											| TrainTypeLoco
											| TrainTypeCleaning
											| TrainTypeOther
};
