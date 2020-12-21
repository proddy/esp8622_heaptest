/*
 * Lightweight c++11 ring buffer queue implementation
 * Based on https://github.com/muwerk/ustd
 * Limits to max 255 entries
 */

#pragma once

namespace emsesp {

template <typename T>
class queueIterator {
  public:
    queueIterator(T * values_ptr, uint8_t p)
        : values_ptr_{values_ptr}
        , position_{p} {
    }

    bool operator!=(const queueIterator<T> & other) const {
        return !(*this == other);
    }

    bool operator==(const queueIterator<T> & other) const {
        return position_ == other.position_;
    }

    queueIterator & operator++() {
        ++position_;
        return *this;
    }

    T & operator*() const {
        return *(values_ptr_ + position_);
    }

  private:
    T *     values_ptr_;
    uint8_t position_;
};

template <class T>
class queue {
  private:
    T *     que_;
    uint8_t peakSize_;
    uint8_t maxSize_;
    uint8_t size_;
    uint8_t quePtrFront_; // back
    uint8_t quePtrBack_;  // front
    T       bad_;

  public:
    // Constructs a queue object with the maximum number of <T> pointer entries
    queue(uint8_t maxQueueSize)
        : maxSize_(maxQueueSize) {
        memset(&bad_, 0, sizeof(bad_));
        quePtrFront_ = 0;
        quePtrBack_  = 0;
        size_        = 0;
        peakSize_    = 0;
        que_         = (T *)malloc(sizeof(T) * maxSize_);
        if (que_ == nullptr)
            maxSize_ = 0;
    }

    // Deallocate the queue structure
    ~queue() {
        if (que_ != nullptr) {
            free(que_);
            que_ = nullptr;
        }
    }

    // Push a new entry into the queue.
    // true on success, false if queue is full, then the element is relaced with the front one
    bool push(T ent) {
        Serial.print("quePtrFront_: ");
        Serial.print(quePtrFront_);
        Serial.print(" quePtrBack_: ");
        Serial.print(quePtrBack_);
        Serial.println();
        if (size_ >= maxSize_) {
            // que_[quePtrFront_] = ent;
            return false;
        }
        que_[quePtrBack_] = ent;
        quePtrBack_       = (quePtrBack_ + 1) % maxSize_;
        ++size_;
        if (size_ > peakSize_) {
            peakSize_ = size_;
        }
        return true;
    }

    bool push_back(T ent) {
        return push(ent);
    }

    // Push a new entry into the front of queue, moving the rest down
    // true on success, false if queue is full
    // there are no good checks for overflow
    bool push_front(T ent) {
        if (size_ >= maxSize_) {
            return false;
        }
        // Serial.print("quePtrFront_: ");
        // Serial.print(quePtrFront_);
        // Serial.print(" quePtrBack_: ");
        // Serial.print(quePtrBack_);
        // Serial.println();

        for (uint8_t i = 0; i <= size_; i++) {
            que_[quePtrBack_ - i + 1] = que_[quePtrBack_ - i]; // move the rest up 1
        }
        que_[quePtrFront_] = ent; // add the new one
        quePtrBack_        = (quePtrBack_ + 1) % maxSize_;
        ++size_;
        if (size_ > peakSize_) {
            peakSize_ = size_;
        }
        return true;
    }

    T & operator[](unsigned int i) {
        return que_[i + quePtrFront_];
    }

    // Pop the oldest entry from the queue
    T pop() {
        if (size_ == 0)
            return bad_;
        T ent        = que_[quePtrFront_];
        quePtrFront_ = (quePtrFront_ + 1) % maxSize_;
        --size_;
        return ent;
    }

    // alias pop_front to keep backwards compatibility with std::list/queue
    T pop_front() {
        pop();
    }

    // Set the value for <T>entry that's given back, if read from an empty
    // queue is requested. By default, an entry all memset to zero is given
    // back. Using this function, the value of an invalid read can be configured
    void setInvalidValue(T & entryInvalidValue) {
        bad_ = entryInvalidValue;
    }

    // returns true: queue empty, false: not empty
    bool empty() {
        if (size_ == 0)
            return true;
        else
            return false;
    }

    // returns number of entries in the queue
    uint8_t size() {
        return (size_);
    }

    // max number of queue entries that have been in the queue
    uint8_t peak() {
        return (peakSize_);
    }

    // iterators
    queueIterator<T> begin() {
        return queueIterator<T>(que_, quePtrFront_);
    }
    queueIterator<T> end() {
        return queueIterator<T>(que_, quePtrFront_ + size_);
    }

    queueIterator<const T> begin() const {
        return queueIterator<const T>(que_, quePtrFront_);
    }

    queueIterator<const T> end() const {
        return queueIterator<const T>(que_, quePtrFront_ + size_);
    }
};

} // namespace emsesp