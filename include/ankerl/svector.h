// ┌─┐┬  ┬┌─┐┌─┐┌┬┐┌─┐┬─┐   Compact SVO optimized vector C++17 or higher
// └─┐└┐┌┘├┤ │   │ │ │├┬┘   Version 1.0.3
// └─┘ └┘ └─┘└─┘ ┴ └─┘┴└─   https://github.com/martinus/svector
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2023 Martin Leitner-Ankerl <martin.ankerl@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ANKERL_SVECTOR_H
#define ANKERL_SVECTOR_H

// see https://semver.org/spec/v2.0.0.html
#define ANKERL_SVECTOR_VERSION_MAJOR 1 // incompatible API changes
#define ANKERL_SVECTOR_VERSION_MINOR 0 // add functionality in a backwards compatible manner
#define ANKERL_SVECTOR_VERSION_PATCH 3 // backwards compatible bug fixes

// API versioning with inline namespace, see https://www.foonathan.net/2018/11/inline-namespaces/
#define ANKERL_SVECTOR_VERSION_CONCAT1(major, minor, patch) v##major##_##minor##_##patch
#define ANKERL_SVECTOR_VERSION_CONCAT(major, minor, patch) ANKERL_SVECTOR_VERSION_CONCAT1(major, minor, patch)
#define ANKERL_SVECTOR_NAMESPACE \
    ANKERL_SVECTOR_VERSION_CONCAT(ANKERL_SVECTOR_VERSION_MAJOR, ANKERL_SVECTOR_VERSION_MINOR, ANKERL_SVECTOR_VERSION_PATCH)

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
inline namespace ANKERL_SVECTOR_NAMESPACE {
namespace detail {

template <typename Condition, typename T = void>
using enable_if_t = typename std::enable_if<Condition::value, T>::type;

template <typename It>
using is_input_iterator = std::is_base_of<std::input_iterator_tag, typename std::iterator_traits<It>::iterator_category>;

constexpr auto round_up(size_t n, size_t multiple) -> size_t {
    return ((n + (multiple - 1)) / multiple) * multiple;
}

template <typename T>
constexpr auto cx_min(T a, T b) -> T {
    return a < b ? a : b;
}

template <typename T>
constexpr auto cx_max(T a, T b) -> T {
    return a > b ? a : b;
}

template <typename T>
constexpr auto alignment_of_svector() -> size_t {
    return cx_max(sizeof(void*), std::alignment_of_v<T>);
}

/**
 * @brief Calculates sizeof(svector<T, N>) for a given type and inline capacity
 */
template <typename T>
constexpr auto size_of_svector(size_t min_inline_capacity) -> size_t {
    // + 1 for one byte size in direct mode
    return round_up(sizeof(T) * min_inline_capacity + 1, alignment_of_svector<T>());
}

/**
 * @brief Calculates how many T we can actually store inside of an svector without increasing its sizeof().
 *
 * E.g. svector<char, 1> could store 7 bytes even though 1 is specified. This makes sure we don't waste any
 * of the padding.
 */
template <typename T>
constexpr auto automatic_capacity(size_t min_inline_capacity) -> size_t {
    return cx_min((size_of_svector<T>(min_inline_capacity) - 1U) / sizeof(T), size_t{127});
}

/**
 * Holds size & capacity, a glorified struct.
 */
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

/**
 * @brief Holds header (size+capacity) plus an arbitrary number of T.
 *
 * To make storage compact, we don't actually store a pointer to T. We don't have to
 * because we know exactly at which location it begins.
 */
template <typename T>
struct storage : public header {
    static constexpr auto alignment_of_t = std::alignment_of_v<T>;
    static constexpr auto max_alignment = (std::max)(std::alignment_of_v<header>, std::alignment_of_v<T>);
    static constexpr auto offset_to_data = detail::round_up(sizeof(header), alignment_of_t);
    static_assert(max_alignment <= __STDCPP_DEFAULT_NEW_ALIGNMENT__);

    explicit storage(size_t capacity)
        : header(capacity) {}

    auto data() -> T* {
        auto ptr_to_data = reinterpret_cast<std::byte*>(this) + offset_to_data;
        return std::launder(reinterpret_cast<T*>(ptr_to_data));
    }

    /**
     * @brief Allocates space for storage plus capacity*T objects.
     *
     * Checks to make sure that allocation won't overflow.
     *
     * @param capacity Number of T to allocate.
     * @return storage<T>*
     */
    static auto alloc(size_t capacity) -> storage<T>* {
        // make sure we don't overflow!
        auto mem = sizeof(T) * capacity;
        if (mem < capacity) {
            throw std::bad_alloc();
        }
        if (offset_to_data + mem < mem) {
            throw std::bad_alloc();
        }
        mem += offset_to_data;
        if (static_cast<uint64_t>(mem) > static_cast<uint64_t>((std::numeric_limits<std::ptrdiff_t>::max)())) {
            throw std::bad_alloc();
        }

        void* ptr = ::operator new(offset_to_data + sizeof(T) * capacity);
        if (nullptr == ptr) {
            throw std::bad_alloc();
        }
        // use void* to ensure we don't use an overload for T*
        return new (ptr) storage<T>(capacity);
    }

    /**
     * @brief Counterpart to alloc(). Does not touch the T's, they have to be destroyed already.
     */
    static void dealloc(storage<T>* ptr) {
        std::destroy_at(ptr);
        ::operator delete(ptr);
    }
};

/**
 * @brief Destroys [first, last) on the way out, unless it has been released.
 *
 * A half built container can normally describe what it has by its size, and then its own destructor
 * is all the cleanup that is needed. This is for the one place where the constructed part is not a
 * prefix, so no size can express it.
 */
template <typename T>
class destroy_guard {
    T* m_first;
    T* m_last;

public:
    destroy_guard(T* first, T* last)
        : m_first(first)
        , m_last(last) {}

    destroy_guard(destroy_guard const&) = delete;
    destroy_guard(destroy_guard&&) = delete;
    auto operator=(destroy_guard const&) -> destroy_guard& = delete;
    auto operator=(destroy_guard&&) -> destroy_guard& = delete;

    ~destroy_guard() {
        std::destroy(m_first, m_last);
    }

    // The guarded range only ever grows to the front, and stays contiguous while it does.
    void extend_front(T* first) {
        m_first = first;
    }

    void release() {
        m_last = m_first;
    }
};

} // namespace detail

template <typename T, size_t MinInlineCapacity>
class svector {
    static_assert(MinInlineCapacity <= 127, "sorry, can't have more than 127 direct elements");
    static constexpr auto N = detail::automatic_capacity<T>(MinInlineCapacity);

    // Only used by svector.natvis: the Visual Studio debugger can't evaluate functions, so it needs
    // these offsets as data. Keep in sync with direct_data() and detail::storage<T>::offset_to_data.
    static constexpr auto alignment_of_t = std::alignment_of_v<T>;
    static constexpr auto offset_to_indirect_data = detail::round_up(sizeof(detail::header), alignment_of_t);

    enum class direction { direct, indirect };

    /**
     * A buffer to hold the data of the svector Depending on direct/indirect mode, the content it holds is like so:
     *
     * direct:
     *    m_data[0] & 1: lowest bit is 1 for direct mode.
     *    m_data[0] >> 1: size for direct mode
     *    Then 0-X bytes unused (padding), and then the actual inline T data.
     * indirect:
     *    m_data[0] & 1: lowest bit is 0 for indirect mode
     *    m_data[0..7]: stores an uintptr_t, which points to the indirect data.
     */
    alignas(detail::alignment_of_svector<T>()) std::array<uint8_t, detail::size_of_svector<T>(MinInlineCapacity)> m_data;

    // direct mode ///////////////////////////////////////////////////////////

    [[nodiscard]] auto is_direct() const -> bool {
        return (m_data[0] & 1U) != 0U;
    }

    [[nodiscard]] auto direct_size() const -> size_t {
        return m_data[0] >> 1U;
    }

    // sets size of direct mode and mode to direct too.
    constexpr void set_direct_and_size(size_t s) {
        m_data[0] = static_cast<uint8_t>((s << 1U) | 1U);
    }

    [[nodiscard]] auto direct_data() -> T* {
        return std::launder(reinterpret_cast<T*>(m_data.data() + std::alignment_of_v<T>));
    }

    // indirect mode /////////////////////////////////////////////////////////

    [[nodiscard]] auto indirect() -> detail::storage<T>* {
        detail::storage<T>* ptr; // NOLINT(cppcoreguidelines-init-variables)
        std::memcpy(&ptr, m_data.data(), sizeof(ptr));
        return ptr;
    }

    [[nodiscard]] auto indirect() const -> detail::storage<T> const* {
        return const_cast<svector*>(this)->indirect(); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }

    void set_indirect(detail::storage<T>* ptr) {
        std::memcpy(m_data.data(), &ptr, sizeof(ptr));

        // safety check to guarantee the lowest bit is 0
        if (is_direct()) {
            throw std::bad_alloc(); // LCOV_EXCL_LINE
        }
    }

    // helpers ///////////////////////////////////////////////////////////////

    /**
     * @brief Moves size objects from source_ptr to target_ptr, and destroys what remains in source_ptr.
     *
     * Assumes data is not overlapping
     */
    static void uninitialized_move_and_destroy(T* source_ptr, T* target_ptr, size_t size) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(target_ptr, source_ptr, size * sizeof(T));
        } else {
            std::uninitialized_move_n(source_ptr, size, target_ptr);
            std::destroy_n(source_ptr, size);
        }
    }

    /**
     * @brief True when value is one of our own elements, e.g. from v.push_back(v[0]).
     *
     * Growing or shifting moves those elements around and destroys the originals, so anything
     * that takes a T const& has to copy it out of the way first. Compared as uintptr_t because
     * relational operators on pointers into different objects are not specified.
     */
    [[nodiscard]] auto is_reference_into_self(T const& value) const -> bool {
        auto const p = reinterpret_cast<uintptr_t>(std::addressof(value));
        auto const first = reinterpret_cast<uintptr_t>(data());
        return p >= first && p < first + (sizeof(T) * size());
    }

    /**
     * @brief Moves all current elements into storage, frees the old one and adopts storage.
     *
     * Takes ownership of storage even when it fails: T's move constructor is allowed to throw,
     * and then nothing here has happened yet, so storage has to be freed again on the way out or
     * it leaks. num_constructed says how many elements the caller has already built at the end of
     * storage, which then have to be destroyed too. std::uninitialized_move_n already cleans up
     * whatever it managed to move itself.
     *
     * Not a template, so every emplace_back instantiation shares this instead of inlining its
     * own copy of the direct/indirect fork.
     */
    void take_over_storage(detail::storage<T>* storage, size_t new_size, size_t num_constructed = 0) {
        try {
            if (is_direct()) {
                uninitialized_move_and_destroy(data<direction::direct>(), storage->data(), size<direction::direct>());
            } else {
                uninitialized_move_and_destroy(data<direction::indirect>(), storage->data(), size<direction::indirect>());
                detail::storage<T>::dealloc(indirect());
            }
        } catch (...) {
            std::destroy_n(storage->data() + new_size - num_constructed, num_constructed);
            detail::storage<T>::dealloc(storage);
            throw;
        }
        storage->size(new_size);
        set_indirect(storage);
    }

    /**
     * @brief Grows and appends in a single step. Precondition: size() == capacity().
     *
     * args may reference one of our own elements, as in v.push_back(v[0]). Reallocating first
     * would move that element into the new storage and destroy the original, leaving args
     * dangling, so the new element is constructed into the fresh storage while the old elements
     * are still untouched. Only afterwards are they moved over.
     *
     * emplace(cend(), ...) relies on this ordering too, don't turn it back into
     * reallocate-then-construct.
     */
    template <class... Args>
    auto emplace_back_grow(size_t s, Args&&... args) -> T& {
        // s + 1 > capacity() >= N, so the new storage is always indirect
        auto* storage = detail::storage<T>::alloc(calculate_new_capacity(s + 1, s));

        T* element = nullptr;
        try {
            element = new (static_cast<void*>(storage->data() + s)) T(std::forward<Args>(args)...);
        } catch (...) {
            detail::storage<T>::dealloc(storage);
            throw;
        }

        // args has been consumed, the old elements can be moved now. If that throws, element is
        // the one thing already built in storage, so tell take_over_storage to clean it up.
        take_over_storage(storage, s + 1, 1);
        return *element;
    }

    /**
     * @brief Reallocates all data when capacity changes.
     *
     * if new_capacity <= N chooses direct memory, otherwise indirect.
     *
     * Invalidates every reference into the container, so any T const& argument that might be
     * one of our own elements has to be copied out of the way before calling this. See
     * is_reference_into_self().
     */
    void realloc(size_t new_capacity) {
        if (new_capacity <= N) {
            // put everything into direct storage
            if (is_direct()) {
                // direct -> direct: nothing to do!
                return;
            }

            // indirect -> direct
            auto* storage = indirect();
            uninitialized_move_and_destroy(storage->data(), direct_data(), storage->size());
            set_direct_and_size(storage->size());
            detail::storage<T>::dealloc(storage);
        } else {
            // put everything into indirect storage
            take_over_storage(detail::storage<T>::alloc(new_capacity), size());
        }
    }

    /**
     * @brief Doubles starting_capacity until it is >= size_to_fit.
     */
    [[nodiscard]] static auto calculate_new_capacity(size_t size_to_fit, size_t starting_capacity) -> size_t {
        if (size_to_fit > max_size()) {
            // not enough space
            throw std::bad_alloc();
        }

        if (size_to_fit == 0) {
            // special handling for 0 so N==0 works
            return starting_capacity;
        }
        // start with at least 1, so N==0 works
        auto new_capacity = std::max<size_t>(1, starting_capacity);

        // double capacity until its large enough, but make sure we don't overflow
        while (new_capacity < size_to_fit && new_capacity * 2 > new_capacity) {
            new_capacity *= 2;
        }
        if (new_capacity < size_to_fit) {
            // got an overflow, set capacity to max
            new_capacity = max_size();
        }
        return (std::min)(new_capacity, max_size());
    }

    template <direction D>
    [[nodiscard]] auto capacity() const -> size_t {
        if constexpr (D == direction::direct) {
            return N;
        } else {
            return indirect()->capacity();
        }
    }

    template <direction D>
    [[nodiscard]] auto size() const -> size_t {
        if constexpr (D == direction::direct) {
            return direct_size();
        } else {
            return indirect()->size();
        }
    }

    template <direction D>
    void set_size(size_t s) {
        if constexpr (D == direction::direct) {
            set_direct_and_size(s);
        } else {
            indirect()->size(s);
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
    [[nodiscard]] auto data() -> T* {
        if constexpr (D == direction::direct) {
            return direct_data();
        } else {
            return indirect()->data();
        }
    }

    template <direction D>
    [[nodiscard]] auto data() const -> T const* {
        return const_cast<svector*>(this)->data<D>(); // NOLINT(cppcoreguidelines-pro-type-const-cast)
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
            // The hand written loop this replaces left everything it had already built behind when
            // a constructor threw: the size still said current_size, so nothing ever destroyed
            // them. These two clean up after themselves, which is the same reason insert() builds
            // through them rather than looping. See issue #70.
            //
            // Value construction, not default construction: resize(n) on an svector<int> has to
            // zero the new elements the way T() does.
            auto* const first_new = data<D>() + current_size;
            if constexpr (sizeof...(Args) == 0) {
                std::uninitialized_value_construct_n(first_new, count - current_size);
            } else {
                std::uninitialized_fill_n(first_new, count - current_size, args...);
            }
        }
        set_size<D>(count);
    }

    // Makes sure that to is not past the end iterator, and does nothing when that leaves an empty
    // range to erase
    template <direction D>
    auto erase_checked_end(T const* cfrom, T const* to) -> T* {
        auto* const erase_begin = const_cast<T*>(cfrom); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        auto* const container_end = data<D>() + size<D>();
        auto* const erase_end = (std::min)(const_cast<T*>(to), container_end); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        auto const num_erased = std::distance(erase_begin, erase_end);

        if (num_erased == 0) {
            // Not just a shortcut: std::move below would be handed a destination equal to its
            // source begin, which it does not allow, and it would self-move-assign every element
            // from here to the end. See issue #66.
            return erase_begin;
        }

        std::move(erase_end, container_end, erase_begin);
        std::destroy(container_end - num_erased, container_end);
        set_size<D>(size<D>() - num_erased);
        return erase_begin;
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
        /**
         * Everything that makes up an svector lives inside m_data: in indirect mode just the
         * pointer, in direct mode the size byte and the elements themselves. So copying the array
         * copies the whole vector, with no branch on the mode and no loop over the elements. See
         * issue #54.
         *
         * Two things have to hold. The elements must survive being relocated bytewise, which is
         * what trivially copyable buys: byte-relocating e.g. a libstdc++ std::string in its small
         * string mode leaves its data pointer aimed at other's inline buffer, which dangles as
         * soon as other is gone.
         *
         * And the object has to be small, because the copy always covers the full inline capacity
         * and not just the used part. Sorting a std::vector of svector<int> holding two elements
         * each: gcc 16 -O3 gains 31% at 24 bytes, 21% at 72 bytes, but loses 72% at 264; clang 22
         * gains 38% / 29% with the same crossover. Filling a std::vector by moving svectors into
         * it is allocation bound and gains 15% on clang while costing up to 3% on gcc.
         */
        constexpr auto relocate_by_copying_m_data = std::is_trivially_copyable_v<T> && sizeof(m_data) <= 128U;

        if constexpr (relocate_by_copying_m_data) {
            m_data = other.m_data;
        } else if (!other.is_direct()) {
            // take other's memory, even when empty
            set_indirect(other.indirect());
        } else {
            auto* other_ptr = other.data<direction::direct>();
            auto s = other.size<direction::direct>();
            auto* other_end = other_ptr + s;

            std::uninitialized_move(other_ptr, other_end, data<direction::direct>());
            std::destroy(other_ptr, other_end);
            set_size(s);
        }
        other.set_direct_and_size(0);
    }

    /**
     * @brief How insert_n() puts new elements somewhere: source is a range of count of them.
     *
     * An insert in the middle needs both halves of this. Everything landing past the old end goes
     * into raw storage and has to be constructed, everything before it lands on an element that is
     * still there and can be assigned over, which is the whole point: assigning is what keeps a
     * gap of raw memory from ever existing inside size(). Types like std::string would rather be
     * assigned to anyway, they can answer it out of the buffer they already hold.
     */
    template <typename It>
    struct place_range {
        It source;

        void construct(T* dst, size_t offset, size_t n) const {
            std::uninitialized_copy_n(std::next(source, static_cast<ptrdiff_t>(offset)), n, dst);
        }

        void assign(T* dst, size_t offset, size_t n) const {
            std::copy_n(std::next(source, static_cast<ptrdiff_t>(offset)), n, dst);
        }
    };

    /**
     * @brief Same, for count copies of one value.
     */
    struct place_copies {
        T const& value;

        void construct(T* dst, size_t /*offset*/, size_t n) const {
            std::uninitialized_fill_n(dst, n, value);
        }

        void assign(T* dst, size_t /*offset*/, size_t n) const {
            std::fill_n(dst, n, value);
        }
    };

    /**
     * @brief Places a single element by moving it out of source, which the caller owns.
     */
    static auto place_moved(T& source) -> place_range<std::move_iterator<T*>> {
        return {std::make_move_iterator(std::addressof(source))};
    }

    /**
     * @brief Inserts count elements at pos, taken from place. Returns the first one.
     *
     * Invalidates every reference into the container: the elements from pos on are either shifted
     * right or moved into a fresh allocation. A T const& argument that might be one of our own
     * elements has to be dealt with first, see is_reference_into_self().
     *
     * There is deliberately no point in here where size() counts memory that holds no element.
     * Opening a hole first and constructing into it afterwards was simpler, but it made the
     * container undestroyable for as long as the hole lasted, so every constructor that could throw
     * needed a rollback to close it again -- and getting that rollback right is what issues #68 and
     * #74 were about. Threading the new elements through the shift instead, the way std::vector
     * does, removes the hole and the rollback and the whole family of problems with them.
     *
     * What that costs is the strong exception guarantee for a partly done insert: an element
     * assignment throwing part way leaves the container with the right number of elements but no
     * promise about which. That is what the standard allows for insert, and what std::vector does.
     * Where nothing has to be shifted over live elements the insert still either happens or does
     * not: growing builds the result in a separate allocation, and single element inserts go
     * through emplace(), which builds the element before touching anything.
     */
    template <direction D, typename Place>
    auto insert_n(T const* pos, size_t count, Place const& place) -> T* {
        auto* const p = const_cast<T*>(pos); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        auto const s = size<D>();

        if (count == 0) {
            // Not just a shortcut: the shift below would be handed a destination equal to its
            // source end, which std::move_backward does not allow, and it would self-move-assign
            // every element it covers. See issue #65.
            return p;
        }

        // Both written as subtractions so neither can wrap: s <= max_size() and s <= capacity()
        // always hold. It used to say s + count > capacity(), which overflowed for a huge count,
        // and the wrapped sum then looked small enough to fit, so the in place shift below ran
        // straight past the end of the buffer. See issue #69.
        if (count > max_size() - s) {
            throw std::bad_alloc();
        }

        if (count > capacity<D>() - s) {
            return insert_n_new<D>(p, s, count, place);
        }

        auto* const old_end = data<D>() + s;
        auto const tail = static_cast<size_t>(old_end - p);

        if (tail > count) {
            // The tail is long enough that shifting it right stays within the old elements plus
            // the count raw slots behind them, so all the new elements land on live ones.
            std::uninitialized_move(old_end - count, old_end, old_end);
            set_size<D>(s + count);
            std::move_backward(p, old_end - count, old_end);
            place.assign(p, 0, count);
        } else {
            // The tail clears the ground it stood on, so the new elements behind the old end have
            // nothing under them and are built instead.
            place.construct(old_end, tail, count - tail);
            set_size<D>(s + count - tail);
            std::uninitialized_move(p, old_end, p + count);
            set_size<D>(s + count);
            place.assign(p, 0, tail);
        }
        return p;
    }

    /**
     * @brief insert_n() for when the elements no longer fit. Builds the result in fresh storage.
     */
    template <direction D, typename Place>
    auto insert_n_new(T* p, size_t s, size_t count, Place const& place) -> T* {
        auto target = svector();
        target.reserve(s + count); // we know target is indirect because we're increasing capacity

        auto* const dst = target.template data<direction::indirect>();
        auto* const gap = dst + (p - data<D>());

        // The new elements before ours: nothing of ours has moved yet if this throws, so the insert
        // simply has not happened, and place can still read the elements we hold.
        place.construct(gap, 0, count);

        // What is built now is the gap but nothing in front of it, and no size can say that, so for
        // as long as the hole lasts the cleanup is spelled out here. Leaving it to target's
        // destructor, which sees a size of zero, is what leaked the relocated elements in issue
        // #74. Both moves below destroy whatever they managed to build themselves.
        auto guard = detail::destroy_guard<T>(gap, gap + count);
        std::uninitialized_move(data<D>(), p, dst);
        guard.extend_front(dst);
        std::uninitialized_move(p, data<D>() + s, gap + count);
        guard.release();

        target.template set_size<direction::indirect>(s + count);
        *this = std::move(target);
        return gap;
    }

    template <typename Place>
    auto insert_n(T const* pos, size_t count, Place const& place) -> T* {
        if (is_direct()) {
            return insert_n<direction::direct>(pos, count, place);
        }
        return insert_n<direction::indirect>(pos, count, place);
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
            detail::storage<T>::dealloc(indirect());
        }
        set_direct_and_size(0);
    }

    // performs a const_cast so we don't need this implementation twice
    template <direction D>
    auto at(size_t idx) -> T& {
        if (idx >= size<D>()) {
            throw std::out_of_range{"svector: idx out of range"};
        }
        auto* ptr = const_cast<T*>(data<D>() + idx); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        return *ptr;
    } // LCOV_EXCL_LINE why is this single } marked as not covered? gcov bug?

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

    svector() noexcept {
        set_direct_and_size(0);
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

    /**
     * @brief Moving is only noexcept when moving a T is.
     *
     * std::vector can promise this unconditionally because its move only steals a pointer. In
     * direct mode we have to relocate the inline elements instead, which calls T's move
     * constructor, so the promise is only ours to make when that one is noexcept. Claiming it
     * anyway turns a throwing move into std::terminate, and makes std::move_if_noexcept pick us
     * up for a move where it should have fallen back to a copy. See issue #63.
     */
    svector(svector&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
        : svector() {
        do_move_assign(std::move(other));
    }

    svector(std::initializer_list<T> init)
        : svector(init.begin(), init.end()) {}

    ~svector() {
        destroy();
    }

    void assign(size_t count, T const& value) {
        if (is_reference_into_self(value)) {
            // clear() destroys every element, including the one value refers to.
            // Copy it to the stack first, then it's an ordinary assign. v.assign(1000, v[0])
            auto const tmp = value;
            assign(count, tmp);
            return;
        }
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

    // conditional for the same reason as the move constructor, see there
    auto operator=(svector&& other) noexcept(std::is_nothrow_move_constructible_v<T>) -> svector& {
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
        if (count > capacity()) {
            reserve(count);
        }
        if (is_direct()) {
            resize_after_reserve<direction::direct>(count);
        } else {
            resize_after_reserve<direction::indirect>(count);
        }
    }

    void resize(size_t count, T const& value) {
        if (is_reference_into_self(value)) {
            // reserve() below moves the elements into new storage and destroys the originals,
            // so value has to be copied out of the way first. v.resize(1000, v[0])
            // Deliberately not also testing count > capacity(): duplicating the condition
            // below would silently stop matching if that one ever changes.
            auto const tmp = value;
            resize(count, tmp);
            return;
        }
        if (count > capacity()) {
            reserve(count);
        }
        if (is_direct()) {
            resize_after_reserve<direction::direct>(count, value);
        } else {
            resize_after_reserve<direction::indirect>(count, value);
        }
    }

    void reserve(size_t s) {
        auto old_capacity = capacity();
        auto new_capacity = calculate_new_capacity(s, old_capacity);
        if (new_capacity > old_capacity) {
            realloc(new_capacity);
        }
    }

    [[nodiscard]] auto capacity() const noexcept -> size_t {
        if (is_direct()) {
            return capacity<direction::direct>();
        }
        return capacity<direction::indirect>();
    }

    [[nodiscard]] auto size() const noexcept -> size_t {
        if (is_direct()) {
            return size<direction::direct>();
        }
        return size<direction::indirect>();
    }

    [[nodiscard]] auto data() noexcept -> T* {
        if (is_direct()) {
            return direct_data();
        }
        return indirect()->data();
    }

    [[nodiscard]] auto data() const noexcept -> T const* {
        return const_cast<svector*>(this)->data(); // NOLINT(cppcoreguidelines-pro-type-const-cast)
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
            return emplace_back_grow(s, std::forward<Args>(args)...);
        }

        auto* ptr = (is_dir ? data<direction::direct>() : data<direction::indirect>()) + s;
        // construct before updating the size, so a throwing constructor doesn't leave the
        // vector claiming an element that was never built
        auto& element = *new (static_cast<void*>(ptr)) T(std::forward<Args>(args)...);
        set_size(s + 1);
        return element;
    }

    void push_back(T const& value) {
        emplace_back(value);
    }

    void push_back(T&& value) {
        emplace_back(std::move(value));
    }

    [[nodiscard]] auto operator[](size_t idx) const noexcept -> T const& {
        return *(data() + idx);
    }

    [[nodiscard]] auto operator[](size_t idx) noexcept -> T& {
        return *(data() + idx);
    }

    auto at(size_t idx) -> T& {
        if (is_direct()) {
            return at<direction::direct>(idx);
        }
        return at<direction::indirect>(idx);
    }

    auto at(size_t idx) const -> T const& {
        return const_cast<svector*>(this)->at(idx); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }

    [[nodiscard]] auto begin() const noexcept -> T const* {
        return data();
    }

    [[nodiscard]] auto cbegin() const noexcept -> T const* {
        return begin();
    }

    [[nodiscard]] auto begin() noexcept -> T* {
        return data();
    }

    [[nodiscard]] auto end() noexcept -> T* {
        if (is_direct()) {
            return data<direction::direct>() + size<direction::direct>();
        }
        return data<direction::indirect>() + size<direction::indirect>();
    }

    [[nodiscard]] auto end() const noexcept -> T const* {
        return const_cast<svector*>(this)->end(); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }

    [[nodiscard]] auto cend() const noexcept -> T const* {
        return end();
    }

    [[nodiscard]] auto rbegin() noexcept -> reverse_iterator {
        return reverse_iterator{end()};
    }

    [[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator {
        return crbegin();
    }

    [[nodiscard]] auto crbegin() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator{end()};
    }

    [[nodiscard]] auto rend() noexcept -> reverse_iterator {
        return reverse_iterator{begin()};
    }

    [[nodiscard]] auto rend() const noexcept -> const_reverse_iterator {
        return crend();
    }

    [[nodiscard]] auto crend() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator{begin()};
    }

    [[nodiscard]] auto front() const noexcept -> T const& {
        return *data();
    }

    [[nodiscard]] auto front() noexcept -> T& {
        return *data();
    }

    [[nodiscard]] auto back() noexcept -> T& {
        if (is_direct()) {
            return *(data<direction::direct>() + size<direction::direct>() - 1);
        }
        return *(data<direction::indirect>() + size<direction::indirect>() - 1);
    }

    [[nodiscard]] auto back() const noexcept -> T const& {
        return const_cast<svector*>(this)->back(); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }

    void clear() noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy(begin(), end());
        }

        if (is_direct()) {
            set_size<direction::direct>(0);
        } else {
            set_size<direction::indirect>(0);
        }
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return 0U == size();
    }

    void pop_back() noexcept {
        if (is_direct()) {
            pop_back<direction::direct>();
        } else {
            pop_back<direction::indirect>();
        }
    }

    [[nodiscard]] static auto max_size() noexcept -> size_t {
        return (std::numeric_limits<std::ptrdiff_t>::max)();
    }

    // std::swap does one move construction and two move assignments, so it inherits exactly
    // the condition those carry
    void swap(svector& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
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
        if (pos == cend()) {
            // no elements have to move out of the way, and emplace_back already builds the
            // new element before it grows, so args referencing us is fine there
            return std::addressof(emplace_back(std::forward<Args>(args)...));
        }

        // args may reference one of our own elements, and making space either shifts those
        // elements right or moves them into a new allocation, either of which leaves args
        // dangling. Build the element first. Inserting in the middle already moves every
        // element after pos, so one extra move does not change the cost.
        auto tmp = T(std::forward<Args>(args)...);
        return insert_n(pos, 1, place_moved(tmp));
    }

    // Both of these build the element before anything is shifted, which is what emplace() does, so
    // they go there. It costs one move, and it is what makes a single element insert either happen
    // or not happen at all: the copy is the only part that can throw, and by the time anything has
    // been touched it is already done. It also means value is allowed to be one of our own
    // elements, without a copy to get it out of the way first.
    auto insert(const_iterator pos, T const& value) -> iterator {
        return emplace(pos, value);
    }

    auto insert(const_iterator pos, T&& value) -> iterator {
        return emplace(pos, std::move(value));
    }

    auto insert(const_iterator pos, size_t count, T const& value) -> iterator {
        if (is_reference_into_self(value)) {
            // the shift moves our elements around and assigns over them, so value has to be
            // copied out of the way first. v.insert(v.begin(), 1000, v[0])
            auto const tmp = value;
            return insert(pos, count, tmp);
        }
        return insert_n(pos, count, place_copies{value});
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
    auto insert(const_iterator pos, It first, It last, std::forward_iterator_tag /*unused*/) -> iterator {
        auto const count = static_cast<size_t>(std::distance(first, last));
        return insert_n(pos, count, place_range<It>{first});
    }

    template <typename InputIt, typename = detail::enable_if_t<detail::is_input_iterator<InputIt>>>
    auto insert(const_iterator pos, InputIt first, InputIt last) -> iterator {
        return insert(pos, first, last, typename std::iterator_traits<InputIt>::iterator_category());
    }

    auto insert(const_iterator pos, std::initializer_list<T> l) -> iterator {
        return insert(pos, l.begin(), l.end());
    }

    /**
     * @brief Resizes to count elements, letting op initialize the new ones in place.
     *
     * Same contract as std::string::resize_and_overwrite, see
     * https://en.cppreference.com/w/cpp/string/basic_string/resize_and_overwrite
     *
     * op is called as op(p, count) with p == data(), and returns the actual new size:
     *  * p[0, min(count, size())) are the existing elements, readable and assignable.
     *  * p[min(count, size()), count) is raw uninitialized storage. op has to construct
     *    every element it wants to keep, e.g. with placement new.
     *  * op returns r, which must be in [0, count]. Afterwards size() == r, so p[0, r)
     *    must all be constructed objects when op returns.
     *
     * This skips the value-initialization that resize() would do, which is what makes it
     * faster: for e.g. reading into an svector<char> the zero fill is pure overhead.
     */
    template <class Operation>
    void resize_and_overwrite(size_t count, Operation op) {
        // step 1: make room. This preserves the existing elements and may switch to indirect mode.
        reserve(count);

        auto const old_size = size();
        if (count < old_size) {
            // Shrinking: the tail is gone. Commit the smaller size *before* running op, so that
            // if op throws, the destructor sees exactly the elements that are still alive.
            std::destroy_n(data() + count, old_size - count);
            set_size(count);
        }

        // step 2: op initializes [min(count, old_size), count) and tells us how much it kept.
        // The stored size is still min(count, old_size) here, so an exception escaping op
        // destroys the untouched prefix and leaks only what op itself constructed.
        auto const new_size = std::move(op)(data(), count);

        // step 3: commit. new_size <= count <= capacity() is a precondition, so in direct mode
        // this can never overflow the 7 bit size field.
        set_size(new_size);
    }

    auto erase(const_iterator pos) -> iterator {
        return erase(pos, pos + 1);
    }

    auto erase(const_iterator first, const_iterator last) -> iterator {
        if (is_direct()) {
            return erase_checked_end<direction::direct>(first, last);
        }
        return erase_checked_end<direction::indirect>(first, last);
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

} // namespace ANKERL_SVECTOR_NAMESPACE
} // namespace ankerl

namespace std {
// NOLINTNEXTLINE(cert-dcl58-cpp)
inline namespace ANKERL_SVECTOR_NAMESPACE {

template <class T, size_t N, class U>
constexpr auto erase(ankerl::svector<T, N>& sv, U const& value) -> typename ankerl::svector<T, N>::size_type {
    auto* removed_begin = std::remove(sv.begin(), sv.end(), value);
    auto num_removed = std::distance(removed_begin, sv.end());
    sv.erase(removed_begin, sv.end());
    return num_removed;
}

template <class T, size_t N, class Pred>
constexpr auto erase_if(ankerl::svector<T, N>& sv, Pred pred) -> typename ankerl::svector<T, N>::size_type {
    auto* removed_begin = std::remove_if(sv.begin(), sv.end(), pred);
    auto num_removed = std::distance(removed_begin, sv.end());
    sv.erase(removed_begin, sv.end());
    return num_removed;
}

} // namespace ANKERL_SVECTOR_NAMESPACE
} // namespace std

#endif
