#include "Logger.h"


namespace TopGear
{
	std::unique_ptr<Logger> Logger::instance;
	const std::string Logger::LoggerName = "default";

	Logger::Logger()
		: dist_sink(std::make_shared<spdlog::sinks::dist_sink_mt>()),
		std_sink(std::make_shared<spdlog::sinks::stdout_sink_mt>())
#ifdef __linux__
        ,sys_sink()
#endif
	{
		try
		{
			spdlog::set_async_mode(1048576, spdlog::async_overflow_policy::discard_log_msg,
				nullptr, std::chrono::milliseconds(1000)); //queue size must be power of 2
			logger = std::make_shared<spdlog::logger>(LoggerName, dist_sink);
			spdlog::register_logger(logger);
			dist_sink->add_sink(std_sink);
			std_sink_en = true;
			spdlog::set_level(spdlog::level::off);
		}
		catch (const spdlog::spdlog_ex&)
		{
		}
	}

	void Logger::Write(spdlog::level::level_enum level, const std::string &text)
	{
		if (instance == nullptr)
			return;
		switch (level)
		{
		case spdlog::level::trace: 
			instance->logger->trace(text);
			break;
		case spdlog::level::debug:
			instance->logger->debug(text);
			break;
		case spdlog::level::info:
			instance->logger->info(text);
			break;
		case spdlog::level::notice:
			instance->logger->notice(text);
			break;
		case spdlog::level::warn:
			instance->logger->warn(text);
			break;
		case spdlog::level::err:
			instance->logger->error(text);
			break;
		case spdlog::level::critical:
			instance->logger->critical(text);
			break;
		case spdlog::level::alert:
			instance->logger->alert(text);
			break;
		case spdlog::level::emerg:
			instance->logger->emerg(text);
			break;
		default: break;
		}
		//instance->logger->flush();
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
		if (enable == instance->std_sink_en)
			return true;
		auto sink = std::static_pointer_cast<spdlog::sinks::sink>(instance->std_sink);
		return instance->ConfigSink(enable, sink, instance->std_sink_en);
	}

	bool Logger::SwitchDaily(bool enable)
	{
		if (instance == nullptr)
			return false;
		if (enable == instance->daily_sink_en)
			return true;
		if (enable && instance->daily_sink == nullptr)
			instance->daily_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("daily", "log", 0, 0);
		auto sink = std::static_pointer_cast<spdlog::sinks::sink>(instance->daily_sink);
		return instance->ConfigSink(enable, sink, instance->daily_sink_en);
	}

#ifdef __linux__
	bool Logger::SwitchSyslog(bool enable)
	{
		if (instance == nullptr)
			return false;
		if (enable == instance->sys_sink_en)
			return true;
		auto sink = std::static_pointer_cast<spdlog::sinks::sink>(instance->sys_sink);
		return instance->ConfigSink(enable, sink, instance->sys_sink_en);
	}
#endif
}
