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

#include <sstream>
#include <string>

#include "Server/Web/Response.h"

namespace Server { namespace Web
{
	const Response::responseCodeMap Response::responseTexts = {
		{ Response::OK, "OK" },
		{ Response::NotFound, "Not found"},
		{ Response::NotImplemented, "Not Implemented"}
	};

	void Response::AddHeader(const std::string& key, const std::string& value)
	{
		headers[key] = value;
	}

	Response::operator std::string()
	{
		std::stringstream reply;
		reply << *this;
		return reply.str();
	}

	std::ostream& operator<<(std::ostream& stream, const Response& response)
	{
		stream << "HTTP/1.1 " << std::to_string(response.responseCode) << " " << Response::responseTexts.at(response.responseCode) << "\r\n";
		for (auto& header : response.headers)
		{
			stream << header.first << ": " << header.second << "\r\n";
		}
		stream << "\r\n";
		stream << response.content;
		return stream;
	}
}} // namespace Server::Web
