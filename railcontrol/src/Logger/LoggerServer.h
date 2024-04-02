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

#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "Logger/LoggerClient.h"
#include "Logger/LoggerClientConsole.h"
#include "Logger/LoggerClientFile.h"
#include "Logger/LoggerClientTcp.h"

namespace Logger
{
	class Logger;

	class LoggerServer
	{
		public:
			LoggerServer(const LoggerServer&) = delete;
			LoggerServer& operator=(const LoggerServer&) = delete;

			Logger* GetLogger(const std::string& component);

			void Send(const std::string& text);

			static inline LoggerServer& Instance()
			{
				static LoggerServer server;
				return server;
			}

			inline void AddFileLogger(const std::string& fileName)
			{
				if (fileLoggerStarted == true)
				{
					return;
				}
				clients.push_back(new LoggerClientFile(fileName));
				fileLoggerStarted = true;
			}

			inline void AddConsoleLogger()
			{
				if (consoleLoggerStarted == true)
				{
					return;
				}
				clients.push_back(new LoggerClientConsole());
				consoleLoggerStarted = true;
			}

		private:
			inline LoggerServer()
			:	fileLoggerStarted(false),
			 	consoleLoggerStarted(false)
			{
			}

			~LoggerServer();

			bool fileLoggerStarted;
			bool consoleLoggerStarted;
			std::vector<LoggerClient*> clients;
			std::vector<Logger*> loggers;
	};
}
