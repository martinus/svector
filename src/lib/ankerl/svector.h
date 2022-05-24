#ifndef ANKERL_SVECTOR_H
#define ANKERL_SVECTOR_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ankerl {

namespace detail {

template <typename Condition, typename T = void>
using enable_if_t = typename std::enable_if<Condition::value, T>::type;

template <typename It>
using is_input_iterator = std::is_base_of<std::input_iterator_tag, typename std::iterator_traits<It>::iterator_category>;

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
            // TODO use a better fitting exception?
            throw std::bad_alloc(); // LCOV_EXCL_LINE
        }

        return new (ptr) storage<T>(capacity);
    }
};

} // namespace detail

template <typename T, size_t N>
class svector {
    static_assert(N <= 127, "sorry, can't have more than 127 direct elements (yet)");

    enum class direction { direct, indirect };

    // TODO what about big endianness?
    class Direct {
        uint8_t m_is_direct : 1;                                  // NOLINT(misc-non-private-member-variables-in-classes)
        uint8_t m_size : 7;                                       // NOLINT(misc-non-private-member-variables-in-classes)
        alignas(T) std::array<std::byte, sizeof(T) * N> m_buffer; // NOLINT(misc-non-private-member-variables-in-classes)

    public:
        auto data() -> T* {
            return reinterpret_cast<T*>(m_buffer.data());
        }

        auto data() const -> T const* {
            return reinterpret_cast<T const*>(m_buffer.data());
        }

        [[nodiscard]] auto size() const -> size_t {
            return m_size;
        }

        void size(size_t s) {
            m_size = s;
        }

        [[nodiscard]] auto is_direct() const -> bool {
            return m_is_direct;
        }

        void is_direct(bool b) {
            m_is_direct = b ? 1 : 0;
        }

        void reset() {
            m_is_direct = 1;
            m_size = 0;
        }
    };

    union {
        Direct m_direct;
        detail::storage<T>* m_indirect;
    } m_union;

    [[nodiscard]] auto is_direct() const -> bool {
        return m_union.m_direct.is_direct();
    }

