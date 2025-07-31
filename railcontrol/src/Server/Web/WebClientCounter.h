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

#pragma once

#include <map>
#include <string>

#include "Manager.h"

namespace Server { namespace Web
{
	class WebClient;

	class WebClientCounter
	{
		public:
			WebClientCounter() = delete;
			WebClientCounter(const WebClientCounter&) = delete;
			WebClientCounter& operator=(const WebClientCounter&) = delete;

			inline WebClientCounter(Manager& manager, WebClient& client)
			:	manager(manager),
				client(client)
			{
			}

			void HandleCounterEdit(const std::map<std::string,std::string>& arguments);
			void HandleCounterSave(const std::map<std::string,std::string>& arguments);
			void HandleCounterList();
			void HandleCounterAskDelete(const std::map<std::string,std::string>& arguments);
			void HandleCounterDelete(const std::map<std::string,std::string>& arguments);
			void HandleCounterGet(const std::map<std::string, std::string>& arguments);
			void HandleCounterIncrement(const std::map<std::string, std::string>& arguments);
			void HandleCounterDecrement(const std::map<std::string, std::string>& arguments);

		private:
			Manager& manager;
			WebClient& client;
	};
}} // namespace Server::Web

