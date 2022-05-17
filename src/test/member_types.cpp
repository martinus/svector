#include <ankerl/svector.h>

#include <string>

namespace {

using Vec = ankerl::svector<std::string, 3>;

// see https://en.cppreference.com/w/cpp/container/vector

static_assert(std::is_same_v<Vec::value_type, std::string>);
// no allocator_type
static_assert(std::is_same_v<Vec::size_type, std::size_t>);
static_assert(std::is_same_v<Vec::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<Vec::reference, Vec::value_type&>);
static_assert(std::is_same_v<Vec::const_reference, Vec::value_type const&>);
static_assert(std::is_same_v<Vec::pointer, std::string*>);
static_assert(std::is_same_v<Vec::const_pointer, std::string const*>);
static_assert(std::is_same_v<Vec::iterator, std::string*>);
static_assert(std::is_same_v<Vec::const_iterator, std::string const*>);
static_assert(std::is_same_v<Vec::reverse_iterator, std::reverse_iterator<Vec::iterator>>);
static_assert(std::is_same_v<Vec::const_reverse_iterator, std::reverse_iterator<Vec::const_iterator>>);

} // namespace
