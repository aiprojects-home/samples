#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <iostream>
#include <ctime>
#include <chrono>

namespace aux
{
	class Logger
	{
	private:

		static std::unique_ptr<Logger> m_Logger;
		static std::mutex              m_Mutex;
		static std::ostream*           m_pLogStream;

	private:
		Logger()
		{
			// private ctor
		}
    
	public:
		~Logger()
		{
			// public dtor
		}

	public:
		static Logger& Get()
		{
			if (m_Logger == nullptr)
			{
				m_Logger = std::unique_ptr<Logger>(new Logger());
			};

			return *(m_Logger);
		}

		Logger& operator << (const std::string& s)
		{
			{
				std::lock_guard<std::mutex> lock(m_Mutex);
				
				(*m_pLogStream) << s;
			}

			return *this;
		}

		std::string Now()
		{
			char timeBuf[0x100];

			std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		    ctime_s(timeBuf, sizeof(timeBuf), &t);

			std::string ts = timeBuf;
			ts.resize(ts.size() - 1);
			
			return ts;
		}

		void SetLogStream(std::ostream& stream)
		{
			m_pLogStream = &stream;
		}
	};
};

