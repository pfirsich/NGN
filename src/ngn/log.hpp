#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <iostream>

namespace ngn {
    /*
     * I thought about having two hardcoded handlers (for file and console)
     * with changeable log level and enabling, but it is not much more work
     * to make it a variable number of handlers, so I'll do that
     *
     * I also considered having the log handlers get all the information the log
     * command gets, so that a game may later implement a log handler, which
     * can then distinguish the errors and decide what to do in a game specific way
     * but I also had to include a short unique identifier for each error (which is bothersome)
     * Also I decided that for debugging console/file is enough and in release
     * the log can contain further info and the game itself only has to say that
     * *something* went wrong and then only decide if it wants to keep going or not
     * and possibly close
     *
     * Also I don't want the handlers to mess up the formatting and have
     * different formatting through different handlers
     */

    //TODO: Thread safety

    const unsigned LOGLVL_DEBUG = 0;
    const unsigned LOGLVL_INFO = 1;
    const unsigned LOGLVL_WARNING = 2;
    const unsigned LOGLVL_ERROR = 3;
    const unsigned LOGLVL_CRITICAL = 4;

    extern const char* levelMap[5];

    class LoggingHandler {
    private:
        unsigned mLogLevel;
    public:
        LoggingHandler() : mLogLevel(LOGLVL_DEBUG) {}
        virtual ~LoggingHandler() {}

        void setLogLevel(unsigned level) {mLogLevel = level;}
        unsigned getLogLevel() const {return mLogLevel;}

        virtual void log(unsigned level, const char* str) = 0;
    };

    class FileLoggingHandler : public LoggingHandler {
    private:
        std::string filename;
    public:
        FileLoggingHandler(const char* filename, unsigned level = LOGLVL_DEBUG) : filename(filename) {
            setLogLevel(level);
        }
        ~FileLoggingHandler() {}

        void log(unsigned level, const char* str);
    };

    class ConsoleLoggingHandler : public LoggingHandler {
    public:
        ConsoleLoggingHandler(unsigned level = LOGLVL_DEBUG) {setLogLevel(level);}
        ~ConsoleLoggingHandler() {}

        void log(unsigned level, const char* str) {
            std::cout << str << std::endl;
        }
    };

    extern std::vector<LoggingHandler*> loggingHandlers;
    extern std::string loggingFormat;

    void setupDefaultLogging();

    void log(unsigned level, const char* filename, int line, const char* format, ...);

    // http://stackoverflow.com/questions/5588855/standard-alternative-to-gccs-va-args-trick
    #define LOG_DEBUG(_msg, ...) ngn::log(ngn::LOGLVL_DEBUG, __FILE__, __LINE__, _msg, ##__VA_ARGS__)
    #define LOG_INFO(_msg, ...) ngn::log(ngn::LOGLVL_INFO, __FILE__, __LINE__, _msg, ##__VA_ARGS__)
    #define LOG_WARNING(_msg, ...) ngn::log(ngn::LOGLVL_WARNING, __FILE__, __LINE__, _msg, ##__VA_ARGS__)
    #define LOG_ERROR(_msg, ...) ngn::log(ngn::LOGLVL_ERROR, __FILE__, __LINE__, _msg, ##__VA_ARGS__)
    #define LOG_CRITICAL(_msg, ...) ngn::log(ngn::LOGLVL_CRITICAL, __FILE__, __LINE__, _msg, ##__VA_ARGS__)
}
