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

#include <cstdint>
#include <climits>

namespace Utils
{
	class Integer
	{
		public:
			static inline int StringToInteger(const std::string& value)
			{
				return StringToInteger(value, 0, INT_MAX);
			}

			static int StringToInteger(const std::string& value, const int defaultValue);
			static int StringToInteger(const std::string& value, const int min, const int max);
			static long HexToInteger(const std::string& value, const long defaultValue = 0);
			static signed char HexToChar(signed char c);

			static void IntToDataBigEndian(const uint32_t i, unsigned char* buffer);
			static uint32_t DataBigEndianToInt(const unsigned char* buffer);
			static void ShortToDataBigEndian(const uint16_t i, unsigned char* buffer);
			static uint16_t DataBigEndianToShort(const unsigned char* buffer);
			static void IntToDataLittleEndian(const uint32_t i, unsigned char* buffer);
			static uint32_t DataLittleEndianToInt(const unsigned char* buffer);
			static void ShortToDataLittleEndian(const uint16_t i, unsigned char* buffer);
			static uint16_t DataLittleEndianToShort(const unsigned char* buffer);
			static std::string IntegerToBCD(const unsigned int input);
			static std::string IntegerToHex(const unsigned int input, const unsigned int size = 1);

	}; // class Integer
} // namespace Utils
