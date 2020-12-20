/*
 * Lightweight c++11 array implementation.
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

    arrayIterator(T * values_ptr, size_t size)
        : values_ptr_{values_ptr}
        , position_{size} {
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
    T *     arr;
    uint8_t startSize;
    uint8_t maxSize;
    uint8_t incSize = ARRAY_INC_SIZE;
    uint8_t allocSize;
    uint8_t size;
    T       bad;

    T * ualloc(uint8_t n) {
        return new T[n];
    }
    void ufree(T * p) {
        delete[] p;
    }

  public:
    array(uint8_t startSize = ARRAY_INIT_SIZE, uint8_t maxSize = ARRAY_MAX_SIZE, uint8_t incSize = ARRAY_INC_SIZE)
        : startSize(startSize)
        , maxSize(maxSize)
        , incSize(incSize) {
        /*!
         * Constructs an array object. All allocation-hints are optional, the
         * array class will allocate memory as needed during writes, if
         * startSize!=maxSize.
         * @param startSize The number of array entries that are allocated
         * during object creation
         * @param maxSize The maximal limit of records that will be allocated.
         * If startSize < maxSize, the array size will grow automatically as
         * needed.
         * @param incSize The number of array entries that are allocated as a
         * chunk if the array needs to grow
         */
        size = 0;
        memset(&bad, 0, sizeof(bad));
        if (maxSize < startSize)
            maxSize = startSize;
        allocSize = startSize;
        arr       = ualloc(allocSize); // new T[allocSize];
    }

    ~array() {
        /*! Free resources */
        if (arr != nullptr) {
            ufree(arr);
            arr = nullptr;
        }
    }

    bool resize(uint8_t newSize) {
        /*! Change the array allocation size.
         *
         * Note: Usage of this function is optional for optimization. By
         * default, all necessary allocations (and deallocations, if shrink=true
         * during construction was set) are handled automatically.
         * @param newSize the new number of array entries, corresponding memory
         * is allocated/freed as necessary.
         */
        uint8_t mv = newSize;
        if (newSize > maxSize) {
            if (maxSize == allocSize)
                return false;
            else
                newSize = maxSize;
        }
        if (newSize <= allocSize)
            return true;
        T * arrn = ualloc(newSize); // new T[newSize];
        if (arrn == nullptr)
            return false;
        for (uint8_t i = 0; i < mv; i++) {
            arrn[i] = arr[i];
        }
        ufree(arr);
        arr       = arrn;
        allocSize = newSize;
        return true;
    }

    void setInvalidValue(T & entryInvalidValue) {
        /*! Set the value for <T>entry that's given back, if read of an invalid
        index is requested. By default, an entry all memset to zero is given
        back. Using this function, the value of an invalid read can be
        configured.
        * @param entryInvalidValue The value that is given back in case an
        invalid operation (e.g. read out of bounds) is tried.
        */
        bad = entryInvalidValue;
    }

    int add(T & entry) {
        /*! Append an array element after the current end of the array
         * @param entry array element that is appended after the last current
         * entry. The new array size must be smaller than maxSize as defined
         * during array creation. New array memory is automatically allocated if
         * within maxSize boundaries. */
        if (size >= allocSize) {
            if (incSize == 0)
                return -1;
            if (!resize(allocSize + incSize))
                return -1;
        }
        arr[size] = entry;
        ++size;
        return size - 1;
    }

    T operator[](unsigned int i) const {
        /*! Read content of array element at i, a=myArray[3] */
        if (i >= allocSize) {
            if (incSize == 0) {
#ifdef USTD_ASSERT
                assert(i < allocSize);
#endif
            }
            if (!resize(allocSize + incSize)) {
#ifdef USTD_ASSERT
                assert(i < allocSize);
#endif
            }
        }
        if (i >= size && i <= allocSize)
            size = i + 1;
        if (i >= allocSize) {
            return bad;
        }
        return arr[i];
    }

    T & operator[](unsigned int i) {
        /*! Assign content of array element at i, e.g. myArray[3]=3 */
        if (i >= allocSize) {
            if (incSize == 0) {
#ifdef USTD_ASSERT
                assert(i < allocSize);
#endif
            }
            if (!resize(allocSize + incSize)) {
#ifdef USTD_ASSERT
                assert(i < allocSize);
#endif
            }
        }
        if (i >= size && i <= allocSize)
            size = i + 1;
        if (i >= allocSize) {
            return bad;
        }
        return arr[i];
    }

    bool isEmpty() const {
        /*! Check, if array is empty.
        @return true if array empty, false otherwise. s*/
        if (size == 0)
            return true;
        else
            return false;
    }

    uint8_t length() const {
        /*! Check number of array-members.
        @return number of array entries */
        return (size);
    }

    uint8_t alloclen() const {
        /*! Check the number of allocated array-entries, which can be larger
         * than the length of the array.
         * @return number of allocated entries. */
        return (allocSize);
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
        return arrayIterator<T>(arr);
    }
    arrayIterator<T> end() {
        return arrayIterator<T>(arr, size);
    }

    arrayIterator<const T> begin() const {
        return arrayIterator<const T>(arr);
    }

    arrayIterator<const T> end() const {
        return arrayIterator<const T>(arr, size);
    }
};


} // namespace ustd