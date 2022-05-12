#ifndef ANKERL_SVECTOR_H
#define ANKERL_SVECTOR_H

#include <algorithm> // max
#include <array>
#include <cstddef> // size_t, byte
#include <cstdint> // uintptr_t
#include <limits>
#include <new>

namespace ankerl {

namespace detail {

class header {
    size_t m_size{};
    size_t const m_capacity;

public:
    inline explicit header(size_t capacity)
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
struct storage : public header {
    static constexpr auto alignment_of_t = std::alignment_of_v<T>;
    static constexpr auto max_Alignment = std::max(std::alignment_of_v<header>, std::alignment_of_v<T>);
    static constexpr auto offset_to_data = ((sizeof(header) + alignment_of_t - 1) / alignment_of_t) * alignment_of_t;
    static_assert(max_Alignment <= __STDCPP_DEFAULT_NEW_ALIGNMENT__);

    explicit storage(size_t capacity)
        : header(capacity) {}

    auto data() -> T* {
        auto ptr_to_data = reinterpret_cast<std::byte*>(this) + offset_to_data;
        return reinterpret_cast<T*>(ptr_to_data);
    }

    auto data() const -> T const* {
        auto ptr_to_data = reinterpret_cast<std::byte const*>(this) + offset_to_data;
        return reinterpret_cast<T const*>(ptr_to_data);
    }

    static auto alloc(size_t capacity) -> storage<T>* {
        // only void* is allowed to be converted to uintptr_t
        void* ptr = ::operator new(offset_to_data + sizeof(T) * capacity);

        // make sure the lowest significant bit is 0
        auto intPtr = reinterpret_cast<std::uintptr_t>(ptr);
        if ((intPtr & 1U) != 0U) {
            throw std::bad_alloc(); // TODO use a better fitting exception?
        }

        return new (ptr) storage<T>(capacity);
    }
};

} // namespace detail

template <typename T, size_t N>
class svector {
    static_assert(N <= 127, "sorry, can't have more than 127 direct elements (yet)");

    enum class direction { direct, indirect };

    struct Direct {
        // TODO what about big endianness?
        uint8_t m_is_direct : 1;
        uint8_t m_size : 7;
        // see https://stackoverflow.com/a/71828512/48181
        alignas(T) std::array<std::byte, sizeof(T) * N> m_buffer;

        auto data() -> T* {
            return reinterpret_cast<T*>(m_buffer.data());
        }

        auto data() const -> T const* {
            return reinterpret_cast<T const*>(m_buffer.data());
        }
    };
    union {
        Direct m_direct;
        detail::storage<T>* m_indirect;
    } m_union;

    [[nodiscard]] auto is_direct() const -> bool {
        return m_union.m_direct.m_is_direct;
    }

    static void move_elements(T* source_ptr, T* target_ptr, size_t size) {
        for (size_t i = 0; i != size; ++i) {
            new (target_ptr) T(std::move(*source_ptr));
            source_ptr->~T();
            ++target_ptr;
            ++source_ptr;
        }
    }

    void realloc(size_t new_capacity) {
        if (new_capacity <= N) {
            // put everything into direct storage

            if (is_direct()) {
                // direct -> direct: nothing to do!
            } else {
                // indirect -> direct
                auto* storage = m_union.m_indirect;
                move_elements(storage->data(), m_union.m_direct.data(), storage->size());
                m_union.m_direct.m_size = storage->size();
                m_union.m_direct.m_is_direct = 1;
                delete storage;
            }
        } else {
            // put everything into indirect storage
            auto* storage = detail::storage<T>::alloc(new_capacity);
            move_elements(data(), storage->data(), m_union.m_direct.m_size);
            if (is_direct()) {
                // direct -> indirect, nothing special to do here
            } else {
                // indirect -> indirect, delete the old storage
                delete m_union.m_indirect;
            }
            m_union.m_indirect = storage;
        }
    }

    [[nodiscard]] static auto calculate_new_capacity(size_t size_to_fit, size_t starting_capacity) -> size_t {
        auto new_capacity = starting_capacity;

        // double capacity until its large enough, but make sure we don't overflow
        while (new_capacity < size_to_fit && new_capacity * 2 > new_capacity) {
            new_capacity *= 2;
        }
        if (new_capacity < size_to_fit) {
            // got an overflow, set capacity to max
            new_capacity = std::numeric_limits<size_t>::max();
        }
        return new_capacity;
    }

    template <direction D>
    [[nodiscard]] auto capacity() const -> size_t {
        if constexpr (D == direction::direct) {
            return N;
        } else {
            return m_union.m_indirect->capacity();
        }
    }

    template <direction D>
    [[nodiscard]] auto size() const -> size_t {
        if constexpr (D == direction::direct) {
            return m_union.m_direct.m_size;
        } else {
            return m_union.m_indirect->size();
        }
    }

    template <direction D>
    [[nodiscard]] auto data() -> T* {
        if constexpr (D == direction::direct) {
            return reinterpret_cast<T*>(m_union.m_direct.m_buffer.data());
        } else {
            return m_union.m_indirect->data();
        }
    }

    template <direction D>
    void push_back(T const& value) {
        if (size<D>() == capacity<D>()) {
            reserve(size<D>() * 2);
        }
        new (data<D>() + size<D>()) T(value);

        auto new_size = size<D>() + 1;
        if constexpr (D == direction::direct) {
            m_union.m_direct.m_size = new_size;
        } else {
            m_union.m_indirect->size(new_size);
        }
    }

public:
    svector() {
        m_union.m_direct.m_is_direct = 1;
    };

    auto reserve(size_t s) {
        auto new_capacity = calculate_new_capacity(s, capacity());
        realloc(new_capacity);
    }

    [[nodiscard]] auto capacity() const -> size_t {
        if (is_direct()) {
            return capacity<direction::direct>();
        }
        return capacity<direction::indirect>();
    }

    [[nodiscard]] auto size() const -> size_t {
        if (is_direct()) {
            return size<direction::direct>();
        }
        return size<direction::indirect>();
    }

    [[nodiscard]] auto data() -> T* {
        if (is_direct()) {
            return data<direction::direct>();
        }
        return data<direction::indirect>();
    }

    [[nodiscard]] auto data() const -> T const* {
        if (is_direct()) {
            return reinterpret_cast<T const*>(m_union.m_direct.m_buffer.data());
        }
        return m_union.m_indirect->data();
    }

    void push_back(T const& value) {
        if (is_direct()) {
            push_back<direction::direct>(value);
        } else {
            push_back<direction::indirect>(value);
        }
        // TODO
    }

    void push_back(T&& value) {
        if (is_direct()) {
            push_back<direction::direct>(std::move(value));
        } else {
            push_back<direction::indirect>(std::move(value));
        }
    }
};

} // namespace ankerl

#endif