    static void uninitialized_move_and_destroy(T* source_ptr, T* target_ptr, size_t size) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(target_ptr, source_ptr, size * sizeof(T));
        } else {
            std::uninitialized_move_n(source_ptr, size, target_ptr);
            std::destroy_n(source_ptr, size);
        }
    }

    void realloc(size_t new_capacity) {
        if (new_capacity <= N) {
            // put everything into direct storage

            // direct -> direct: nothing to do!
            if (!is_direct()) {
                // indirect -> direct
                auto* storage = m_union.m_indirect;
                uninitialized_move_and_destroy(storage->data(), m_union.m_direct.data(), storage->size());
                m_union.m_direct.size(storage->size());
                m_union.m_direct.is_direct(true);
                delete storage;
            }
        } else {
            // put everything into indirect storage
            auto* storage = detail::storage<T>::alloc(new_capacity);
            if (is_direct()) {
                // direct -> indirect
                uninitialized_move_and_destroy(data<direction::direct>(), storage->data(), size<direction::direct>());
                storage->size(size<direction::direct>());
            } else {
                // indirect -> indirect
                uninitialized_move_and_destroy(data<direction::indirect>(), storage->data(), size<direction::indirect>());
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
            return m_union.m_direct.size();
        } else {
            return m_union.m_indirect->size();
        }
    }

    void set_size(size_t s) {
        if (is_direct()) {
            set_size<direction::direct>(s);
        } else {
            set_size<direction::indirect>(s);
        }
    }

    template <direction D>
    void set_size(size_t s) {
        if constexpr (D == direction::direct) {
            m_union.m_direct.size(s);
        } else {
            m_union.m_indirect->size(s);
        }
    }

    template <direction D>
    [[nodiscard]] auto data() -> T* {
        if constexpr (D == direction::direct) {
            return std::launder(reinterpret_cast<T*>(m_union.m_direct.data()));
        } else {
            return std::launder(m_union.m_indirect->data());
        }
    }

    template <direction D>
    [[nodiscard]] auto data() const -> T const* {
        if constexpr (D == direction::direct) {
            return std::launder(reinterpret_cast<T const*>(m_union.m_direct.data()));
        } else {
            return std::launder(m_union.m_indirect->data());
        }
    }

    template <direction D>
    void pop_back() {
        if constexpr (std::is_trivially_destructible_v<T>) {
            set_size<D>(size<D>() - 1);
        } else {
            auto s = size<D>() - 1;
            (data<D>() + s)->~T();
            set_size<D>(s);
        }
    }

    /**
     * @brief We need variadic arguments so we can either use copy ctor or default ctor
     */
    template <direction D, class... Args>
    void resize_after_reserve(size_t count, Args&&... args) {
        auto current_size = size<D>();
        if (current_size > count) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                auto* d = data<D>();
                std::destroy(d + count, d + current_size);
            }
        } else {
            auto* d = data<D>();
            for (auto ptr = d + current_size, end = d + count; ptr != end; ++ptr) {
                new (static_cast<void*>(ptr)) T(std::forward<Args>(args)...);
            }
        }
        set_size<D>(count);
    }

    template <typename It>
    void assign(It first, It last, std::input_iterator_tag /*unused*/) {
        clear();

        // TODO this can be made faster, e.g. by setting size only when finished.
        while (first != last) {
            push_back(*first);
            ++first;
        }
    }

    template <typename It>
    void assign(It first, It last, std::forward_iterator_tag /*unused*/) {
        clear();

        auto s = std::distance(first, last);
        reserve(s);
        std::uninitialized_copy(first, last, data());
        set_size(s);
    }

    // precondition: all uninitialized
    void do_move_assign(svector&& other) {
        if (!other.is_direct()) {
            // take other's memory, even when empty
            m_union.m_indirect = other.m_union.m_indirect;
        } else {
            auto* other_ptr = other.data<direction::direct>();
            auto s = other.size<direction::direct>();
            auto* other_end = other_ptr + s;

            std::uninitialized_move(other_ptr, other_end, data<direction::direct>());
            std::destroy(other_ptr, other_end);
            set_size(s);
        }
        other.m_union.m_direct.reset();
    }

    /**
     * @brief Shifts data [source_begin, source_end( to the right, starting on target_begin.
     *
     * Preconditions:
     * * contiguous memory
     * * source_begin <= target_begin
     * * source_end onwards is uninitialized memory
     *
     * Destroys then empty elements in [source_begin, source_end(
     */
    auto shift_right(T* source_begin, T* source_end, T* target_begin) {
        // 1. uninitialized moves
        auto const num_moves = std::distance(source_begin, source_end);
        auto const target_end = target_begin + num_moves;
        auto const num_uninitialized_move = std::min(num_moves, std::distance(source_end, target_end));
        std::uninitialized_move(source_end - num_uninitialized_move, source_end, target_end - num_uninitialized_move);
        std::move_backward(source_begin, source_end - num_uninitialized_move, target_end - num_uninitialized_move);
        std::destroy(source_begin, std::min(source_end, target_begin));
    }

    // makes space for uninitialized data of cout elements. Also updates size.
    auto make_uninitialized_space(T const* pos, size_t count) -> T* {
        auto* const p = const_cast<T*>(pos); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        auto s = size();
        if (s + count > capacity()) {
            auto target = svector();
            // we know target is indirect because we're increasing capacity
            target.reserve(s + count);

            // move everything [begin, pos[
            auto* target_pos = std::uninitialized_move(begin(), p, target.template data<direction::indirect>());

            // move everything [pos, end]
            std::uninitialized_move(p, end(), target_pos + count);

            target.template set_size<direction::indirect>(s + count);
            *this = std::move(target);
            return target_pos;
        }

        shift_right(p, end(), p + count);
        set_size(s + count);
        return p;
    }

    void destroy() {
        auto const is_dir = is_direct();
        if constexpr (!std::is_trivially_destructible_v<T>) {
            T* ptr = nullptr;
            size_t s = 0;
            if (is_dir) {
                ptr = data<direction::direct>();
                s = size<direction::direct>();
            } else {
                ptr = data<direction::indirect>();
                s = size<direction::indirect>();
            }
            std::destroy_n(ptr, s);
        }
        if (!is_dir) {
            delete m_union.m_indirect;
        }
    }

    template <direction D>
    auto at(size_t idx) const -> T const& {
        if (idx >= size<D>()) {
            throw std::out_of_range{"svector: idx out of range"};
        }
        return *(data<D>() + idx);
    }

    template <direction D>
    auto at(size_t idx) -> T& {
        if (idx >= size<D>()) {
            throw std::out_of_range{"svector: idx out of range"};
        }
        return *(data<D>() + idx);
    }

