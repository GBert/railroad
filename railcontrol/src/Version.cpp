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

#include "Version.h"

const std::string& GetVersionInfoGitHash()
{
	static const std::string gitHash = "6bfeee9d30ad53ba36e8010a2c2bf7e8bb138632";
	return gitHash;
}

time_t GetVersionInfoGitTimestamp()
{
	return 1710609008;
}

unsigned int GetVersionInfoGitDirty()
{
	return 1;
}

time_t GetVersionInfoCompileTimestamp()
{
	return 1710672086;
}

const std::string& GetVersionInfoRailControlVersion()
{
	static const std::string railControlVersion = "23";
	return railControlVersion;
}

