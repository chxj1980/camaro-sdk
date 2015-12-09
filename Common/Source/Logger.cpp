#include "Logger.h"


namespace TopGear
{
	std::unique_ptr<Logger> Logger::instance;
	const std::string Logger::LoggerName = "default";

	Logger::Logger()
		: dist_sink(std::make_shared<spdlog::sinks::dist_sink_mt>()),
		std_sink(std::make_shared<spdlog::sinks::stdout_sink_mt>()),
		daily_sink(std::make_shared<spdlog::sinks::daily_file_sink_mt>("daily", "log", 0, 0))
#ifdef __linux__
		,sys_sink(std::make_shared<spdlog::sinks::syslog_sink_mt>())
#endif
	{
		try
		{
			spdlog::set_async_mode(1048576); //queue size must be power of 2
			logger = std::make_shared<spdlog::logger>(LoggerName, dist_sink);
			spdlog::register_logger(logger);
			
		}
		catch (const spdlog::spdlog_ex&)
		{
		}
	}

	bool Logger::ConfigSink(bool enable, spdlog::sink_ptr &sink, bool &original) const
	{
		if (enable)
		{
			if (original)
				return false;
			original = true;
			dist_sink->add_sink(sink);
		}
		else
		{
			if (!original)
				return false;
			original = false;
			dist_sink->remove_sink(sink);
		}
		return true;
	}

	bool Logger::SwitchStdout(bool enable)
	{
		if (instance == nullptr)
			return false;
		auto sink = std::static_pointer_cast<spdlog::sinks::sink>(instance->std_sink);
		return instance->ConfigSink(enable, sink, instance->std_sink_en);
	}

	bool Logger::SwitchDaily(bool enable)
	{
		if (instance == nullptr)
			return false;
		auto sink = std::static_pointer_cast<spdlog::sinks::sink>(instance->daily_sink);
		return instance->ConfigSink(enable, sink, instance->daily_sink_en);
	}

#ifdef __linux__
	bool Logger::SwitchSyslog(bool enable)
	{
		if (instance == nullptr)
			return false;
		auto sink = std::static_pointer_cast<spdlog::sinks::sink>(instance->sys_sink);
		return instance->ConfigSink(enable, sink, instance->sys_sink_en);
	}
#endif
}