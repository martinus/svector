<a id="top"></a>

# ankerl::svector 🚚

`ankerl::svector` is an `std::vector`-like container that can hold some elements on the heap without the need for any allocation.
There are lots of small vector implementations ([absl](https://github.com/abseil/abseil-cpp/blob/master/absl/container/inlined_vector.h), [boost](https://www.boost.org/doc/libs/1_79_0/doc/html/boost/container/small_vector.html), [folly](https://github.com/facebook/folly/blob/main/folly/docs/small_vector.md), [llvm](https://llvm.org/doxygen/classllvm_1_1SmallVector.html), ...) but this here is special by how compact it is.

The smallest `ankerl::svector` is just 8 bytes large (on 64bit machines). E.g. `ankerl::svector<std::byte, 7>` can store 7 bytes inline on the stack without allocating, yet it's `sizeof()` is only 8. As far as I know, no other implementation is that compact. Most implementation need at least 24 or 32 bytes.

Here is a table comparing the amount of bytes you can store on the heap for a given total heap size of the vector:

|                                  |  8  |  16  |  24  |  32  |  40  |  48  |  56  |  64  |
|----------------------------------|----:|-----:|-----:|-----:|-----:|-----:|-----:|-----:|
| `boost::container::small_vector` |  -  |   -  |   -  |   8  |  16  |  24  |  32  |  40  |
| `absl::InlinedVector`            |  -  |   -  |  16  |  24  |  32  |  40  |  48  |  56  |
| `ankerl::svector`                |**7**|**15**|**23**|**31**|**39**|**47**|**55**|**63**|

In short:

* 24 bytes overhead for `boost::container::small_vector`
* 8 bytes overhead for `absl::InlinedVector`
* 1 (one!) byte overhead for `ankerl::svector`.

Note that boost's `small_vector` cheats a bit. E.g. `boost::container::small_vector<std::byte, 15>` won't give you 15 elements on the heap, but only 8 bytes. It seems to round down to the closest multiple of 8 bytes.

## Disclaimer

`ankerl::svector` is new and relatively untested! I have implemented and tested all of std::vector's API though, and have 100% test coverage. Still, using it might set your computer on fire.
