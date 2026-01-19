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

#include <algorithm>
#include <arpa/inet.h>
#include <cstdarg>    // va_* in xlog
#include <cstdio>     // printf
#include <cstdlib>    // exit(0);
#include <cstring>    // memset
#include <dirent.h>
#include <fstream>
#include <iostream>   // cout
#include <netdb.h>
#include <string>
#include <sys/time.h> // gettimeofday
#include <vector>

#include "Languages.h"
#include "Logger/Logger.h"
#include "Network/Select.h"
#include "Utils/Integer.h"
#include "Utils/Utils.h"

using std::cout;
using std::deque;
using std::endl;
using std::string;
using std::to_string;
using std::vector;

namespace Utils
{
	void Utils::ReplaceString(std::string& str, const std::string& from, const std::string& to)
	{
		size_t lastOccurrence = 0;
		while (true)
		{
			size_t startPos = str.find(from, lastOccurrence);
			if (startPos == string::npos)
			{
				return;
			}
			str.replace(startPos, from.length(), to);
			lastOccurrence = startPos + to.length();
		}
	}

	void Utils::SplitString(const string& input, const string& delimiter, deque<string>& list)
	{
		size_t delimiterLength = delimiter.length();
		string workingString(input);
		while (true)
		{
			size_t pos = workingString.find(delimiter);
			list.push_back(workingString.substr(0, pos));
			if (pos == string::npos)
			{
				return;
			}
			workingString = string(workingString.substr(pos + delimiterLength, string::npos));
		}
	}

	void Utils::SplitString(const string& input, const string& delimiter, string& first, string& second)
	{
		size_t delimiterLength = delimiter.length();
		size_t pos = input.find(delimiter);
		first = input.substr(0, pos);
		second = input.substr(pos + delimiterLength);
	}

	string Utils::StringBeforeDelimiter(const std::string& input, const std::string& delimiter)
	{
		string ret;
		string unused;
		SplitString(input, delimiter, ret, unused);
		return ret;
	}

	string Utils::UrlDecode(const string& value)
	{
		string output = value;
		size_t startSearch = 0;
		while (true)
		{
			size_t pos = output.find('%', startSearch);
			if (pos == string::npos || pos + 3 > output.length())
			{
				break;
			}
			const unsigned char highNibble = Integer::HexToChar(output[pos + 1]);
			const unsigned char lowNibble = Integer::HexToChar(output[pos + 2]);
			const unsigned char c = (highNibble << 4) + lowNibble;
			output.replace(pos, 3, 1, c);
			startSearch = pos + 1;
		}
		return output;
	}

	string Utils::UrlEncode(const string& value)
	{
		string output = value;
		ReplaceString(output, "%", "%25");
		ReplaceString(output, "&", "%26");
		ReplaceString(output, "=", "%3d");
		return output;
	}

	string Utils::HtmlEncode(const string& value)
	{
		string modifiedValue = value;
		ReplaceString(modifiedValue, "&", "&amp;");
		ReplaceString(modifiedValue, "\"", "&quot;");
		ReplaceString(modifiedValue, "'", "&apos;");
		ReplaceString(modifiedValue, "<", "&lt;");
		ReplaceString(modifiedValue, ">", "&gt;");
		return modifiedValue;
	}

	const std::string& Utils::GetStringMapEntry(const std::map<std::string, std::string>& map, const std::string& key, const std::string& defaultValue)
	{
		if (map.count(key) == 0)
		{
			return defaultValue;
		}
		return map.at(key);
	}

	int Utils::GetIntegerMapEntry(const std::map<std::string, std::string>& map, const std::string& key, const int defaultValue)
	{
		if (map.count(key) == 0)
		{
			return defaultValue;
		}
		return Integer::StringToInteger(map.at(key), defaultValue);
	}

	bool Utils::GetBoolMapEntry(const std::map<std::string, std::string>& map, const std::string& key, const bool defaultValue)
	{
		if (map.count(key) == 0)
		{
			return defaultValue;
		}
		string value = map.at(key);
		return (value.compare("true") == 0 || value.compare("on") == 0 || value.compare("1") == 0 || key.compare(value) == 0);
	}

	string Utils::ToStringWithLeadingZeros(const unsigned int number, const unsigned char chars)
	{
		string out = to_string(number);
		while (out.length() < chars)
		{
			out.insert(0, "0");
		}
		return out;
	}

