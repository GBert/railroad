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
#include <string>

#include "Languages.h"
#include "Hardware/Protocols/LocoNet.h"

namespace Hardware
{
	class HardwareParams;

	class LocoNetAdapter63120 : public Protocols::LocoNet
	{
		public:
			LocoNetAdapter63120() = delete;
			LocoNetAdapter63120(const LocoNetAdapter63120&) = delete;
			LocoNetAdapter63120& operator=(const LocoNetAdapter63120&) = delete;

			inline LocoNetAdapter63120(const HardwareParams* params)
			:	Protocols::LocoNet(params, "LocoNet Adapter 63120", B115200)
			{
			}

			virtual ~LocoNetAdapter63120()
			{
			}

			static inline void GetArgumentTypesAndHint(std::map<unsigned char,ArgumentType>& argumentTypes, std::string& hint)
			{
				argumentTypes[1] = ArgumentTypeSerialPort;
				hint = Languages::GetText(Languages::TextHintLocoNetAdapter63120);
			}
	};
} // namespace

