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
	static const std::string gitHash = "ab380be5ab8c316b3db0c5cdc38c2c4e1350be22";
	return gitHash;
}

time_t GetVersionInfoGitTimestamp()
{
	return 1700060240;
}

unsigned int GetVersionInfoGitDirty()
{
	return 1;
}

time_t GetVersionInfoCompileTimestamp()
{
	return 1700088465;
}

const std::string& GetVersionInfoRailControlVersion()
{
	static const std::string railControlVersion = "22";
	return railControlVersion;
}