	bool Utils::StringToBool(const std::string& value, const bool defaultValue)
	{
		if (value.size() == 0)
		{
			return defaultValue;
		}
		int intValue = Integer::StringToInteger(value);
		return intValue != 0;
	}

	string Utils::StringToLower(const string& input)
	{
		string output = input;
		for(auto& c : output)
		{
		   c = tolower(c);
		}
		return output;
	}

	bool Utils::CopyFile(Logger::Logger* logger, const std::string& from, const std::string& to)
	{
		std::ifstream source(from, std::ios::binary);
		if (!source.is_open())
		{
			return false;
		}
		std::ofstream destination(to, std::ios::binary);
		if (!destination.is_open())
		{
			return false;
		}
		destination << source.rdbuf();
		source.close();
		destination.close();
		logger->Info(Languages::TextCopyingFromTo, from, to);
		return true;
	}

	bool Utils::RenameFile(Logger::Logger* logger, const std::string& from, const std::string& to)
	{
		const bool ret = std::rename(from.c_str(), to.c_str());
		if (ret && logger)
		{
			logger->Info(Languages::TextRenamingFromTo, from, to);
		}
		return ret;
	}

	void Utils::RemoveOldBackupFiles (Logger::Logger *logger,
		const std::string &filename,
		unsigned int keepBackups)
	{
		DIR *dir = opendir(".");
		if (!dir)
		{
			return;
		}
		struct dirent *ent;
		std::vector < string > fileNames;
		const string filenameSearch = filename + ".";
		const size_t filenameSearchLength = filenameSearch.length() + 10;
		while (true)
		{
			ent = readdir(dir);
			if (!ent)
			{
				break;
			}
			string fileName = ent->d_name;
			if ((fileName.length() != filenameSearchLength) || (fileName.find(filenameSearch) == string::npos))
			{
				continue;
			}
			fileNames.push_back(ent->d_name);
		}
		closedir(dir);
		std::sort(fileNames.begin(), fileNames.end());

		size_t numberOfFiles = fileNames.size();
		if ((numberOfFiles == 0) || (numberOfFiles < keepBackups))
		{
			return;
		}

		unsigned int removeBackups = fileNames.size() - keepBackups;
		++removeBackups; // at shutdown we create another backupfile
		for (auto &fileName : fileNames)
		{
			if (removeBackups == 0)
			{
				return;
			}

			--removeBackups;
			logger->Info(Languages::TextRemoveBackupFile, fileName);
			remove(fileName.c_str());
		}
	}

	void Utils::SetMinThreadPriority()
	{
		sched_param param;
		int policy;
		pthread_t self = pthread_self();
		pthread_getschedparam(self, &policy, &param);
		param.sched_priority = sched_get_priority_min(policy);
		pthread_setschedparam(self, policy, &param);
	}

	std::string Utils::TimestampToDate(const time_t timestamp)
	{
		struct tm *tm = localtime(&timestamp);
		char date[20];
		strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);
		return date;
	}

#ifdef __CYGWIN__
	bool Utils::GetFilesInDir(vector<string>& filesFound, const string& path, const string& prefix)
	{
		bool ret = false;
		size_t prefixLength = prefix.length();
		DIR* dirp = opendir(path.c_str());
		struct dirent* dp;
		while ((dp = readdir(dirp)) != nullptr)
		{
			string file(dp->d_name);
			string prefixRead = file.substr(0, prefixLength);
			if (prefix.compare(prefixRead) != 0)
			{
				continue;
			}
			filesFound.push_back(file);
			ret = true;
		}
		closedir(dirp);
		return ret;
	}

	bool Utils::GetComPorts(std::vector<unsigned char>& comPorts)
	{
		vector<string> filesFound;
		bool ret = GetFilesInDir(filesFound, "/dev/", "ttyS");
		if (ret == false)
		{
			return false;
		}
		for (auto& file : filesFound)
		{
			string comPortString = file.substr(4);
			unsigned char comPort = Integer::StringToInteger(comPortString);
			comPorts.push_back(comPort);
		}
		return true;
	}
#endif

	uint8_t Utils::CalcXORCheckSum(const uint8_t* const buffer, size_t length)
	{
		uint8_t ret = 0;
		for (size_t i = 0; i < length; ++i)
		{
			ret ^= buffer[i];
		}
		return ret;
	}
}
