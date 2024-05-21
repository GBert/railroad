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

#include <condition_variable>
#include <list>
#include <mutex>

namespace Utils
{
	template<class T>
	class ThreadSafeQueue
	{
		public:
			inline ThreadSafeQueue()
			:	list(),
				mutex(),
				run(true)
			{
			}

			inline ~ThreadSafeQueue()
			{
				Terminate();
			}

			inline void EnqueueBack(T t)
			{
				std::unique_lock<std::mutex> lock(mutex);
				list.push_back(t);
				cv.notify_all();
			}

			inline void EnqueueFront(T t)
			{
				std::unique_lock<std::mutex> lock(mutex);
				list.push_front(t);
				cv.notify_all();
			}

			T Dequeue()
			{
				std::unique_lock<std::mutex> lock(mutex);
				while (list.empty())
				{
					if (run == false)
					{
						return T();
					}
					cv.wait_for(lock, std::chrono::seconds(1));
				}
				T val = list.front();
				list.pop_front();
				return val;
			}

			inline bool IsEmpty()
			{
				std::unique_lock<std::mutex> lock(mutex);
				return list.empty();
			}

			inline void Terminate()
			{
				run = false;
				cv.notify_all();
			}

		private:
			std::list<T> list;
			mutable std::mutex mutex;
			std::condition_variable cv;
			volatile bool run;
	};
}
