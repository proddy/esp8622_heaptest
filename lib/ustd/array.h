// array.h - array class
#pragma once

namespace ustd {

#define ARRAY_INC_SIZE 16
#define ARRAY_MAX_SIZE 65535 // 65535 or 4294967295 (mostly)
#define ARRAY_INIT_SIZE 16

/*! \brief Lightweight c++11 array implementation.

array.h is a minimal, yet highly portable array data type implementation
that runs well on architectures with very limited resources such as attiny 8kb
avr.

The array class either:

* * allocates memory dynamically on array-writes or array-reads, or
* * work in fully static mode without any dynamic allocation once the array
object has been created.

The library header-only.

Make sure to provide the <a
href="https://github.com/muwerk/ustd/blob/master/README.md">required platform
define</a> before including ustd headers.

## An example for dynamic mode:

~~~{.cpp}
#define __ATTINY__ 1   // Platform defines required, see doc, mainpage.
#include <array.h>

ustd::array<int> intArray;

intArray[0] = 13; // Memory for array[0] is allocated
intArray.add(3);  // the array is extended, if necessary
int p = intArray[0];

printf("[0]:%d [1]:%d length=%d\n", intArray[0], intArray[1], intArray.length())
~~~

## An example for static mode

~~~{.cpp}
#include <array.h>

// array length is fixed 5 (startSize==maxSize), no dynamic extensions:
ustd::array<int> intArray = ustd::array<int>(5, 5, 0, false);
~~~
 */

template <typename T>
class array {
  private:
    T *          arr;
    unsigned int startSize;
    unsigned int maxSize;
    unsigned int incSize = ARRAY_INC_SIZE;
    unsigned int allocSize;
    unsigned int size;
    T            bad;

    T * ualloc(unsigned int n) {
        return new T[n];
    }
    void ufree(T * p) {
        delete[] p;
    }

  public:
    array(unsigned int startSize = ARRAY_INIT_SIZE, unsigned int maxSize = ARRAY_MAX_SIZE, unsigned int incSize = ARRAY_INC_SIZE)
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

    bool resize(unsigned int newSize) {
        /*! Change the array allocation size.
         *
         * Note: Usage of this function is optional for optimization. By
         * default, all necessary allocations (and deallocations, if shrink=true
         * during construction was set) are handled automatically.
         * @param newSize the new number of array entries, corresponding memory
         * is allocated/freed as necessary.
         */
        unsigned int mv = newSize;
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
        for (unsigned int i = 0; i < mv; i++) {
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

    unsigned int length() const {
        /*! Check number of array-members.
        @return number of array entries */
        return (size);
    }

    unsigned int alloclen() const {
        /*! Check the number of allocated array-entries, which can be larger
         * than the length of the array.
         * @return number of allocated entries. */
        return (allocSize);
    }
};


} // namespace ustd