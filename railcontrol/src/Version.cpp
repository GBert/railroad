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

#include "Version.h"

const std::string& GetVersionInfoGitHash()
{
	static const std::string gitHash = "8051e725276bd16b6b051314efc88e778c141f09";
	return gitHash;
}

time_t GetVersionInfoGitTimestamp()
{
	return 1772652976;
}

unsigned int GetVersionInfoGitDirty()
{
	return 2;
}

time_t GetVersionInfoCompileTimestamp()
{
	return 1772808385;
}

const std::string& GetVersionInfoRailControlVersion()
{
	static const std::string railControlVersion = "24";
	return railControlVersion;
}

const std::string& GetOS()
{
#if defined(__CYGWIN__)
	static const std::string os = "Cygwin";
#elif defined(_WIN64) or defined(_WIN32)
	static const std::string os = "Windows";
#elif defined(__android__)
	static const std::string os = "Android";
#elif defined(__linux__) or defined(__gnu_linux__)
	static const std::string os = "Linux";
#elif defined(macintosh)
	static const std::string os = "Mac OS";
#elif defined(__APPLE) and defined(__MACH__)
	static const std::string os = "Mac OS X";
#else
	static const std::string os = "Unknown OS";
#endif
	return os;
}

const std::string& GetCompiler()
{
#if defined(__clang__)
	static const std::string compiler = "clang";
#elif defined(__GNUC__)
	static const std::string compiler = "GCC";
#else
	static const std::string compiler = "Unknown Compiler";
#endif
	return compiler;
}
