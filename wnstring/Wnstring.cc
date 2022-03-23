#include "wnstring.h"
#include <cstring>
#include <cassert>
#include <sys/types.h> // for ssize_t

Wnstring::Wnstring(const char* const data, const size_t size, bool disableSSO)
{
    if (!disableSSO && size <= maxSmallSize) {
        initSmall(data, size);
    } else if (size <= maxMediumSize) {
        initMedium(data, size);
    } else {
        initLarge(data, size);
    }
}
// 首先，如果传入的字符串地址是内存对齐的，则配合 reinterpret_cast 进行 word-wise copy，提高效率。
// 否则，调用 podCopy 进行 memcpy。
// 最后，通过 setSmallSize 设置 small string 的 size。
void Wnstring::initSmall(const char* const data, const size_t size)
{
    if ((reinterpret_cast<size_t>(data) & (sizeof(size_t) - 1)) == 0) {
        const size_t byteSize = size * sizeof(char);
        constexpr size_t wordWidth = sizeof(size_t);
        switch ((byteSize + wordWidth - 1) / wordWidth) { // Number of words.
        case 3:
            ml_.capacity_ = reinterpret_cast<const size_t*>(data)[2];
            break;
        case 2:
            ml_.size_ = reinterpret_cast<const size_t*>(data)[1];
            break;
        case 1:
            ml_.data_ = *reinterpret_cast<char**>(const_cast<char*>(data));
            break;
        case 0:
            break;
        }
    } else {
        if (size != 0) {
            memcpy(small_,data,  size);
        }
    }
    setSmallSize(size);
}

void Wnstring::initMedium(const char* const data, const size_t size)
{
    auto const allocSize = (1 + size) * sizeof(char);
    ml_.data_ = new char[allocSize];
    memcpy(ml_.data_,data,  size);
    ml_.size_ = size;
    ml_.setCapacity(allocSize - 1, Category::isMedium);
    ml_.data_[size] = '\0';
}
void Wnstring::initLarge(const char* const data, const size_t size)
{
    // size_t effectiveCapacity = size;
    // auto const newRC = RefCounted::create(data, &effectiveCapacity);
    // ml_.data_ = newRC->data_;
    // ml_.size_ = size;
    // ml_.setCapacity(effectiveCapacity, Category::isLarge);
    // ml_.data_[size] = '\0';
}
size_t Wnstring::size() const
{
    size_t ret = ml_.size_;
    // 如果不是small类型，那么union最后一个字节存的是类型10000000或01000000，它们都必然大于maxSmallSize
    typedef typename std::make_unsigned<char>::type UChar;
    auto maybeSmallSize = size_t(maxSmallSize) - size_t(static_cast<UChar>(small_[maxSmallSize]));
    // 使用这个语法, GCC 和 Clang 会生成 a CMOV（条件传送指令） 而不会进行 “处理器分支预测” 这一步，减少控制冒险
    ret = (static_cast<ssize_t>(maybeSmallSize) >= 0) ? maybeSmallSize : ret;

    return ret;
}
// small strings : 直接返回 maxSmallSize。
// medium strings : 返回 ml_.capacity()。
// large strings :
// 当字符串引用大于 1 时，直接返回 size。因为此时的 capacity 是没有意义的，任何 append data 操作都会触发一次 cow
// 否则，返回 ml_.capacity()。
size_t Wnstring::capacity() const
{
    switch (category()) {
    case Category::isSmall:
        return maxSmallSize;
    // case Category::isLarge:
    //     // For large-sized strings, a multi-referenced chunk has no
    //     // available capacity. This is because any attempt to append
    //     // data would trigger a new allocation.
    //     if (RefCounted::refs(ml_.data_) > 1) {
    //         return ml_.size_;
    //     }
    //     break;
    case Category::isMedium:
    default:
        break;
    }
    return ml_.capacity();
}
// small strings 存放的 size 不是真正的 size，是maxSmallSize - size
// 这样做的原因是可以 small strings 可以多存储一个字节
// 因为假如存储 size 的话，small中最后两个字节就得是\0 和 size，
// 但是存储maxSmallSize - size，当 size == maxSmallSize 时，
// small的最后一个字节恰好也是\0。
void Wnstring::setSmallSize(size_t s)
{
    assert(s <= maxSmallSize);
    small_[maxSmallSize] = char(maxSmallSize - static_cast<uint8_t>(s));
    small_[s] = '\0';
    assert(category() == Category::isSmall && size() == s);
}

// 获取字符串类型
Category Wnstring::category() const
{
    return static_cast<Category>(bytes_[lastChar] & categoryExtractMask);
}

Wnstring::~Wnstring()
{
}