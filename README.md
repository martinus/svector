<a id="top"></a>
# ankerl::svector

# svector

`ankerl::svector` is an `std::vector` like container that can hold some elements on the heap without the need for any allocation.
This implementation is special in that it is highly compact.

|                            | std::vector | boost :: container :: small_vector | absl :: InlinedVector | ankerl::svector |
|----------------------------|------------:|-----------------------------------:|----------------------:|----------------:|
| minimum heap size          |          24 |                                 32 |                    24 |           **8** |
| inline capacity at 8 byte  |           0 |                                  0 |                     0 |           **7** |
| inline capacity at 16 byte |           0 |                                  0 |                     0 |          **15** |
| inline capacity at 24 byte |           0 |                                  0 |                    16 |          **23** |
| inline capacity at 32 byte |           0 |                                  8 |                    24 |          **31** |
| inline capacity at 64 byte |           0 |                                 40 |                    56 |          **63** |
| minimum inline capacity    |       **0** |                              **0** |                     1 |           **0** |
|                            |             |                                    |                       |                 |

## FAQ

* **Does this work?**
  
  Yes.

* **Are you sure?**

  Absolutely not! This implementation is barely tested and not feature complete. It might set your computer on fire.



Alternatives:


## llvm/ADT/SmallVector.h

* min: 32byte
* fits 8 byte SVO without getting bigger

https://github.com/thelink2012/SmallVector/blob/master/SmallVector.h

* members:
  * `void *BeginX, *EndX, *CapacityX;`
  * 1 element in the base class: std::aligned_union<1, T> FirstEl;
  * N-1 elements in SmallVectorStorage
* minimum size: 32byte
* uses `std::aligned_union`

https://godbolt.org/z/3TE1dsjxb

## KonanM/small_vector

* min: 32byte
* fits 6 bytes SVO without getting bigger

https://github.com/KonanM/small_vector

It's actually an `std::vector` but with a special `small_buffer_vector_allocator` that stores some data on the stack.

Neat idea, because its simple. A bit slower than custom implementations. Has some benchmarks.

# bensaratikian/small_vector

* min: 40byte
* fits 24 bytes SVO without getting bigger

https://github.com/bensaratikian/small_vector

Simple implementation with a union of std::vector and data, and separate `size` and `isCArray`. 40 bytes minimum.

# p-ranav/small_vector

* min: 40byte
* fits 24 bytes SVO without getting bigger
* 
https://github.com/p-ranav/small_vector

3 members, no union:

```cpp
std::array<T, N> stack_;
std::vector<T, Allocator> heap_;
std::size_t size_{0};
```

Needs minor code changes to compile https://godbolt.org/z/Pv6evaqqv

# gharveymn/small_vector

https://github.com/gharveymn/small_vector 

* min: 32byte
* fits 8 bytes SVO without getting bigger

Looks very full featured, lots of documentation.
```cpp
std::aligned_storage<element_size (), alignment ()>::type
ptr     m_data_ptr;
size_ty m_capacity;
size_ty m_size;
```

# boost::container::small_vector

* min 32 byte
* fits 15 byte SVO without getting bigger

https://www.boost.org/doc/libs/1_79_0/doc/html/boost/container/small_vector.html

# folly::small_vector

Customizeable size type: https://github.com/facebook/folly/blob/main/folly/docs/small_vector.md

Loads of dependencies... https://github.com/facebook/folly/blob/main/folly/small_vector.h

```cpp
union Data {
    PointerType pdata_;
    // value_type* heap_;
    // InternalSizeType capacity_;
    // InlineStorageType storage_;
} u;
```


# absl::InlinedVector

* min: 24 byte
* fits 16 byte

