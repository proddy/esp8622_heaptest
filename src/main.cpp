#include <Arduino.h>

#ifndef STANDALONE
static uint32_t heap_start_   = ESP.getFreeHeap();
static uint32_t old_free_heap = heap_start_;
static uint32_t mem_used      = 0;
#endif

namespace uuid {
std::string read_flash_string(const __FlashStringHelper * flash_str) {
    std::string str(::strlen_P(reinterpret_cast<PGM_P>(flash_str)), '\0');
    ::strncpy_P(&str[0], reinterpret_cast<PGM_P>(flash_str), str.capacity() + 1);
    return str;
}
} // namespace uuid

// NUM_ENTRIES
#define NUM_ENTRIES 200

// STRUCT_NUM
// 2 - uses std::function
// 3 - uses C void * function pointer
#define STRUCT_NUM 3

#if STRUCT_NUM == 2
#include <functional>
using mqtt_cmdfunction_p = std::function<void(const char * data, const int8_t id)>;
// no constructor, with std::function
// size on ESP8266 - 28 bytes (ubuntu 56, osx 80)
struct MQTTCmdFunction {
    uint8_t                     device_type_;      // 1 byte
    uint8_t                     dummy1_;           // 1 byte
    const __FlashStringHelper * dummy2_;           // 4
    const __FlashStringHelper * cmd_;              // 4
    mqtt_cmdfunction_p          mqtt_cmdfunction_; // 14
};
#endif

#if STRUCT_NUM == 3
using mqtt_cmdfunction_p = void (*)(const char *, const int8_t);
// no constructor, using C style function pointers instead of std::function
// size on ESP8266 - 16 bytes (ubuntu 32, osx 32)
struct MQTTCmdFunction {
    uint8_t                     device_type_;      // 1 byte
    uint8_t                     dummy1_;           // 1 byte
    const __FlashStringHelper * dummy2_;           // 4
    const __FlashStringHelper * cmd_;              // 4
    mqtt_cmdfunction_p          mqtt_cmdfunction_; // 6
};
#endif

// with constructor
// struct MQTTCmdFunction_constructor {
//     uint8_t                     device_type_;
//     uint8_t                     dummy1_;
//     const __FlashStringHelper * dummy2_;
//     const __FlashStringHelper * cmd_;
//     mqtt_cmdfunction_p          mqtt_cmdfunction_;

//     MQTTCmdFunction1(uint8_t device_type, uint8_t dummy1, const __FlashStringHelper * dummy2, const __FlashStringHelper * cmd, mqtt_cmdfunction_p mqtt_cmdfunction)
//         : device_type_(device_type)
//         , dummy1_(dummy1)
//         , dummy2_(dummy2)
//         , cmd_(cmd)
//         , mqtt_cmdfunction_(mqtt_cmdfunction) {
//     }
// };

// std::vector
// memory (since boot/of which is inplace) in bytes:
// with 2 (C style function pointers)
//
// with 3 (direct function)
//      4416, 4104, frag 9%, 22 bytes per element
// #include <vector>
// static std::vector<MQTTCmdFunction> mqtt_cmdfunctions_;

// std::list (is worst than queue and deque)
// memory (since boot/of which is inplace) in bytes:
// with 2 (C style function pointers)
//
// with 3 (direct function)
//      6712, 6400, frag 0%, 33 bytes per element
// #include <list>
// static std::list<MQTTCmdFunction> mqtt_cmdfunctions_;

// std::queue (is best from the std:: library with 20 bytes per element)
// memory (since boot/of which is inplace) in bytes:
// with 2 (C style function pointers)
//
// with 3 (direct function)
//      4032, 3160, frag 1%, 20 bytes per element
// #include <queue>
// static std::queue<MQTTCmdFunction> mqtt_cmdfunctions_;

// std::deque (is worst than queue!)
// memory (since boot/of which is inplace) in bytes:
// with 2 (C style function pointers)
//
// with 3 (direct function)
//      4552, 3680, frag 1%, 22 bytes per element
// #include <deque>
// static std::deque<MQTTCmdFunction> mqtt_cmdfunctions_;

// std::array - not well suited! - can't add and can't grow
// #include <array>
// static std::array<STRUCT, 200> mqtt_cmdfunctions_;

