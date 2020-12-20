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

/* structs */

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
// no constructor, no std::function but function ptr
// size on ESP8266 - 16 bytes (ubuntu 32, osx 32)
struct MQTTCmdFunction {
    uint8_t                     device_type_;      // 1 byte
    uint8_t                     dummy1_;           // 1 byte
    const __FlashStringHelper * dummy2_;           // 4
    const __FlashStringHelper * cmd_;              // 4
    mqtt_cmdfunction_p          mqtt_cmdfunction_; // 6
};
#endif

/*
// with constructor
struct MQTTCmdFunction_constructor {
    uint8_t                     device_type_;
    uint8_t                     dummy1_;
    const __FlashStringHelper * dummy2_;
    const __FlashStringHelper * cmd_;
    mqtt_cmdfunction_p          mqtt_cmdfunction_;

    MQTTCmdFunction1(uint8_t device_type, uint8_t dummy1, const __FlashStringHelper * dummy2, const __FlashStringHelper * cmd, mqtt_cmdfunction_p mqtt_cmdfunction)
        : device_type_(device_type)
        , dummy1_(dummy1)
        , dummy2_(dummy2)
        , cmd_(cmd)
        , mqtt_cmdfunction_(mqtt_cmdfunction) {
    }
};
*/

// std::vector - 9336
// #include <vector>
// static std::vector<STRUCT> mqtt_cmdfunctions_;

// std::list - 10160
// #include <list>
// static std::list<STRUCT> mqtt_cmdfunctions_;

// std::queue - 7832
// #include <queue>
// static std::queue<STRUCT> mqtt_cmdfunctions_;

// std::deque - 7832, 6232
// #include <deque>
// static std::deque<STRUCT> mqtt_cmdfunctions_;

// std::array - not well suited! - can't add and can't grow
// #include <array>
// static std::array<STRUCT, 200> mqtt_cmdfunctions_;

// home grown Vector - doesn't use heap so not suitable
// #include <Vector.h>
// static Vector<STRUCT> mqtt_cmdfunctions_;

// ustd queue.h - not dynamic, only fixed size - 2160 (same as array) - or 560 when not using bind::
// #include "queue.h"
// static ustd::queue<STRUCT> mqtt_cmdfunctions_ = ustd::queue<STRUCT>(200);

// ustd array.h - fixed size - 2160, or 560 when not using bind::
// memory (since boot/of which is inplace) in bytes:
//  with 3 (direct function)
//           3520, 0 (201, 255, 16), 0% frag, 17 bytes per element
//           3648, 0 (16, 255, 16) with 12% frag! 18 bytes per element
//           3712, 1792 (100, 255, 16) 4% frag, starting at 100 elements and growing, 18 bytes per element
//  with 2: 7520, 1600 (lambda) 1% frag, 37 bytes per element
//          9120, 3200 (bind) 1% frag, 45 bytes per element <-- the worst
//          5920, 0, 0% frag (direct function) 29 bytes per element
//
// conclusion: use C function pointers, try to avoid growing because of fragmentation. but it's not too bad.
//
#include "array.h"
// static ustd::array<MQTTCmdFunction> mqtt_cmdfunctions_ = ustd::array<MQTTCmdFunction>(NUM_ENTRIES, 255, 16);
// static ustd::array<MQTTCmdFunction> mqtt_cmdfunctions_; // same as (16, 255, 16)
static ustd::array<MQTTCmdFunction> mqtt_cmdfunctions_ = ustd::array<MQTTCmdFunction>(100, 255, 16); // start 100 and grow

// CODE below

void register_mqtt_cmd(uint8_t device_type, uint8_t dummy1, const __FlashStringHelper * dummy2, const __FlashStringHelper * cmd, mqtt_cmdfunction_p f) {
    MQTTCmdFunction mf;
    mf.device_type_      = device_type;
    mf.dummy1_           = dummy1;
    mf.dummy2_           = dummy2;
    mf.cmd_              = cmd;
    mf.mqtt_cmdfunction_ = f; // 2 - with using std::function, 3 with normal C function pointer

    // mqtt_cmdfunctions_.emplace_back(device_type, dummy1, dummy2, cmd, f); // std::list and std::vector
    // mqtt_cmdfunctions_.emplace(device_type, dummy1, dummy2, cmd, f); // std::queue
    // mqtt_cmdfunctions_.emplace_back(device_type, dummy1, dummy2, cmd, f); // std::deque
    // mqtt_cmdfunctions_.push_back(mf); // Vector.h
    // mqtt_cmdfunctions_.push(mf); // ustd::queue
    mqtt_cmdfunctions_.add(mf); // ustd::array
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
    uint8_t  heap_frag = ESP.getHeapFragmentation();
    mem_used           = heap_start_ - free_heap;
    Serial.printf("(%10s) Free heap: %3d%% (%d) (~%d), frag:%d%% (~%d), used since boot: %d",
                  note,
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

    // for fixed arrays
    // for (uint8_t i = 0; i < NUM_ENTRIES; i++) {
    //     (mqtt_cmdfunctions_[i].mqtt_cmdfunction_)("hello", i);
    // }
    // Serial.println();

    // for containers
    for (const MQTTCmdFunction & mf : mqtt_cmdfunctions_) {
        (mf.mqtt_cmdfunction_)(uuid::read_flash_string(mf.cmd_).c_str(), mf.device_type_);
    }

    // for ustd::queue
    /*
    for (uint8_t i = 0; i < NUM_ENTRIES; i++) {
        auto mf = mqtt_cmdfunctions_.pop();
        (mf.mqtt_cmdfunction_)("iterator2", mf.device_type_);
    }
*/
    Serial.println();

    // size 2
    Serial.print("size of struct = ");
    Serial.print(sizeof(MQTTCmdFunction));
#ifndef STANDALONE
    Serial.print(", size per element = ");
    Serial.print(mem_used / NUM_ENTRIES);
#endif
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
