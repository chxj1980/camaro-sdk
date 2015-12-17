#pragma once
#include "spdlog/spdlog.h"
#include "spdlog/sinks/dist_sink.h"
#include "spdlog/sinks/syslog_sink.h"

#include <string>

namespace TopGear
{
	class Logger
	{
	public:
		static void Initialize()
		{
			if (instance == nullptr)
				instance = std::unique_ptr<Logger>(new Logger);
		}

		//static std::shared_ptr<spdlog::logger> &Instance()
		//{
		//	return instance->logger;
		//}

		static void Write(spdlog::level::level_enum level, const std::string &text);

		static bool SwitchStdout(bool enable);
		static bool SwitchDaily(bool enable);
#ifdef __linux__
		static bool SwitchSyslog(bool enable);
#endif
		~Logger() {}
	private:
		Logger();
		std::shared_ptr<spdlog::sinks::dist_sink_mt> dist_sink;
		std::shared_ptr<spdlog::sinks::stdout_sink_mt> std_sink;
		std::shared_ptr<spdlog::sinks::daily_file_sink_mt> daily_sink;
		std::shared_ptr<spdlog::logger> logger;
		bool std_sink_en = false;
		bool daily_sink_en = false;
#ifdef __linux__
		std::shared_ptr<spdlog::sinks::syslog_sink> sys_sink;
		bool sys_sink_en = false;
#endif
		bool ConfigSink(bool enable, spdlog::sink_ptr &sink, bool &original) const;
		static std::unique_ptr<Logger> instance;
		static const std::string LoggerName;
	};


}