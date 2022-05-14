#ifndef ANKERL_SVECTOR_H
#define ANKERL_SVECTOR_H

#include <algorithm> // max
#include <array>
#include <cstddef> // size_t, byte
#include <cstdint> // uintptr_t
#include <cstring> // memcpy
#include <limits>
#include <new>
#include <type_traits>

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
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(target_ptr, source_ptr, size * sizeof(T));
        } else {
            for (size_t i = 0; i != size; ++i) {
                new (target_ptr) T(std::move(*source_ptr));
                source_ptr->~T();
                ++target_ptr;
                ++source_ptr;
            }
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
            if (is_direct()) {
                // direct -> indirect
                move_elements(data<direction::direct>(), storage->data(), size<direction::direct>());
                storage->size(size<direction::direct>());
            } else {
                // indirect -> indirect
                move_elements(data<direction::indirect>(), storage->data(), size<direction::indirect>());
                storage->size(size<direction::indirect>());
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
    void size(size_t s) {
        if constexpr (D == direction::direct) {
            m_union.m_direct.m_size = s;
        } else {
            return m_union.m_indirect->size(s);
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
    void pop_back() {
        if constexpr (std::is_trivially_destructible_v<T>) {
            size<D>(size<D>() - 1);
        } else {
            auto s = size<D>() - 1;
            (data<D>() + s)->~T();
            size<D>(s);
        }
    }

public:
    svector() {
        m_union.m_direct.m_is_direct = 1;
        m_union.m_direct.m_size = 0;
    }

    ~svector() {
        auto const is_dir = is_direct();
        if constexpr (!std::is_trivially_destructible_v<T>) {
            T* ptr;
            size_t s;
            if (is_dir) {
                ptr = data<direction::direct>();
                s = size<direction::direct>();
            } else {
                ptr = data<direction::indirect>();
                s = size<direction::indirect>();
            }
            for (size_t i = 0; i < s; ++i) {
                (ptr + i)->~T();
            }
        }
        if (!is_dir) {
            delete m_union.m_indirect;
        }
    }

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

    template <class... Args>
    auto emplace_back(Args&&... args) -> T& {
        size_t c;
        size_t s;
        bool is_dir = is_direct();
        if (is_dir) {
            c = capacity<direction::direct>();
            s = size<direction::direct>();
        } else {
            c = capacity<direction::indirect>();
            s = size<direction::indirect>();
        }

        if (s == c) {
            auto new_capacity = calculate_new_capacity(s + 1, c);
            realloc(new_capacity);
            // reallocation happened, so we definitely are now in indirect mode
            is_dir = false;
        }

        if (is_dir) {
            size<direction::direct>(s + 1);
            return *new (data<direction::direct>() + s) T(std::forward<Args>(args)...);
        }

        size<direction::indirect>(s + 1);
        return *new (data<direction::indirect>() + s) T(std::forward<Args>(args)...);
    }

    void push_back(T const& value) {
        emplace_back(value);
    }

    void push_back(T&& value) {
        emplace_back(std::move(value));
    }

    [[nodiscard]] auto operator[](size_t idx) const -> T const& {
        return *(data() + idx);
    }

    [[nodiscard]] auto operator[](size_t idx) -> T& {
        return *(data() + idx);
    }

    [[nodiscard]] auto begin() const -> T const* {
        return data();
    }

    [[nodiscard]] auto begin() -> T* {
        return data();
    }

    [[nodiscard]] auto end() const -> T const* {
        if (is_direct()) {
            return data<direction::direct>() + size<direction::direct>();
        }
        return data<direction::indirect>() + size<direction::indirect>();
    }

    [[nodiscard]] auto end() -> T* {
        if (is_direct()) {
            return data<direction::direct>() + size<direction::direct>();
        }
        return data<direction::indirect>() + size<direction::indirect>();
    }

    [[nodiscard]] auto front() const -> T const& {
        return *data();
    }

    [[nodiscard]] auto front() -> T& {
        return *data();
    }

    [[nodiscard]] auto back() const -> T const& {
        if (is_direct()) {
            return *(data<direction::direct>() + size<direction::direct>() - 1);
        }
        return *(data<direction::indirect>() + size<direction::indirect>() - 1);
    }

    [[nodiscard]] auto back() -> T& {
        if (is_direct()) {
            return *(data<direction::direct>() + size<direction::direct>() - 1);
        }
        return *(data<direction::indirect>() + size<direction::indirect>() - 1);
    }

    void clear() {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (auto& x : *this) {
                x.~T();
            }
        }

        if (is_direct()) {
            size<direction::direct>(0);
        } else {
            size<direction::indirect>(0);
        }
    }

    [[nodiscard]] auto empty() const -> bool {
        return 0U == size();
    }

    void pop_back() {
        if (is_direct()) {
            pop_back<direction::direct>();
        } else {
            pop_back<direction::indirect>();
        }
    }

    [[nodiscard]] auto max_size() const -> size_t {
        return std::numeric_limits<size_t>::max();
    }
};

} // namespace ankerl

#endif
