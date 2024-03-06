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

#pragma once

// protocol specification at https://www.z21.eu/media/Kwc_Basic_DownloadTag_Component/47-1652-959-downloadTag/default/69bad87e/1558674980/z21-lan-protokoll.pdf

namespace Hardware
{
	namespace Protocols
	{
		namespace Z21Enums
		{
			enum BroadCastFlags : uint32_t
			{
				BroadCastFlagNone            = 0x00000000,
				BroadCastFlagBasic           = 0x00000001,
				BroadCastFlagRBus            = 0x00000002,
				BroadCastFlagRailCom         = 0x00000004,
				BroadCastFlagSystemState     = 0x00000100,
				BroadCastFlagAllLoco         = 0x00010000,
				BroadCastFlagRailComData     = 0x00040000,
				BroadCastFlagCanDetector     = 0x00080000,
				BroadCastFlagLocoNetBasic    = 0x01000000,
				BroadCastFlagLocoNetLoco     = 0x02000000,
				BroadCastFlagLocoNetSwitch   = 0x04000000,
				BroadCastFlagLocoNetDetector = 0x08000000
			};

			enum Command : uint8_t
			{
				CommandSetLocoMode        = 0x61,
				CommandSetTurnoutMode     = 0x70
			};

			enum ProtocolMode : uint8_t
			{
				ProtocolModeDCC           = 0,
				ProtocolModeMM            = 1
			};

			enum FeatureSet : uint8_t
			{
				FeaturesNotRestricted     = 0x00,
				FeaturesStartLocked       = 0x01,
				FeaturesStartUnlocked     = 0x02
			};

			enum Header : uint16_t
			{
				HeaderSerialNumber        = 0x10,
				HeaderGetCode             = 0x18,
				HeaderGetHardwareInfo     = 0x1A,
				HeaderLogOff              = 0x30,
				HeaderSeeXHeader          = 0x40,
				HeaderSetBroadcastFlags   = 0x50,
				HeaderGetBroadcastFlags   = 0x51,
				HeaderGetLocoMode         = 0x60,
				HeaderSetLocoMode         = 0x61,
				HeaderGetTurnoutMode      = 0x70,
				HeaderSetTurnoutMode      = 0x71,
				HeaderRmBusData           = 0x80,
				HeaderSystemData          = 0x84,
				HeaderGetSystemState      = 0x85,
				HeaderRailComtData        = 0x88,
				HeaderLocoNetRx           = 0xA0,
				HeaderLocoNetTx           = 0xA1,
				HeaderLocoNetLan          = 0xA2,
				HeaderLocoNetDispatch     = 0xA3,
				HeaderLocoNetDetector     = 0xA4,
				HeaderDetector            = 0xC4
			};

			enum XHeader : uint8_t
			{
				XHeaderSeeDB0_1           = 0x21,
				XHeaderTurnoutInfo        = 0x43,
				XHeaderGetTurnoutInfo     = 0x43,
				XHeaderSetTurnout         = 0x53,
				XHeaderSeeDB0_2           = 0x61,
				XHeaderStatusChanged      = 0x62,
				XHeaderVersion            = 0x63,
				XHeaderCvResult           = 0x64,
				XHeaderSetStop            = 0x80,
				XHeaderBcStopped          = 0x81,
				XHeaderGetLocoInfo        = 0xE3,
				XHeaderLocoDrive          = 0xE4,
				XHeaderSetLocoBinaryState = 0xE5,
				XHeaderLocoInfo           = 0xEF,
				XHeaderGetFirmwareVersion = 0xF1,
				XHeaderFirmwareVersion    = 0xF3
			};

			enum DB0 : uint8_t
			{
				DB0PowerOff               = 0x00,
				DB0PowerOn                = 0x01,
				DB0ProgrammingMode        = 0x02,
				DB0ShortCircuit           = 0x08,
				DB0FirmwareVersion        = 0x0A,
				DB0SetLocoDrive14         = 0x10,
				DB0SetLocoDrive28         = 0x12,
				DB0CvShortCircuit         = 0x12,
				DB0SetLocoDrive128        = 0x13,
				DB0CvNack                 = 0x13,
				DB0Version                = 0x21,
				DB0StatusChanged          = 0x22,
				DB0Status                 = 0x24,
				DB0SetPowerOff            = 0x80,
				DB0SetPowerOn             = 0x81,
				DB0UnknownCommand         = 0x82,
				DB0LocoFunction           = 0xF8
			};

			enum PomDB0 : uint8_t
			{
				PomLoco                   = 0x30,
				PomAccessory              = 0x31
			};

			enum PomOption : uint16_t
			{
				PomWriteByte              = 0xEC00,
				PomWriteBit               = 0xE800,
				PomReadByte               = 0xE400
			};
		}
	}
}
