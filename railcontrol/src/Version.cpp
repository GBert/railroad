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

#include "Version.h"

const std::string& GetVersionInfoGitHash()
{
	static const std::string gitHash = "54ebbde4de42fa5d7ab691e5e56c4105b3a8444e";
	return gitHash;
}

time_t GetVersionInfoGitTimestamp()
{
	return 1722106338;
}

unsigned int GetVersionInfoGitDirty()
{
	return 1;
}

time_t GetVersionInfoCompileTimestamp()
{
	return 1722452936;
}

const std::string& GetVersionInfoRailControlVersion()
{
	static const std::string railControlVersion = "23";
	return railControlVersion;
}