public:
    using value_type = T;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = value_type const&;
    using pointer = T*;
    using const_pointer = T const*;
    using iterator = T*;
    using const_iterator = T const*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    svector() {
        m_union.m_direct.reset();
    }

    svector(size_t count, T const& value)
        : svector() {
        resize(count, value);
    }

    explicit svector(size_t count)
        : svector() {
        reserve(count);
        if (is_direct()) {
            resize_after_reserve<direction::direct>(count);
        } else {
            resize_after_reserve<direction::indirect>(count);
        }
    }

    template <typename InputIt, typename = detail::enable_if_t<detail::is_input_iterator<InputIt>>>
    svector(InputIt first, InputIt last)
        : svector() {
        assign(first, last);
    }

    svector(svector const& other)
        : svector() {
        auto s = other.size();
        reserve(s);
        std::uninitialized_copy(other.begin(), other.end(), begin());
        set_size(s);
    }

    svector(svector&& other) noexcept
        : svector() {
        do_move_assign(std::move(other));
    }

    svector(std::initializer_list<T> init)
        : svector(init.begin(), init.end()) {}

    ~svector() {
        destroy();
    }

    void assign(size_t count, T const& value) {
        clear();
        resize(count, value);
    }

    template <typename InputIt, typename = detail::enable_if_t<detail::is_input_iterator<InputIt>>>
    void assign(InputIt first, InputIt last) {
        assign(first, last, typename std::iterator_traits<InputIt>::iterator_category());
    }

    void assign(std::initializer_list<T> l) {
        assign(l.begin(), l.end());
    }

    auto operator=(svector const& other) -> svector& {
        if (&other == this) {
            return *this;
        }

        assign(other.begin(), other.end());
        return *this;
    }

    auto operator=(svector&& other) noexcept -> svector& {
        if (&other == this) {
            // It doesn't seem to be required to do self-check, but let's do it anyways to be safe
            return *this;
        }
        destroy();
        do_move_assign(std::move(other));
        return *this;
    }

    auto operator=(std::initializer_list<T> l) -> svector& {
        assign(l.begin(), l.end());
        return *this;
    }

    void resize(size_t count) {
        resize(count, T());
    }

    void resize(size_t count, T const& value) {
        if (count > capacity()) {
            reserve(count);
        }
        if (is_direct()) {
            resize_after_reserve<direction::direct>(count, value);
        } else {
            resize_after_reserve<direction::indirect>(count, value);
        }
    }

    auto reserve(size_t s) {
        auto old_capacity = capacity();
        auto new_capacity = calculate_new_capacity(s, old_capacity);
        if (new_capacity > old_capacity) {
            realloc(new_capacity);
        }
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
            return reinterpret_cast<T const*>(m_union.m_direct.data());
        }
        return m_union.m_indirect->data();
    }

    template <class... Args>
    auto emplace_back(Args&&... args) -> T& {
        size_t c; // NOLINT(cppcoreguidelines-init-variables)
        size_t s; // NOLINT(cppcoreguidelines-init-variables)
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
            set_size<direction::direct>(s + 1);
            return *new (static_cast<void*>(data<direction::direct>() + s)) T(std::forward<Args>(args)...);
        }

        set_size<direction::indirect>(s + 1);
        return *new (static_cast<void*>(data<direction::indirect>() + s)) T(std::forward<Args>(args)...);
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

    auto at(size_t idx) const -> T const& {
        if (is_direct()) {
            return at<direction::direct>(idx);
        }
        return at<direction::indirect>(idx);
    }

    auto at(size_t idx) -> T& {
        if (is_direct()) {
            return at<direction::direct>(idx);
        }
        return at<direction::indirect>(idx);
    }

    [[nodiscard]] auto begin() const -> T const* {
        return data();
    }

    [[nodiscard]] auto cbegin() const -> T const* {
        return begin();
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

    [[nodiscard]] auto cend() const -> T const* {
        return end();
    }

    [[nodiscard]] auto end() -> T* {
        if (is_direct()) {
            return data<direction::direct>() + size<direction::direct>();
        }
        return data<direction::indirect>() + size<direction::indirect>();
    }

    [[nodiscard]] auto rbegin() -> reverse_iterator {
        return reverse_iterator{end()};
    }

    [[nodiscard]] auto rbegin() const -> const_reverse_iterator {
        return const_reverse_iterator{end()};
    }

    [[nodiscard]] auto crbegin() const -> const_reverse_iterator {
        return const_reverse_iterator{end()};
    }

    [[nodiscard]] auto rend() -> reverse_iterator {
        return reverse_iterator{begin()};
    }

    [[nodiscard]] auto rend() const -> const_reverse_iterator {
        return const_reverse_iterator{begin()};
    }

    [[nodiscard]] auto crend() const -> const_reverse_iterator {
        return const_reverse_iterator{begin()};
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
            std::destroy(begin(), end());
        }

        if (is_direct()) {
            set_size<direction::direct>(0);
        } else {
            set_size<direction::indirect>(0);
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

    void swap(svector& other) {
        // TODO we could try to do the minimum number of moves
        std::swap(*this, other);
    }

    void shrink_to_fit() {
        // per the standard we wouldn't need to do anything here. But since we are so nice,
        // let's do the shrink.
        auto const c = capacity();
        auto const s = size();
        if (s >= c) {
            return;
        }

        auto new_capacity = calculate_new_capacity(s, N);
        if (new_capacity == c) {
            // nothing change!
            return;
        }

        realloc(new_capacity);
    }

    template <class... Args>
    auto emplace(const_iterator pos, Args&&... args) -> iterator {
        auto* p = make_uninitialized_space(pos, 1);
        return new (static_cast<void*>(p)) T(std::forward<Args>(args)...);
    }

    auto insert(const_iterator pos, T const& value) -> iterator {
        return emplace(pos, value);
    }

    auto insert(const_iterator pos, T&& value) -> iterator {
        return emplace(pos, std::move(value));
    }

    auto insert(const_iterator pos, size_t count, T const& value) -> iterator {
        auto* p = make_uninitialized_space(pos, count);
        std::uninitialized_fill_n(p, count, value);
        return p;
    }

    template <typename It>
    auto insert(const_iterator pos, It first, It last, std::input_iterator_tag /*unused*/) {
        if (!(first != last)) {
            return const_cast<T*>(pos); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        }

        // just input_iterator_tag makes this very slow. Let's do the same as the STL.
        if (pos == end()) {
            auto s = size();
            while (first != last) {
                emplace_back(*first);
                ++first;
            }
            return begin() + s;
        }

        auto tmp = svector(first, last);
        return insert(pos, std::make_move_iterator(tmp.begin()), std::make_move_iterator(tmp.end()));
    }

    template <typename It>
    auto insert(const_iterator pos, It first, It last, std::forward_iterator_tag /*unused*/) {
        auto* p = make_uninitialized_space(pos, std::distance(first, last));
        std::uninitialized_copy(first, last, p);
        return p;
    }

    template <typename InputIt, typename = detail::enable_if_t<detail::is_input_iterator<InputIt>>>
    auto insert(const_iterator pos, InputIt first, InputIt last) -> iterator {
        return insert(pos, first, last, typename std::iterator_traits<InputIt>::iterator_category());
    }

    auto insert(const_iterator pos, std::initializer_list<T> l) -> iterator {
        return insert(pos, l.begin(), l.end());
    }
};

template <typename T, size_t NA, size_t NB>
[[nodiscard]] auto operator==(svector<T, NA> const& a, svector<T, NB> const& b) -> bool {
    return std::equal(a.begin(), a.end(), b.begin(), b.end());
}

template <typename T, size_t NA, size_t NB>
[[nodiscard]] auto operator!=(svector<T, NA> const& a, svector<T, NB> const& b) -> bool {
    return !(a == b);
}

template <typename T, size_t NA, size_t NB>
[[nodiscard]] auto operator<(svector<T, NA> const& a, svector<T, NB> const& b) -> bool {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}

template <typename T, size_t NA, size_t NB>
[[nodiscard]] auto operator>=(svector<T, NA> const& a, svector<T, NB> const& b) -> bool {
    return !(a < b);
}

template <typename T, size_t NA, size_t NB>
[[nodiscard]] auto operator>(svector<T, NA> const& a, svector<T, NB> const& b) -> bool {
    return std::lexicographical_compare(b.begin(), b.end(), a.begin(), a.end());
}

template <typename T, size_t NA, size_t NB>
[[nodiscard]] auto operator<=(svector<T, NA> const& a, svector<T, NB> const& b) -> bool {
    return !(a > b);
}

} // namespace ankerl

#endif
