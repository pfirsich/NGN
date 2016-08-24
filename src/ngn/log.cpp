#include "log.hpp"
#include <ctime>
#include <map>
#include <cstdarg>
#include <cstdio>

namespace ngn {
    const char* levelMap[5] = {
        "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"
    };

    std::vector<LoggingHandler*> loggingHandlers;
    std::string loggingFormat("{levelname} [{y}.{m}.{d} {H}:{M}:{S}] [{filename}:{line}] - {message}");

    void setupDefaultLogging() {
        #ifdef DEBUG
        loggingHandlers.push_back(new ConsoleLoggingHandler());
        #endif
        loggingHandlers.push_back(new FileLoggingHandler("log.txt", LOGLVL_INFO));
    }

    std::string formatString(const std::string& format, const std::map<std::string, std::string>& argMap) {
        std::string ret = "";
        for(std::string::size_type i = 0; i < format.size(); ++i) {
            if(format[i] == '{') {
                if(format[i+1] == '{') {
                    ret += '{';
                    ++i;
                } else {
                    std::string::size_type keyStart = ++i;
                    std::string key = "";
                    for(; i < format.size(); ++i) {
                        if(format[i] == '}') {
                            if(format[i+1] == '}') {
                                ++i;
                            } else {
                                key = format.substr(keyStart, i - keyStart);
                                break;
                            }
                        }
                    }

                    auto arg = argMap.find(key);
                    if(arg != argMap.end()) {
                        ret += arg->second;
                    } else {
                        ret += "{UNKNOWN ARGUMENT:'" + key + "'}";
                    }
                }
            } else if(format[i] == '}') {
                if(format[i+1] == '}') {
                    ret += "}";
                } else {
                    // What do I do here?
                }
            } else {
                ret += format[i];
            }
        }
        return ret;
    }

    void log(unsigned level, const char* filename, int line, const char* format, ...) {
        std::map<std::string, std::string> formatArguments;
        formatArguments["levelname"] = level < 5 ? levelMap[level] : std::to_string(level);
        formatArguments["filename"] = filename;
        formatArguments["line"] = std::to_string(line);

        std::time_t t = std::time(nullptr);
        std::tm* tm = std::localtime(&t);
        formatArguments["d"] = std::to_string(tm->tm_mday);
        formatArguments["m"] = std::to_string(tm->tm_mon);
        formatArguments["y"] = std::to_string(tm->tm_year + 1900);
        formatArguments["H"] = std::to_string(tm->tm_hour);
        formatArguments["M"] = std::to_string(tm->tm_min);
        formatArguments["S"] = std::to_string(tm->tm_sec);

        std::string message;
        message = format;
        va_list args;
        va_start(args, format);
        // This might be slow
        // +1 for null-termination
        int len = vsnprintf(nullptr, 0, format, args) + 1;
        char* buffer = new char[len];
        vsnprintf(buffer, len, format, args);
        va_end(args);
        formatArguments["message"] = buffer;
        delete[] buffer;

        std::string logLine = formatString(loggingFormat, formatArguments);

        for(auto handler : loggingHandlers) {
            if(level >= handler->getLogLevel()) {
                handler->log(level, logLine.c_str());
            }
        }
    }

    void FileLoggingHandler::log(unsigned level, const char* str) {
        // TODO: do something here
    }
}