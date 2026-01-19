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

#include <iomanip>
#include <sstream>

#include "Utils/Integer.h"

namespace Utils
{
	int Integer::StringToInteger(const std::string& value, const int defaultValue)
	{
		size_t valueSize = value.length();
		if (valueSize == 0)
		{
			return defaultValue;
		}

		char* end;
		const char* start = value.c_str();
		long longValue = std::strtol(start, &end, 10);
		if (errno == ERANGE || start == end)
		{
			return defaultValue;
		}
		if (longValue > INT_MAX || longValue < INT_MIN)
		{
			return defaultValue;
		}
		return static_cast<int>(longValue);
	}

	int Integer::StringToInteger(const std::string& value, const int min, const int max)
	{
		int intValue = StringToInteger(value, min);

		if (intValue < min)
		{
			return min;
		}

		if (intValue > max)
		{
			return max;
		}

		return intValue;
	}

	long Integer::HexToInteger(const std::string& value, const long defaultValue)
	{
		size_t valueSize = value.length();
		if (valueSize == 0)
		{
			return defaultValue;
		}

		char* end;
		const char* start = value.c_str();
		long longValue = std::strtol(start, &end, 16);
		if (errno == ERANGE || start == end)
		{
			return defaultValue;
		}
		return longValue;
	}

	signed char Integer::HexToChar(signed char c)
	{
		if (c >= 'a')
		{
			c -= 'a' - 10;
		}
		else if (c >= 'A')
		{
			c -= 'A' - 10;
		}
		else if (c >= '0')
		{
			c -= '0';
		}

		return c > 15 ? 0 : c;
	}

	void Integer::IntToDataBigEndian(const uint32_t i, unsigned char* buffer)
	{
		buffer[0] = (i >> 24);
		buffer[1] = ((i >> 16) & 0xFF);
		buffer[2] = ((i >> 8) & 0xFF);
		buffer[3] = (i & 0xFF);
	}

	uint32_t Integer::DataBigEndianToInt(const unsigned char* buffer)
	{
		uint32_t i = buffer[0];
		i <<= 8;
		i |= buffer[1];
		i <<= 8;
		i |= buffer[2];
		i <<= 8;
		i |= buffer[3];
		return i;
	}

	void Integer::ShortToDataBigEndian(const uint16_t i, unsigned char* buffer)
	{
		buffer[0] = (i >> 8);
		buffer[1] = (i & 0xFF);
	}

	uint16_t Integer::DataBigEndianToShort(const unsigned char* buffer)
	{
		uint16_t i = buffer[0];
		i <<= 8;
		i |= buffer[1];
		return i;
	}

	void Integer::IntToDataLittleEndian(const uint32_t i, unsigned char* buffer)
	{
		buffer[0] = (i & 0xFF);
		buffer[1] = ((i >> 8) & 0xFF);
		buffer[2] = ((i >> 16) & 0xFF);
		buffer[3] = (i >> 24);
	}

	uint32_t Integer::DataLittleEndianToInt(const unsigned char* buffer)
	{
		return buffer[0]
			+ (buffer[1] << 8)
			+ (buffer[2] << 16)
			+ (buffer[3] << 24);
	}

	void Integer::ShortToDataLittleEndian(const uint16_t i, unsigned char* buffer)
	{
		buffer[0] = (i & 0xFF);
		buffer[1] = (i >> 8);
	}

	uint16_t Integer::DataLittleEndianToShort(const unsigned char* buffer)
	{
		return buffer[0]
			+ (buffer[1] << 8);
	}

	std::string Integer::IntegerToBCD(const unsigned int input)
	{
		unsigned char zero = (input >> 4);
		zero &= 0x0000000F;
		zero += '0';
		unsigned char one = input;
		one &= 0x0000000F;
		one += '0';
		std::string output;
		output.append(1, zero);
		output.append(1, one);
		return output;
	}

	std::string Integer::IntegerToHex(const unsigned long int input, const unsigned int size)
	{
		if (input == 0)
		{
			return "0";
		}
		std::string output;

		unsigned long int decimal = input;
		unsigned int internalSize = 0;
		while (decimal)
		{
			std::stringstream part;
			part << std::setfill('0') << std::setw(1) << std::hex << (decimal & 0xF);
			output = part.str() + output;
			decimal >>= 4;
			++internalSize;
		}
		while (internalSize < size)
		{
			++internalSize;
			output = "0" + output;
		}
		return output;
	}
}
