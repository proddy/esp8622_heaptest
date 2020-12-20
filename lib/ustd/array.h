/*
 * Lightweight c++11 array implementation
 * Based on https://github.com/muwerk/ustd
 * Limits to max 255 entries
 */
#pragma once

#if defined USTD_ASSERT
#include <assert.h>
#endif

namespace ustd {

#define ARRAY_INC_SIZE 16
#define ARRAY_MAX_SIZE 255
#define ARRAY_INIT_SIZE 16

template <typename T>
class arrayIterator {
  public:
    arrayIterator(T * values_ptr)
        : values_ptr_{values_ptr}
        , position_{0} {
    }

    arrayIterator(T * values_ptr, size_t size_)
        : values_ptr_{values_ptr}
        , position_{size_} {
    }

    bool operator!=(const arrayIterator<T> & other) const {
        return !(*this == other);
    }

    bool operator==(const arrayIterator<T> & other) const {
        return position_ == other.position_;
    }

    arrayIterator & operator++() {
        ++position_;
        return *this;
    }

    T & operator*() const {
        return *(values_ptr_ + position_);
    }

  private:
    T *    values_ptr_;
    size_t position_;
};

template <typename T>
class array {
  private:
    T *     arr_;
    uint8_t startSize_;
    uint8_t maxSize_;
    uint8_t incSize_ = ARRAY_INC_SIZE;
    uint8_t allocSize_;
    uint8_t size_;
    T       bad_;

  public:
    // Constructs an array object. All allocation-hints are optional, the
    // array class will allocate memory as needed during writes, if
    // startSize_!=maxSize_.
    // @param startSize_ The number of array entries that are allocated
    // during object creation
    // @param maxSize_ The maximal limit of records that will be allocated.
    // If startSize_ < maxSize_, the array size_ will grow automatically as
    // needed.
    // @param incSize_ The number of array entries that are allocated as a
    // chunk if the array needs to grow
    array(uint8_t startSize_ = ARRAY_INIT_SIZE, uint8_t maxSize_ = ARRAY_MAX_SIZE, uint8_t incSize_ = ARRAY_INC_SIZE)
        : startSize_(startSize_)
        , maxSize_(maxSize_)
        , incSize_(incSize_) {
        size_ = 0;
        memset(&bad_, 0, sizeof(bad_));
        if (maxSize_ < startSize_)
            maxSize_ = startSize_;
        allocSize_ = startSize_;
        arr_       = new T[allocSize_];
    }

    ~array() {
        /*! Free resources */
        if (arr_ != nullptr) {
            delete[] arr_;
            arr_ = nullptr;
        }
    }

    // Change the array allocation size_. the new number of array entries, corresponding memory is allocated/free'd as necessary.
    bool resize(uint8_t newSize) {
        uint8_t mv = newSize;
        if (newSize > maxSize_) {
            if (maxSize_ == allocSize_)
                return false;
            else
                newSize = maxSize_;
        }
        if (newSize <= allocSize_)
            return true;
        T * arrn = new T[newSize];
        if (arrn == nullptr)
            return false;
        for (uint8_t i = 0; i < mv; i++) {
            arrn[i] = arr_[i];
        }
        delete[] arr_;
        arr_       = arrn;
        allocSize_ = newSize;
        return true;
    }

    // Set the value for <T>entry that's given back,
    // if read of an invalid index is requested.
    // By default, an entry all memset to zero is given
    // back. Using this function, the value of an invalid read can be configured.
    // returns the value that is given back in case an invalid operation (e.g. read out of bounds) is tried
    void setInvalidValue(T & entryInvalidValue) {
        bad_ = entryInvalidValue;
    }

    // Append an array element after the current end of the array
    // takes entry array element that is appended after the last current
    // entry. The new array size_ must be smaller than maxSize_ as defined
    // during array creation. New array memory is automatically allocated if
    // within maxSize_ boundaries
    int push(T & entry) {
        if (size_ >= allocSize_) {
            if (incSize_ == 0)
                return -1;
            if (!resize(allocSize_ + incSize_))
                return -1;
        }
        arr_[size_] = entry;
        ++size_;
        return size_ - 1;
    }

    // Assign content of array element at i for const's
    T operator[](unsigned int i) const {
        if (i >= allocSize_) {
            if (incSize_ == 0) {
#ifdef USTD_ASSERT
                assert(i < allocSize_);
#endif
            }
            if (!resize(allocSize_ + incSize_)) {
#ifdef USTD_ASSERT
                assert(i < allocSize_);
#endif
            }
        }
        if (i >= size_ && i <= allocSize_)
            size_ = i + 1;
        if (i >= allocSize_) {
            return bad_;
        }
        return arr_[i];
    }

    // Assign content of array element at i
    T & operator[](unsigned int i) {
        if (i >= allocSize_) {
            if (incSize_ == 0) {
#ifdef USTD_ASSERT
                assert(i < allocSize_);
#endif
            }
            if (!resize(allocSize_ + incSize_)) {
#ifdef USTD_ASSERT
                assert(i < allocSize_);
#endif
            }
        }
        if (i >= size_ && i <= allocSize_)
            size_ = i + 1;
        if (i >= allocSize_) {
            return bad_;
        }
        return arr_[i];
    }

    // true if array empty, false otherwise
    bool empty() const {
        if (size_ == 0)
            return true;
        else
            return false;
    }

    // return number of array elements
    uint8_t size() const {
        return (size_);
    }

    // returns number of allocated entries which can be larger than the length of the array
    uint8_t alloclen() const {
        return (allocSize_);
    }

    // // emplace
    // template <typename... Args>
    // void emplace1(Args... args) {
    //     add(args...);
    // };

    // template <class... Args>
    // void emplace(Args &&... args) {
    //     add(T(std::forward<Args>(args)...));
    // }

    // iterators
    arrayIterator<T> begin() {
        return arrayIterator<T>(arr_);
    }
    arrayIterator<T> end() {
        return arrayIterator<T>(arr_, size_);
    }

    arrayIterator<const T> begin() const {
        return arrayIterator<const T>(arr_);
    }

    arrayIterator<const T> end() const {
        return arrayIterator<const T>(arr_, size_);
    }
};


} // namespace ustd