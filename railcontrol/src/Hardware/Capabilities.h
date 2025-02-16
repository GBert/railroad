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

#pragma once

#include <stdint.h>

namespace Hardware
{
	typedef uint32_t Capability;
	enum Capabilities : Capability
	{
		CapabilityNone                         = 0x00000000,
		CapabilityLoco                         = 0x00000001,
		CapabilityMultipleUnit                 = 0x00000002,
		CapabilityAccessory                    = 0x00000004,
		CapabilityFeedback                     = 0x00000008,
		CapabilityProgram                      = 0x00000010,
		CapabilityProgramMmWrite               = 0x00000020,
		CapabilityProgramMmPomWrite            = 0x00000040,
		CapabilityProgramMfxRead               = 0x00000080,
		CapabilityProgramMfxWrite              = 0x00000100,
		CapabilityProgramDccRegisterRead       = 0x00000200,
		CapabilityProgramDccRegisterWrite      = 0x00000400,
		CapabilityProgramDccPageRead           = 0x00000800,
		CapabilityProgramDccPageWrite          = 0x00001000,
		CapabilityProgramDccDirectRead         = 0x00002000,
		CapabilityProgramDccDirectWrite        = 0x00004000,
		CapabilityProgramDccPomLocoRead        = 0x00008000,
		CapabilityProgramDccPomLocoWrite       = 0x00010000,
		CapabilityProgramDccPomAccessoryRead   = 0x00020000,
		CapabilityProgramDccPomAccessoryWrite  = 0x00040000,
		CapabilityLocoDatabase                 = 0x00080000,
		CapabilityMultipleUnitDatabase         = 0x00100000,
		CapabilityAccessoryDatabase            = 0x00200000,
		CapabilityFeedbackDatabase             = 0x00400000,
	};

	inline Capabilities operator& (const Capabilities c1, const Capabilities c2)
	{
		return static_cast<Capabilities>(static_cast<Capability>(c1) & static_cast<Capability>(c2));
	}

	inline Capabilities operator| (const Capabilities c1, const Capabilities c2)
	{
		return static_cast<Capabilities>(static_cast<Capability>(c1) | static_cast<Capability>(c2));
	}
}
