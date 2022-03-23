#ifndef WNSTRING_H
#define WNSTRING_H
#include <atomic>
#include <cstddef>


// small strings（SSO）时，使用 union 中的 Char small_存储字符串，即对象本身的栈空间。

// medium strings（eager copy）时，使用 union 中的 MediumLarge ml_

// large strings（cow）时， 使用MediumLarge ml_和 RefCounted：
// ml*.data_指向 RefCounted.data，ml*.size_与 ml.capacity_的含义不变。

constexpr static uint8_t categoryExtractMask = 0xC0; // 11000000
constexpr static size_t kCategoryShift = (sizeof(size_t) - 1) * 8;
constexpr static size_t capacityExtractMask = ~(static_cast<size_t>(categoryExtractMask) << kCategoryShift);
constexpr static bool WNSTRING_DISABLE_SSO = false;

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

    // 创建一个RefCounted
    static RefCounted* create(size_t* size);
    static RefCounted* create(const char* data, size_t* size);
    static void incrementRefs(char* p); // 增加一个引用
    static void decrementRefs(char* p); // 减少一个引用

    // 其他函数定义
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
    Wnstring(const char* const data,const size_t size,bool disableSSO = WNSTRING_DISABLE_SSO);
    ~Wnstring();

private:
    union {
        uint8_t bytes_[sizeof(MediumLarge)]; // 配合 lastChar 更加方便的取该结构最后一个字节（字符串种类）
        char small_[sizeof(MediumLarge) / sizeof(char)];
        MediumLarge ml_;
    };

    void setSmallSize(size_t s);
    Category category() const;
    size_t capacity() const;
    size_t size() const;

    void initSmall(const char* const data, const size_t size);
    void initMedium(const char* const data, const size_t size);
    void initLarge(const char* const data, const size_t size);
};

#endif // WNSTRING_H