#ifndef WNSTRING_H
#define WNSTRING_H
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>

// small strings（SSO）时，使用 union 中的 Char small_存储字符串，即对象本身的栈空间。

// medium strings（eager copy）时，使用 union 中的 MediumLarge ml_

// large strings（cow）时， 使用MediumLarge ml_和 RefCounted：
// ml*.data_指向 RefCounted.data，ml*.size_与 ml.capacity_的含义不变。

constexpr static uint8_t categoryExtractMask = 0xC0; // 11000000
constexpr static size_t kCategoryShift = (sizeof(size_t) - 1) * 8;
constexpr static size_t capacityExtractMask = ~(static_cast<size_t>(categoryExtractMask) << kCategoryShift);
constexpr static bool WNSTRING_DISABLE_SSO = false;

inline void* checkedRealloc(void* ptr, size_t size)
{
    void* p = realloc(ptr, size);
    return p;
}
inline void* smartRealloc(void* p, const size_t currentSize, const size_t currentCapacity, const size_t newCapacity)
{
    assert(p);
    assert(currentSize <= currentCapacity && currentCapacity < newCapacity);
    auto const slack = currentCapacity - currentSize;
    if (slack * 2 > currentSize) {
        // Too much slack, malloc-copy-free cycle:
        auto const result = new char[newCapacity];
        std::memcpy(result, p, currentSize);
        delete (char*)p;
        return result;
    }
    // If there's not too much slack, we realloc in hope of coalescing
    return checkedRealloc(p, newCapacity);
}

typedef uint8_t category_type;
enum class Category : category_type {
    // 容量23
    isSmall = 0, // 00000000
    // 容量127
    isMedium = 0x80, // 10000000
    // 容量
    isLarge = 0x40, // 01000000
};

struct RefCounted {
    std::atomic<size_t> refCount_; // 共享字符串的引用计数
    char data_[1]; // flexible array. 存放字符串。

    // 获得data_的数据偏移，也是refCount_的首地址到data_首地址的长度
    constexpr static size_t getDataOffset()
    {
        return offsetof(RefCounted, data_);
    }
    // 创建一个RefCounted
    static RefCounted* create(size_t* size)
    {
        const size_t allocSize = getDataOffset() + (*size + 1) * sizeof(char);
        auto result = reinterpret_cast<RefCounted*>(new char[allocSize]);
        result->refCount_.store(1, std::memory_order_release);
        *size = (allocSize - getDataOffset()) - 1;
        return result;
    }
    static RefCounted* create(const char* data, size_t* size)
    {
        const size_t effectiveSize = *size;
        auto result = create(size);

        memcpy(result->data_, data, effectiveSize);
        return result;
    }
    // 从data获取RefCounted*
    // 转换不同类型结构体的指针并做运算，这里的做法是 ：
    // char* -> void* -> unsigned char* -> 与size_t做减法 -> void * -> RefCounted*
    static RefCounted* fromData(char* p)
    {
        return static_cast<RefCounted*>(static_cast<void*>(
            static_cast<unsigned char*>(static_cast<void*>(p)) - getDataOffset()));
    }
    // static RefCounted * fromData(Char * p) {
    //     // 转换data_[1]的地址
    //     void* voidDataAddr = static_cast<void*>(p);
    //     unsigned char* unsignedDataAddr = static_cast<unsigned char*>(voidDataAddr);

    //     // 获取data_[1]在结构体的偏移量再相减，得到的就是所属RefCounted的地址
    //     unsigned char* unsignedRefAddr = unsignedDataAddr - getDataOffset();

    //     void* voidRefAddr = static_cast<void*>(unsignedRefAddr);
    //     RefCounted* refCountAddr = static_cast<RefCounted*>(voidRefAddr);

    //     return refCountAddr;
    // }
    // 获得引用数量
    static size_t refs(char* p)
    {
        return fromData(p)->refCount_.load(std::memory_order_acquire);
    }
    // 增加一个引用
    static void incrementRefs(char* p)
    {
        fromData(p)->refCount_.fetch_add(1, std::memory_order_acq_rel);
    }
    // 减少一个引用
    static void decrementRefs(char* p)
    {
        auto const dis = fromData(p);
        size_t oldcnt = dis->refCount_.fetch_sub(1, std::memory_order_acq_rel);
        assert(oldcnt > 0);
        if (oldcnt == 1) {
            delete dis;
        }
    }
};

struct MediumLarge {
    char* data_;
    size_t size_; // 字符串长度
    size_t capacity_; // 字符串容量

    // 同时设置容量和字符串类型
    void setCapacity(size_t cap, Category cat)
    {
        // 用最高字节存储字符串类型，低7字节存容量
        capacity_ = cap | (static_cast<size_t>(cat) << kCategoryShift);
    }
    // 取字符串容量
    size_t capacity() const
    {
        return capacity_ & capacityExtractMask;
    }
};
constexpr static size_t lastChar = sizeof(MediumLarge) - 1;
constexpr static uint8_t maxSmallSize = sizeof(MediumLarge) - 1;
constexpr static uint8_t maxMediumSize = 0xFF; // 11111111(255)

class Wnstring {
public:
    Wnstring(const char* const data, const size_t size, bool disableSSO = WNSTRING_DISABLE_SSO);
    Wnstring(const Wnstring& rhs);
    ~Wnstring();

    size_t capacity() const;
    size_t size() const;
    const char* c_str() const;
    // size_type是由string类类型和vector类类型定义的类型，
    // 用于保存任意string对象或vector对象的长度
    char& operator[](size_t pos);
    const char& operator[](size_t pos) const;
    bool empty() const;

    Wnstring& operator=(const Wnstring& str); // fix
    Wnstring& operator=(const char* const s); // fix
    bool operator==(const Wnstring& str) const; // fix
    Wnstring operator+(const Wnstring& rhs); // fix

    void clear(); // fix
    size_t find(const char* const s, size_t pos = 0) const; // fix BM
    size_t find(const Wnstring& str, size_t pos = 0) const; // fix BM
    void pop_back(); // fix
    int compare(const Wnstring& str) const; // fix

private:
    union {
        uint8_t bytes_[sizeof(MediumLarge)]; // 配合 lastChar 更加方便的取该结构最后一个字节（字符串种类）
        char small_[sizeof(MediumLarge) / sizeof(char)];
        MediumLarge ml_;
    };

    void setSmallSize(size_t s);
    Category category() const;

    void initSmall(const char* const data, const size_t size);
    void initMedium(const char* const data, const size_t size);
    void initLarge(const char* const data, const size_t size);
    void copySmall(const Wnstring& rhs);
    void copyMedium(const Wnstring& rhs);
    void copyLarge(const Wnstring& rhs);
    void destroyMediumLarge();
    char* mutableDataLarge();
    void unshare(size_t minCapacity = 0);
};

#endif // WNSTRING_H