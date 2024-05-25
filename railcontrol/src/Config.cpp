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

#include <fstream>
#include <istream> // std::ws
#include <sstream>

#include "Config.h"
#include "Logger/Logger.h"
#include "Utils/Integer.h"

using std::map;
using std::string;

Config::Config(const std::string& fileName)
{
	Logger::Logger* logger = Logger::LoggerServer::Instance().GetLogger("Config");
	// read config values
	logger->Info(Languages::TextReadingConfigFile, fileName);

	std::ifstream configFile;
	configFile.open(fileName);
	if (!configFile.is_open())
	{
		logger->Warning(Languages::TextUnableToOpenFile, fileName);
		return;
	}

	for (string line; std::getline(configFile, line); )
	{
		std::istringstream iss(line);
		string configKey;
		string eq;
		string configValue;

		iss >> configKey >> eq >> configValue >> std::ws;

		if ((configKey.size() == 0) || (configKey[0] == '#'))
		{
			continue;
		}

		if (eq.compare("=") != 0)
		{
			continue;
		}

		config[configKey] = configValue;
		logger->Info(Languages::TextParameterFoundInConfigFile, configKey, configValue);
	}
	configFile.close();
}

const string& Config::getStringValue(const string& key, const string& defaultValue)
{
	if (config.count(key) != 1)
	{
		return defaultValue;
	}
	return config[key];
}

int Config::getIntValue(const string& key, const int defaultValue)
{
	if (config.count(key) != 1)
	{
		return defaultValue;
	}
	return Utils::Integer::StringToInteger(config[key], defaultValue);
}

bool Config::getBoolValue(const string& key, const bool defaultValue)
{
	const string value = Utils::Utils::StringToLower(getStringValue(key, string(defaultValue ? "1" : "0")));
	return ((value.compare("true") == 0) || (value.compare("on") == 0) || (value.compare("1") == 0));
}
