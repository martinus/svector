<a id="top"></a>

# ankerl::svector

`ankerl::svector` is an `std::vector`-like container that can hold some elements on the heap without the need for any allocation.
There lots of small vector implementations ([absl](https://github.com/abseil/abseil-cpp/blob/master/absl/container/inlined_vector.h), [boost](https://www.boost.org/doc/libs/1_79_0/doc/html/boost/container/small_vector.html), [folly](https://github.com/facebook/folly/blob/main/folly/docs/small_vector.md), [llvm](https://llvm.org/doxygen/classllvm_1_1SmallVector.html), ...), but this here is special by how compact it is:

The smallest `ankerl::svector` is just 8 bytes large. E.g. `ankerl::svector<std::byte, 7>` can store 7 bytes on the heap without allocating, yet it's `sizeof()` is only 8. As far as I know, no other implementation is that compact. Most implementation need at least 24 or 32 bytes.

Here is a table comparing the amount of bytes you can store on the heap for a given total heap size of the vector:

| heap size | boost::container::small_vector |absl::InlinedVector | ankerl::svector |
|-----------|-------------------------------:|-------------------:|----------------:|
|         8 |                              - |                  - |           **7** |
|        16 |                              - |                  - |          **15** |
|        24 |                              - |                 16 |          **23** |
|        32 |                              8 |                 24 |          **31** |
|        40 |                             16 |                 32 |          **31** |
|        48 |                             24 |                 40 |          **31** |
|        56 |                             32 |                 48 |          **31** |
|        64 |                             40 |                 56 |          **63** |

In short:

* 24 bytes overhead for `boost::container::small_vector`
*  8 bytes overhead for `absl::InlinedVector`
* 1 (one!) byte overhead for `ankerl::svector`.


## Disclaimer

`ankerl::svector` is new, relatively untested, not feature complete. Using it might set your computer on fire.

## TODO

* Implement most of [std::vector's API](https://en.cppreference.com/w/cpp/container/vector)
* More tests, tests, tests!
* Make it work on big endian machines too

