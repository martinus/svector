#ifndef ANKERL_SVECTOR_H
#define ANKERL_SVECTOR_H

#include <algorithm> // max
#include <array>
#include <cstddef> // size_t, byte
#include <cstdint> // uintptr_t
#include <new>

namespace ankerl {

namespace detail {

class SmallVectorHeader {
    size_t m_size{};
    size_t const m_capacity;

public:
    inline explicit SmallVectorHeader(size_t capacity)
        : m_capacity{capacity} {}

    [[nodiscard]] inline auto size() const -> size_t {
        return m_size;
    }

    [[nodiscard]] inline auto capacity() const -> size_t {
        return m_capacity;
    }

    inline void size(size_t s) {
        m_size = s;
    }
};

template <typename T>
struct SmallVectorStorage : public SmallVectorHeader {
    static constexpr auto alignmentOfT = std::alignment_of_v<T>;
    static constexpr auto maxAlignment = std::max(std::alignment_of_v<SmallVectorHeader>, std::alignment_of_v<T>);
    static constexpr auto offsetToData = ((sizeof(SmallVectorHeader) + alignmentOfT - 1) / alignmentOfT) * alignmentOfT;
    static_assert(maxAlignment <= __STDCPP_DEFAULT_NEW_ALIGNMENT__);

    explicit SmallVectorStorage(size_t capacity)
        : SmallVectorHeader(capacity) {}

    auto data() -> T* {
        auto ptrToData = reinterpret_cast<std::byte*>(this) + offsetToData;
        return reinterpret_cast<T*>(ptrToData);
    }

    auto data() const -> T const* {
        auto ptrToData = reinterpret_cast<std::byte*>(this) + offsetToData;
        return reinterpret_cast<T*>(ptrToData);
    }

    static auto alloc(size_t capacity) -> SmallVectorStorage<T>* {
        // only void* is allowed to be converted to uintptr_t
        void* ptr = ::operator new(offsetToData + sizeof(T) * capacity);

        // make sure the lowest significant bit is 0
        auto intPtr = reinterpret_cast<std::uintptr_t>(ptr);
        if ((intPtr & 1U) != 0U) {
            throw std::bad_alloc(); // TODO use a better fitting exception?
        }

        return new (ptr) SmallVectorStorage<T>(capacity);
    }
};

} // namespace detail

template <typename T, size_t N>
class svector {
    struct Direct {
        // TODO what about big endianness?
        uint8_t m_is_direct : 1;
        uint8_t m_size : 7;
        // see https://stackoverflow.com/a/71828512/48181
        alignas(T) std::array<std::byte, sizeof(T) * N> m_buffer;
    };
    union {
        Direct m_direct;
        detail::SmallVectorStorage<T>* m_indirect;
    } m_union;

    [[nodiscard]] auto is_direct() const -> bool {
        return m_union.m_direct.m_is_direct;
    }

public:
    svector() {
        m_union.m_direct.m_is_direct = 1;
    };

    auto reserve(size_t s) {
        m_union.m_indirect = detail::SmallVectorStorage<T>::alloc(s);
    }

    [[nodiscard]] auto capacity() const -> size_t {
        if (is_direct()) {
            return N;
        }
        return m_union.m_indirect->m_capacity;
    }

    [[nodiscard]] auto size() const -> size_t {
        if (is_direct()) {
            return m_union.m_direct.m_size;
        }
        return m_union.m_indirect->m_size;
    }

    [[nodiscard]] auto data() -> T* {
        if (is_direct()) {
            return reinterpret_cast<T*>(m_union.m_direct.m_buffer);
        }
        return m_union.m_indirect->data();
    }

    [[nodiscard]] auto data() const -> T const* {
        if (is_direct()) {
            return reinterpret_cast<T const*>(m_union.m_direct.m_buffer);
        }
        return m_union.m_indirect->data();
    }
};

} // namespace ankerl

#endif