// ustd queue.h (better than std::list)
// memory (since boot/of which is inplace) in bytes:
// with 2
//      TBD
// with 3
//      3520, 0, frag 0%, 17 bytes per element
#include "queue.h"
static emsesp::queue<MQTTCmdFunction> mqtt_cmdfunctions_ = emsesp::queue<MQTTCmdFunction>(NUM_ENTRIES);

// ustd array.h
// memory (since boot/of which is inplace) in bytes:
// with 3
//      3520, 0 (201, 255, 16), 0% frag, 17 bytes per element
//      3648, 0 (16, 255, 16) with 12% frag! 18 bytes per element
//      3712, 1792 (100, 255, 16) 4% frag, starting at 100 elements and growing, 18 bytes per element
// with 2
//      7520, 1600 (lambda) 1% frag, 37 bytes per element
//      9120, 3200 (bind) 1% frag, 45 bytes per element <-- the worst
//      5920, 0, 0% frag (direct function) 29 bytes per element
//
// conclusion: use C function pointers, try to avoid growing because of fragmentation. but it's not too bad.
//
// #include "array.h"
// static emsesp::array<MQTTCmdFunction> mqtt_cmdfunctions_ = emsesp::array<MQTTCmdFunction>(NUM_ENTRIES, 255, 16);
// static emsesp::array<MQTTCmdFunction> mqtt_cmdfunctions_; // same as (16, 255, 16)
// static emsesp::array<MQTTCmdFunction> mqtt_cmdfunctions_ = emsesp::array<MQTTCmdFunction>(100, 255, 16); // start 100 and grow

//
// CODE below
//

void register_mqtt_cmd(uint8_t device_type, uint8_t dummy1, const __FlashStringHelper * dummy2, const __FlashStringHelper * cmd, mqtt_cmdfunction_p f) {
    MQTTCmdFunction mf;
    mf.device_type_      = device_type;
    mf.dummy1_           = dummy1;
    mf.dummy2_           = dummy2;
    mf.cmd_              = cmd;
    mf.mqtt_cmdfunction_ = f; // 2 - with using std::function or 3 with normal C function pointer

    // emplaces's
    // mqtt_cmdfunctions_.emplace_back(device_type, dummy1, dummy2, cmd, f); // std::list and std::vector
    // mqtt_cmdfunctions_.emplace(device_type, dummy1, dummy2, cmd, f); // std::queue
    // mqtt_cmdfunctions_.emplace_back(device_type, dummy1, dummy2, cmd, f); // std::deque

    // mqtt_cmdfunctions_.push_back(mf); // std::vector std::list
    // mqtt_cmdfunctions_.push_front(mf); // std::deque

    mqtt_cmdfunctions_.push(mf); // emsesp::queue and emsesp::array std::queue
}

// call back functions
void myFunction(const char * data, const int8_t id) {
    Serial.print(data);
    Serial.print(" ");
    Serial.print(id);
    Serial.print(" ");
}

// print out heap memory
void show_mem(const char * note) {
#ifndef STANDALONE
    static uint8_t old_heap_frag = 0;
    delay(100);
    yield(); // wait for CPU to catchup
    uint32_t free_heap = ESP.getFreeHeap();
#if defined ESP8266
    uint8_t heap_frag = ESP.getHeapFragmentation();
#else
    uint8_t heap_frag = 0;
#endif
    mem_used = heap_start_ - free_heap;
    Serial.printf("(%10s) started with %d, Free heap: %3d%% (%d) (~%d), frag:%d%% (~%d), used since boot: %d",
                  note,
                  heap_start_,
                  (100 * free_heap / heap_start_),
                  free_heap,
                  (uint32_t)abs(free_heap - old_free_heap),
                  heap_frag,
                  (uint8_t)abs(heap_frag - old_heap_frag),
                  mem_used);
    old_free_heap = free_heap;
    old_heap_frag = heap_frag;
    Serial.println();
#else
    Serial.println(note);
#endif
    Serial.flush();
}

