#include <app/Counter.h>

#include <fmt/format.h>

Counter::Obj::Obj()
    : mData(0)
    , mCounts(nullptr) {
    ++staticDefaultCtor;
}

Counter::Obj::Obj(const size_t& data, Counter& counts)
    : mData(data)
    , mCounts(&counts) {
    ++mCounts->ctor;
}

Counter::Obj::Obj(const Counter::Obj& o)
    : mData(o.mData)
    , mCounts(o.mCounts) {
    if (nullptr != mCounts) {
        ++mCounts->copyCtor;
    }
}

Counter::Obj::Obj(Counter::Obj&& o) noexcept
    : mData(o.mData)
    , mCounts(o.mCounts) {
    if (nullptr != mCounts) {
        ++mCounts->moveCtor;
    }
}

Counter::Obj::~Obj() {
    if (nullptr != mCounts) {
        ++mCounts->dtor;
    } else {
        ++staticDtor;
    }
}

auto Counter::Obj::operator==(const Counter::Obj& o) const -> bool {
    if (nullptr != mCounts) {
        ++mCounts->equals;
    }
    return mData == o.mData;
}

auto Counter::Obj::operator<(const Obj& o) const -> bool {
    if (nullptr != mCounts) {
        ++mCounts->less;
    }
    return mData < o.mData;
}

// NOLINTNEXTLINE(bugprone-unhandled-self-assignment,cert-oop54-cpp)
auto Counter::Obj::operator=(const Counter::Obj& o) -> Counter::Obj& {
    mCounts = o.mCounts;
    if (nullptr != mCounts) {
        ++mCounts->assign;
    }
    mData = o.mData;
    return *this;
}

auto Counter::Obj::operator=(Counter::Obj&& o) noexcept -> Counter::Obj& {
    if (nullptr != o.mCounts) {
        mCounts = o.mCounts;
    }
    mData = o.mData;
    if (nullptr != mCounts) {
        ++mCounts->moveAssign;
    }
    return *this;
}

auto Counter::Obj::get() const -> size_t const& {
    if (nullptr != mCounts) {
        ++mCounts->constGet;
    }
    return mData;
}

auto Counter::Obj::get() -> size_t& {
    if (nullptr != mCounts) {
        ++mCounts->get;
    }
    return mData;
}

void Counter::Obj::swap(Obj& other) {
    using std::swap;
    swap(mData, other.mData);
    swap(mCounts, other.mCounts);
    if (nullptr != mCounts) {
        ++mCounts->swaps;
    }
}

auto Counter::Obj::getForHash() const -> size_t {
    if (nullptr != mCounts) {
        ++mCounts->hash;
    }
    return mData;
}

Counter::Counter() {
    Counter::staticDefaultCtor = 0;
    Counter::staticDtor = 0;
    printHeaderOnce();
}

void Counter::printHeaderOnce() {
    static bool isFirst = true;
    if (isFirst) {
        fmt::print("     ctor  defctor  cpyctor     dtor   assign    swaps      get  cnstget     "
                   "hash   equals     less   ctormv assignmv |    total\n");
        isFirst = false;
    }
}

void Counter::printCounts(std::string_view title) const {
    size_t total = ctor + staticDefaultCtor + copyCtor + (dtor + staticDtor) + equals + less + assign + swaps + get +
                   constGet + hash + moveCtor + moveAssign;

    fmt::print("{:9}{:9}{:9}{:9}{:9}{:9}{:9}{:9}{:9}{:9}{:9}{:9}{:9} |{:9} {}\n",
               ctor,
               staticDefaultCtor,
               copyCtor,
               dtor + staticDtor,
               assign,
               swaps,
               get,
               constGet,
               hash,
               equals,
               less,
               moveCtor,
               moveAssign,
               total,
               title);
}

void Counter::reset() {
    ctor = 0;
    copyCtor = 0;
    dtor = 0;
    equals = 0;
    less = 0;
    assign = 0;
    swaps = 0;
    get = 0;
    constGet = 0;
    hash = 0;
    moveCtor = 0;
    moveAssign = 0;
}

size_t Counter::staticDefaultCtor = 0;
size_t Counter::staticDtor = 0;
