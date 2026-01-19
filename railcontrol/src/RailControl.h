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

#pragma once

#include <signal.h>

static volatile unsigned char stopSignalCounter;
static const unsigned char MaxStopSignalCounter = 3;


void killRailControlIfNeeded(Logger::Logger* logger);

void shutdownRailControlSignal(int);

void shutdownRailControlWebserver();

void segvHandler(int, siginfo_t*, void*);

inline bool isShutdownRunning()
{
	return stopSignalCounter > 0;
}

inline bool isKillRunning()
{
	return stopSignalCounter > 1;
}

char readFromStdIn(const time_t sec, const long int usec);