// prints the contents of the queue
void print_queue(const char * s, emsesp::queue<uint8_t> & myQueue) {
    Serial.print(s);
    Serial.print(": ");
    for (const uint8_t & element : myQueue) {
        Serial.print(element);
        Serial.print(", ");
    }
    Serial.print("size=");
    Serial.print(myQueue.size());
    Serial.println();

    /*
    // old fashioned way
    Serial.print("traverse queue: ");
    for (uint8_t i = 0; i < myQueue.size(); i++) {
        Serial.print(myQueue[i]);
        Serial.print(", ");
    }
    Serial.println();
    */
}

void queue_test() {
    // queue test
    emsesp::queue<uint8_t> myQueue = emsesp::queue<uint8_t>(20);
    myQueue.push(1);
    myQueue.push(3);
    myQueue.push(5);
    myQueue.push(9);
    myQueue.push(11);

    print_queue("normal", myQueue);
    myQueue.push(12);
    print_queue("add 12", myQueue);
    Serial.print("Popping, Got ");
    Serial.println(myQueue.pop());
    Serial.print("Popping, Got ");
    Serial.println(myQueue.pop());
    print_queue("queue is now", myQueue);
    myQueue.push_back(13);
    print_queue("add 13", myQueue);
    myQueue.push_front(21);
    print_queue("add 21 to front", myQueue);
    myQueue.push_front(22);
    print_queue("add 22 to front", myQueue);
    Serial.print("Popping, Got ");
    Serial.println(myQueue.pop());

    // queue test2 - push_front on empty
    Serial.println();
    emsesp::queue<uint8_t> myQueue2 = emsesp::queue<uint8_t>(20);
    myQueue2.push_front(44);
    print_queue("add 44 to front", myQueue2);
    Serial.print("Popping, Got ");
    Serial.println(myQueue2.pop());

    // queue test3 - replace top when full
    Serial.println();
    emsesp::queue<uint8_t> myQueue3 = emsesp::queue<uint8_t>(5);
    myQueue3.push(11);
    myQueue3.push(21);
    myQueue3.push(31);
    myQueue3.push(41);
    bool a = myQueue3.push(51);
    Serial.print("Returns ");
    Serial.println(a);
    print_queue("5 elements (max)", myQueue3);
    bool b = myQueue3.push(61);
    Serial.print("Adding 61. Returns ");
    Serial.println(b);
    print_queue("5 elements (max)", myQueue3);
    Serial.print("Popping, Got ");
    Serial.println(myQueue3.pop());
    Serial.print("Popping, Got ");
    Serial.println(myQueue3.pop());
}

void setup() {
#ifndef STANDALONE
    Serial.begin(115200);
    Serial.println();
    Serial.printf("Starting heap: %d", heap_start_);
    Serial.println();
#endif
    show_mem("boot");

    // fill container
    show_mem("before");
    for (uint8_t i = 1; i <= NUM_ENTRIES; i++) {
#if STRUCT_NUM == 3
        register_mqtt_cmd(i, 10, F("hi"), F("tf3"), myFunction);
#endif

#if STRUCT_NUM == 2
        // register_mqtt_cmd(i, 10, F("hi"), F("tf2"), myFunction);
        // register_mqtt_cmd(i, 10, F("hi"), F("tf2"), std::bind(&myFunction, std::placeholders::_1, std::placeholders::_2));
        register_mqtt_cmd(i, 10, F("hi"), F("tf2"), [](const char * data, const int8_t id) { myFunction(data, id); });
#endif
    }
    show_mem("after");

    // using container iterators
    Serial.print("number of elements = ");
    Serial.print(mqtt_cmdfunctions_.size());
    Serial.println();
    for (const MQTTCmdFunction & mf : mqtt_cmdfunctions_) {
        (mf.mqtt_cmdfunction_)(uuid::read_flash_string(mf.cmd_).c_str(), mf.device_type_);
    }
    Serial.println();

    // sizes
    Serial.print("size of struct = ");
    Serial.print(sizeof(MQTTCmdFunction));
#ifndef STANDALONE
    Serial.print(", size per element = ");
    Serial.print(mem_used / NUM_ENTRIES);
#endif
    Serial.println();

    queue_test();

    Serial.println();
}

void loop() {
#ifndef STANDALONE
    // see if memory dissapears
    static uint32_t last_memcheck_ = 0;
    if (!last_memcheck_ || (millis() - last_memcheck_ > 10000)) { // 10 seconds
        last_memcheck_ = millis();
        show_mem("loop");
    }
#endif
}
