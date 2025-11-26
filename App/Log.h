#ifndef CLAYAPP_LOG_H
#define CLAYAPP_LOG_H

#include <iostream>
#include <sstream>
#include <atomic>
#include <ctime>

namespace Log {
    enum class Level { Error = 0, Warn = 1, Info = 2, Debug = 3 };

    inline const char* levelName(const Level l) {
        switch (l) {
            case Level::Error: return "ERROR";
            case Level::Warn:  return "WARN";
            case Level::Info:  return "INFO";
            case Level::Debug: return "DEBUG";
        }
        return "?";
    }

    inline std::atomic<int>& currentLevelRef() {
        static std::atomic<int> lvl{ static_cast<int>(Level::Info) };
        return lvl;
    }

    inline void setLevel(Level l) { currentLevelRef().store(static_cast<int>(l), std::memory_order_relaxed); }
    inline Level level() { return static_cast<Level>(currentLevelRef().load(std::memory_order_relaxed)); }
    inline bool enabled(Level l) { return static_cast<int>(l) <= currentLevelRef().load(std::memory_order_relaxed); }

    class Stream {
    public:
        Stream(Level lvl, const char* file, int line, const char* func) : m_level(lvl), m_file(file), m_line(line), m_func(func) {}
        ~Stream() {
            // timestamp minimal
            const std::time_t t = std::time(nullptr);
            char buf[20];
            std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
            std::ostream& out = (m_level == Level::Error || m_level == Level::Warn) ? std::cerr : std::cout;
            out << '[' << buf << "] " << levelName(m_level) << " " << m_file << ':' << m_line << " (" << m_func << ") - " << m_ss.str() << std::endl;
        }
        std::ostringstream& stream() { return m_ss; }
    private:
        Level m_level;
        const char* m_file;
        int m_line;
        const char* m_func;
        std::ostringstream m_ss;
    };
}

#define LOG_ERROR() if(!Log::enabled(Log::Level::Error)) ; else Log::Stream(Log::Level::Error, __FILE__, __LINE__, __func__).stream()
#define LOG_WARN()  if(!Log::enabled(Log::Level::Warn))  ; else Log::Stream(Log::Level::Warn,  __FILE__, __LINE__, __func__).stream()
#define LOG_INFO()  if(!Log::enabled(Log::Level::Info))  ; else Log::Stream(Log::Level::Info,  __FILE__, __LINE__, __func__).stream()
#define LOG_DEBUG() if(!Log::enabled(Log::Level::Debug)) ; else Log::Stream(Log::Level::Debug, __FILE__, __LINE__, __func__).stream()

#endif // CLAYAPP_LOG_H

