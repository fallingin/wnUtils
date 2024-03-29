

#include "wnlogging.h"

#include <cerrno>
#include <cstdio>
#include <ctime>

#include <iostream>
#include <sstream>
#include <thread>


const char* LogLevelName[Logger::NUM_LOG_LEVELS] = {
    " DEBUG ",
    " INFO  ",
    " WARN  ",
    " ERROR ",
    " FATAL ",
};

// 协助类：为了在编译时获得字符串长度
class T {
public:
    T(const char* str, unsigned len): str_(str), len_(len)
    {
        assert(strlen(str) == len_);
    }

    const char* str_;
    const unsigned len_;
};

inline LogStream& operator<<(LogStream& s, T v)
{
    s.append(v.str_, v.len_);
    return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v)
{
    s.append(v.data_, v.size_);
    return s;
}

void defaultOutput(const char* msg, int len)
{
    size_t n = fwrite(msg, 1, len, stdout);
    // 如果流中的字符串长度为空
    if(n == 0){

    }
    // 写入失败（写入的长度跟实际长度不同）
    if(n != len){

    }
}

void defaultFlush()
{
    // 刷新stdout写入文件
    fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;


Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
    : stream_(), level_(level), line_(line), basename_(file)
{
    formatTime();

    stream_ << T(LogLevelName[level], 7);

    std::thread::id threadId = std::this_thread::get_id();
    int* pi = (int*)&threadId;
    stream_ << "[threadID:" << *pi << "] ";

    if (savedErrno != 0) {
        stream_ << strerror(savedErrno) << "(errno=" << savedErrno << ") ";
    }
}

void Logger::Impl::formatTime()
{
    time_t t = time(0);
    char tmp[11];
    strftime(tmp, sizeof(tmp), "[%H:%M:%S]", localtime(&t));
    time_ = tmp;
    stream_ << time_;
}

void Logger::Impl::finish()
{
    if (level_ == Logger::DEBUG)
        stream_ << " [" << __DATE__ << " " << __TIME__ << "]"; //编译时间
    stream_ << " --" << basename_ << ':' << line_ << '\n';
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
    : impl_(level, 0, file, line)
{
    impl_.stream_ << "[fuc:" << func << "] msg: ";
}

Logger::Logger(SourceFile file, int line, bool toAbort, const char* func)
    : impl_(toAbort ? FATAL : ERROR, errno, file, line)
{
    impl_.stream_ << "[fuc:" << func << "] msg: ";
}

Logger::~Logger()
{
    impl_.finish();
    const LogStream::Buffer& buf(stream().buffer());
    g_output(buf.data(), buf.length());
    if (impl_.level_ == FATAL) {
        g_flush();
        abort();
    }
}

void Logger::setOutput(OutputFunc out)
{
    g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
    g_flush = flush;
}
