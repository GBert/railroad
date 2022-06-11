/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2022 Dominik (Teddy) Mahrer - www.railcontrol.org

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

#include "Hardware/HardwareInterface.h"
#include "Hardware/HardwareParams.h"
#include "Logger/Logger.h"
#include "Network/Serial.h"

// protocol specification at https://www.digitrax.com/static/apps/cms/media/documents/loconet/loconetpersonaledition.pdf

namespace Hardware
{
	namespace Protocols
	{
		class LocoNet: Hardware::HardwareInterface
		{
			public:
				LocoNet() = delete;
				LocoNet(const LocoNet&) = delete;
				LocoNet& operator=(const LocoNet&) = delete;

				LocoNet(const HardwareParams* params, const std::string& controlName);
				virtual ~LocoNet();

				inline Hardware::Capabilities GetCapabilities() const override
				{
					return Hardware::CapabilityNone;
				}

				virtual void Booster(const BoosterState status);

			private:
				enum OpCodes : unsigned char
				{
					OPC_GPON        = 0x83,
					OPC_GPOFF       = 0x82
				};

				void Receiver();

				static void CalcCheckSum(unsigned char* data, unsigned char length);

				volatile bool run;
				mutable Network::Serial serialLine;
				std::thread receiverThread;
		};
	} // namespace
} // namespace

