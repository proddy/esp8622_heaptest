#include <Arduino.h>

// #include "functional.h"

#include <functional>
using mqtt_cmdfunction_p = std::function<void(const char * data, const int8_t id)>;

typedef void (*mqtt_cmdfunction_p2)(const char * data, const int8_t id);

// with constructor
struct MQTTCmdFunction1 {
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

// no constructor, with std::function
// size on ESP8266 - 28 bytes (ubuntu 56, osx 80)
struct MQTTCmdFunction2 {
    uint8_t                     device_type_;      // 1 byte
    uint8_t                     dummy1_;           // 1 byte
    const __FlashStringHelper * dummy2_;           // 4
    const __FlashStringHelper * cmd_;              // 4
    mqtt_cmdfunction_p          mqtt_cmdfunction_; // 14
};

// no constructor, no std::function but function ptr
// size on ESP8266 - 16 bytes (ubuntu 32, osx 32)
struct MQTTCmdFunction3 {
    uint8_t                     device_type_;      // 1 byte
    uint8_t                     dummy1_;           // 1 byte
    const __FlashStringHelper * dummy2_;           // 4
    const __FlashStringHelper * cmd_;              // 4
    mqtt_cmdfunction_p2         mqtt_cmdfunction_; // 6
};

#define STRUCT MQTTCmdFunction2

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

// ustd array.h - dynamic grow - 7536
// #include "array.h"
// static ustd::array<STRUCT> mqtt_cmdfunctions_;

// ustd array.h - fixed size - 2160
#include "array.h"
static ustd::array<STRUCT> mqtt_cmdfunctions_ = ustd::array<STRUCT>(200, 200, 0);

// ustd queue.h - not dynamic, only fixed size - 2160 (same as array)
// or 560 if not using bind, but direct functions
// #include "queue.h"
// static ustd::queue<STRUCT> mqtt_cmdfunctions_ = ustd::queue<STRUCT>(200);

void register_mqtt_cmd(uint8_t device_type, uint8_t dummy1, const __FlashStringHelper * dummy2, const __FlashStringHelper * cmd, mqtt_cmdfunction_p f) {
    // mqtt_cmdfunctions_.emplace_back(device_type, dummy1, dummy2, cmd, f); // std::list and std::vector

    // mqtt_cmdfunctions_.emplace(device_type, dummy1, dummy2, cmd, f); // std::queue

    // mqtt_cmdfunctions_.emplace_back(device_type, dummy1, dummy2, cmd, f); // std::deque

    // auto mf = MQTTCmdFunction(device_type, dummy1, dummy2, cmd, f); // Vector.h
    // mqtt_cmdfunctions_.push_back(mf); // Vector.h

    STRUCT mf;
    mf.device_type_      = device_type;
    mf.dummy1_           = dummy1;
    mf.dummy2_           = dummy2;
    mf.cmd_              = cmd;
    mf.mqtt_cmdfunction_ = f;

    // mqtt_cmdfunctions_.push(mf); // ustd::queue

    mqtt_cmdfunctions_.add(mf); // ustd::array
}

void myFunction(const char * data, const uint8_t id) {
    Serial.print(data);
    Serial.print(" ");
    Serial.print(id);
    Serial.print(",");
}

static uint32_t heap_start_;
void            show_mem(const char * note) {
#ifndef STANDALONE
    static uint32_t old_free_heap = 0;
    static uint8_t  old_heap_frag = 0;
    uint32_t        free_heap     = ESP.getFreeHeap();
    uint8_t         heap_frag     = ESP.getHeapFragmentation();
    Serial.printf("(%10s) Free heap: %3d%% (%d) (~%d), frag:%d%% (~%d)",
                  note,
                  (100 * free_heap / heap_start_),
                  free_heap,
                  (uint32_t)abs(free_heap - old_free_heap),
                  heap_frag,
                  (uint8_t)abs(heap_frag - old_heap_frag));
    old_free_heap = free_heap;
    old_heap_frag = heap_frag;
    Serial.println();
#else
    Serial.println(note);
#endif
}

void setup() {
#ifndef STANDALONE
    Serial.begin(115200);
    Serial.println();
    heap_start_ = ESP.getFreeHeap();
#endif
    show_mem("before");
    delay(1000);

    // reserve
    // mqtt_cmdfunctions_.reserve(200);

    // fill container
    // with ustd::queue, bind uses 3760, lambda 2160 and direct 560 bytes
    for (uint8_t i = 0; i < 200; i++) {
        register_mqtt_cmd(i, 10, F("hi"), F("testfunction"), myFunction);

        // register_mqtt_cmd(i, 10, F("hi"), F("testfunction"), std::bind(&myFunction, std::placeholders::_1, std::placeholders::_2));

        // register_mqtt_cmd(i, 10, F("hi"), F("testfunction"), [](const char * data, const uint8_t id) { myFunction(data, id); });
    }

    delay(1000);
    show_mem("after");

    // for arrays
    for (uint8_t i = 0; i < 200; i++) {
        (mqtt_cmdfunctions_[i].mqtt_cmdfunction_)("hello", i++);
    }

    // for containers
    // uint8_t i = 0;
    // for (auto & mf : mqtt_cmdfunctions_) {
    //     (mf.mqtt_cmdfunction_)("hello", i++);
    // }

    // for ustd::queue
    // for (uint8_t i = 0; i < 200; i++) {
    //     auto mf = mqtt_cmdfunctions_.pop();
    //     (mf.mqtt_cmdfunction_)("hello", i++);
    // }

    Serial.println();

    // size 2
    Serial.print("size of struct = ");
    Serial.print(sizeof(STRUCT));
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
