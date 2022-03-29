#ifndef LOGGING_H
#define LOGGING_H

#include "wnlogstream.h"


class Logger {
public:
    enum LogLevel //日志级别
    {
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS,
    };

    class SourceFile {
    public:
        // 可接受可变的const char [N]
        template <int N>
        SourceFile(const char (&arr)[N]): data_(arr), size_(N - 1)
        {
            const char* slash = strrchr(data_, '/'); 
            if (slash) {
                data_ = slash + 1;
                size_ -= static_cast<int>(data_ - arr);
            }
        }

        explicit SourceFile(const char* filename): data_(filename)
        {
            const char* slash = strrchr(filename, '/');
            if (slash) {
                data_ = slash + 1;
            }
            size_ = static_cast<int>(strlen(data_));
        }

        const char* data_;
        int size_;
    };

    Logger(SourceFile file, int line, LogLevel level, const char* func);
    // 错误流用
    Logger(SourceFile file, int line, bool toAbort, const char* func);
    ~Logger();

    LogStream& stream() { return impl_.stream_; }

    typedef void (*OutputFunc)(const char* msg, int len);
    typedef void (*FlushFunc)();
    static void setOutput(OutputFunc);
    static void setFlush(FlushFunc);

private:
    // 用来格式化固定输出的类
    class Impl {
    public:
        typedef Logger::LogLevel LogLevel;
        Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
        void formatTime(); //格式化时间
        void finish(); // 用于析构函数将缓存写入流文件

        std::string time_;
        LogStream stream_;
        LogLevel level_;
        int line_;
        SourceFile basename_;
    };

    Impl impl_;

}; // class Logger

//
// CAUTION: do not write:
//
// if (good)
//   LOG_INFO << "Good news";
// else
//   LOG_WARN << "Bad news";
//
// this expends to
//
// if (good)
//   if (logging_INFO)
//     logInfoStream << "Good news";
//   else
//     logWarnStream << "Bad news";
//

#define LOG_DEBUG Logger(__FILE__, __LINE__, Logger::DEBUG, __func__).stream()
#define LOG_INFO Logger(__FILE__, __LINE__, Logger::INFO, __func__).stream()
#define LOG_WARN Logger(__FILE__, __LINE__, Logger::WARN, __func__).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::ERROR, __func__).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::FATAL, __func__).stream()
#define LOG_SYSERR Logger(__FILE__, __LINE__, false, __func__).stream()
#define LOG_SYSFATAL Logger(__FILE__, __LINE__, true, __func__).stream()

// Taken from glog/logging.h
//
// Check that the input is non NULL.  This very useful in constructor
// initializer lists.

#define CHECK_NOTNULL(val) \
    CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

// A small helper for CHECK_NOTNULL().
template <typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char* names, T* ptr)
{
    if (ptr == NULL) {
        Logger(file, line, Logger::FATAL, __func__).stream() << names;
    }
    return ptr;
}



#endif // LOGGING_H
