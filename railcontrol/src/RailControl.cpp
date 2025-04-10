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

#include <cstdio>		//printf
#include <cstdlib>		//exit(0);
#include <cstring>		//memset
#include <iostream>
#include <signal.h>
#include <sstream>
#include <unistd.h>		//close;
#include <vector>

#include "ArgumentHandler.h"
#include "Hardware/HardwareHandler.h"
#include "Languages.h"
#include "Logger/Logger.h"
#include "Manager.h"
#include "Network/Select.h"
#include "RailControl.h"
#include "Version.h"
#include "Utils/Utils.h"

using std::vector;
using std::string;

void killRailControlIfNeeded(Logger::Logger* logger)
{
	if (++stopSignalCounter < MaxStopSignalCounter)
	{
		return;
	}
	logger->Info(Languages::TextReceivedSignalKill, MaxStopSignalCounter);
	exit(1);
}

void shutdownRailControlSignal(int signo)
{
	Logger::Logger* logger = Logger::Logger::GetLogger("Main");
	logger->Info(Languages::TextShutdownRequestedBySignal, signo);
	killRailControlIfNeeded(logger);
}

void shutdownRailControlWebserver()
{
	Logger::Logger* logger = Logger::Logger::GetLogger("Main");
	logger->Info(Languages::TextShutdownRequestedByWebClient);
	killRailControlIfNeeded(logger);
}

int main (int argc, char* argv[])
{
	std::map<std::string,char> argumentMap;
	argumentMap["config"] = 'c';
	argumentMap["daemonize"] = 'd';
	argumentMap["logfile"] = 'l';
	argumentMap["help"] = 'h';
	argumentMap["silent"] = 's';
	ArgumentHandler argumentHandler(argc, argv, argumentMap, 'c');

	const bool help = argumentHandler.GetArgumentBool('h');
	if (help)
	{
		std::cout << "Usage: " << argv[0] << " <options>" << std::endl;
		std::cout << "Options:" << std::endl;
		std::cout << "   --config=ConfigFile   Read config file with file name ConfigFile (default ConfigFile: railcontrol.conf)" << std::endl;
		std::cout << "-d --daemonize           Daemonize RailControl. Implies -s" << std::endl;
		std::cout << "   --logfile=LogFile     Write a logfile to file LogFile (default LogFile: railcontrol.log)" << std::endl;
		std::cout << "-h --help                Show this help" << std::endl;
		std::cout << "-s --silent              Omit writing to console" << std::endl;
		return EXIT_SUCCESS;
	}

	const bool daemonize = argumentHandler.GetArgumentBool('d');
	if (daemonize)
	{
		pid_t pid = fork();
		if (pid > 0)
		{
			return EXIT_SUCCESS;
		}
		close(STDERR_FILENO);
		close(STDOUT_FILENO);
		close(STDIN_FILENO);
	}

	stopSignalCounter = 0;
	signal(SIGINT, shutdownRailControlSignal);
	signal(SIGTERM, shutdownRailControlSignal);

	const string RailControl = "RailControl";
	Utils::Utils::SetThreadName(RailControl);

	Logger::Logger* logger = Logger::Logger::GetLogger("Main");

	const bool silent = daemonize || argumentHandler.GetArgumentBool('s');
	if (!silent)
	{
		logger->AddConsoleLogger();
	}

	const string logFileName = argumentHandler.GetArgumentString('l', "railcontrol.log");
	if (logFileName.length() > 0)
	{
		logger->AddFileLogger(logFileName);
	}

	logger->Info(Languages::TextStarting, RailControl);
	logger->Info(Languages::TextVersion, GetVersionInfoRailControlVersion());
	logger->Info(Languages::TextCompileDate, Utils::Utils::TimestampToDate(GetVersionInfoCompileTimestamp()));
	logger->Info(Languages::TextGitHash, GetVersionInfoGitHash());
	logger->Info(Languages::TextGitDate, Utils::Utils::TimestampToDate(GetVersionInfoGitTimestamp()));

	const unsigned int changedFiles = GetVersionInfoGitDirty();
	if (changedFiles)
	{
		logger->Info(Languages::TextGitDirty, changedFiles);
	}

	logger->Info(Languages::TextStartArgument, argv[0]);

	const string configFileDefaultName("railcontrol.conf");
	string configFileName = argumentHandler.GetArgumentString('c', configFileDefaultName);
	if (configFileName.compare("") == 0)
	{
		configFileName = configFileDefaultName;
	}

	if (!Utils::Utils::FileExists(configFileName))
	{
		logger->Warning(Languages::TextConfigFileNotFound, configFileName, configFileDefaultName);
		configFileName = configFileDefaultName;
	}

	if (configFileName.compare(configFileDefaultName) == 0 && !Utils::Utils::FileExists(configFileDefaultName))
	{
		Utils::Utils::CopyFile(logger, "railcontrol.conf.dist", configFileDefaultName);
	}

	Config config(configFileName);

	unsigned int logKeepBackups = config.getIntValue("logkeepbackups", 10);
	Utils::Utils::RemoveOldBackupFiles(logger, logFileName, logKeepBackups);

	char input = 0;
	{
		// the main program is running in the manager.
		Manager m(config);

		// wait for q or r followed by \n or SIGINT or SIGTERM
		do
		{
			if (silent)
			{
				Utils::Utils::SleepForSeconds(1);
			}
			else
			{
				struct timeval tv;
				tv.tv_sec = 1;
				tv.tv_usec = 0;
				fd_set set;
				FD_ZERO(&set);
				FD_SET(STDIN_FILENO, &set);
				int ret = TEMP_FAILURE_RETRY(select(FD_SETSIZE, &set, NULL, NULL, &tv));
				if (ret > 0 && FD_ISSET(STDIN_FILENO, &set))
				{
					__attribute__((unused)) size_t unused = read(STDIN_FILENO, &input, sizeof(input));
				}
			}
		} while ((input != 'q') && (input != 'r') && !isShutdownRunning());

		logger->Info(Languages::TextShutdownRailControl);

	}	// here the destructor of manager is called and RailControl is shut down

	if (input == 'r')
	{
		// restart RailControl
		return execv(argv[0], argv);
	}
	else
	{
		// exit RailControl
		return EXIT_SUCCESS;
	}
}
