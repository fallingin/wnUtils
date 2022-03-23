#ifndef WNSTRING_H
#define WNSTRING_H

// small strings（SSO）时，使用 union 中的 Char small_存储字符串，即对象本身的栈空间。

// medium strings（eager copy）时，使用 union 中的 MediumLarge ml_

// large strings（cow）时， 使用MediumLarge ml_和 RefCounted：
// ml*.data_指向 RefCounted.data，ml*.size_与 ml.capacity_的含义不变。

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
    Char data_[1]; // flexible array. 存放字符串。

    // 创建一个RefCounted
    static RefCounted* create(size_t* size);
    static RefCounted* create(const Char* data, size_t* size);
    static void incrementRefs(Char* p); // 增加一个引用
    static void decrementRefs(Char* p); // 减少一个引用

    // 其他函数定义
};

struct MediumLarge {
    Char* data_;
    size_t size_; // 字符串长度
    size_t capacity_; // 字符串容量

    size_t capacity() const
    {
        return capacity_;
    }

    void setCapacity(size_t cap, Category cat)
    {
        capacity_ = cap | (static_cast<size_t>(cat) << kCategoryShift);
    }
};

union {
    uint8_t bytes_[sizeof(MediumLarge)]; // 配合 lastChar 更加方便的取该结构最后一个字节（字符串种类）
    Char small_[sizeof(MediumLarge) / sizeof(Char)];
    MediumLarge ml_;
};

#endif // WNSTRING_H