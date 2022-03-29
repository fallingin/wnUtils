
#ifndef LOGSTREAM_H
#define LOGSTREAM_H

#include <cassert>
#include <cstring> // memcpy
#include <string>


class noncopyable {
public:
    noncopyable(const noncopyable&) = delete;
    void operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

template <typename To, typename From>
inline To implicit_cast(From const& f)
{
    return f;
}
namespace detail {

    constexpr int kSmallBuffer = 4000;
    constexpr int kLargeBuffer = 4000 * 1000;

    template <int SIZE>
    class FixedBuffer : noncopyable {
    public:
        FixedBuffer(): cur_(data_)
        {
        }

        void append(const char* buf, size_t len)
        {
            // FIXME: append partially
            if (implicit_cast<size_t>(avail()) > len) {
                memcpy(cur_, buf, len);
                cur_ += len;
            }else{
                int ava = avail();
                memcpy(cur_, buf, ava);
                cur_ += ava;
            }
        }

        const char* data() const { return data_; }
        int length() const { return static_cast<int>(cur_ - data_); }

        // write to data_ directly
        char* current() { return cur_; }
        int avail() const { return static_cast<int>(end() - cur_); }
        void add(size_t len) { cur_ += len; }

        void reset() { cur_ = data_; } //惰性删除，同redis
        void bzero() { memset(data_, 0, sizeof data_); }

        // for used by unit test
        std::string toString() const { return std::string(data_, length()); }

    private:
        const char* end() const { return data_ + sizeof data_; }


        char data_[SIZE];
        char* cur_;
    };

} // namespace detail

class LogStream : noncopyable {
    typedef LogStream self;

public:
    typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;

    self& operator<<(bool v)
    {
        buffer_.append(v ? "1" : "0", 1);
        return *this;
    }
    self& operator<<(char v)
    {
        buffer_.append(&v, 1);
        return *this;
    }
    self& operator<<(float v)
    {
        *this << static_cast<double>(v);
        return *this;
    }

    self& operator<<(short);
    self& operator<<(unsigned short);
    self& operator<<(int);
    self& operator<<(unsigned int);
    self& operator<<(long);
    self& operator<<(unsigned long);
    self& operator<<(long long);
    self& operator<<(unsigned long long);

    self& operator<<(const void*);

    
    self& operator<<(double);
    // self& operator<<(long double);
    // self& operator<<(signed char);
    // self& operator<<(unsigned char);

    self& operator<<(const char* str)
    {
        if (str) {
            buffer_.append(str, strlen(str));
        } else {
            buffer_.append("(null)", 6);
        }
        return *this;
    }

    self& operator<<(const unsigned char* str)
    {
        return operator<<(reinterpret_cast<const char*>(str));
    }

    self& operator<<(const std::string& v)
    {
        buffer_.append(v.c_str(), v.size());
        return *this;
    }

    void append(const char* data, int len) { buffer_.append(data, len); }
    const Buffer& buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }

private:
    void staticCheck();

    template <typename T>
    void formatInteger(T);

    Buffer buffer_;

    static const int kMaxNumericSize = 32;
};

#endif // LOGSTREAM_H
