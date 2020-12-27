#include <Arduino.h>

#ifndef STANDALONE
static uint32_t heap_start_   = ESP.getFreeHeap();
static uint32_t old_free_heap = heap_start_;
#endif

static uint32_t mem_used = 0;

#include "command.h"

// clang-format off
#define MAKE_PSTR(string_name, string_literal) static const char __pstr__##string_name[] __attribute__((__aligned__(sizeof(uint32_t)))) PROGMEM = string_literal;
#define MAKE_PSTR_WORD(string_name) MAKE_PSTR(string_name, #string_name)
#define F_(string_name) FPSTR(__pstr__##string_name)
#define MAKE_PSTR_LIST(list_name, ...) static const __FlashStringHelper * const __pstr__##list_name[] PROGMEM = {__VA_ARGS__, nullptr};
#define FL_(list_name) (__pstr__##list_name)
// clang-format on

MAKE_PSTR(ten, "10")
MAKE_PSTR(off, "off")
MAKE_PSTR(flow, "flow")
MAKE_PSTR(bufferedflow, "buffered flow")
MAKE_PSTR(buffer, "buffered")
MAKE_PSTR(layeredbuffer, "layered buffered")

MAKE_PSTR_LIST(v1, F_(off))
MAKE_PSTR_LIST(v5, F_(off), F_(flow), F_(bufferedflow), F_(buffer), F_(layeredbuffer))
MAKE_PSTR_LIST(v8, F_(off), F_(flow), F_(bufferedflow), F_(buffer), F_(layeredbuffer), F_(bufferedflow), F_(buffer), F_(layeredbuffer))

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
// #include "containers.h"
// static emsesp::queue<MQTTCmdFunction> mqtt_cmdfunctions_ = emsesp::queue<MQTTCmdFunction>(NUM_ENTRIES);

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

// call back functions
void myFunction(const char * data, const int8_t id) {
    Serial.print(data);
    Serial.print(" ");
    Serial.print(id);
}

// abs of a signed 32-bit integer
uint32_t myabs(const int32_t i) {
    return (i < 0 ? -i : i);
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
                  myabs(free_heap - old_free_heap),
                  heap_frag,
                  myabs(heap_frag - old_heap_frag),
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

// local tests
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

    print_queue("test it", myQueue);
    print_queue("test it", myQueue);
    print_queue("test it", myQueue);

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
    Serial.println();
}

void setup() {
#ifndef STANDALONE
    Serial.begin(115200);
    Serial.println();
    Serial.printf("Starting heap: %d", heap_start_);
    Serial.println();
#endif
    show_mem("boot");

#ifndef STANDALONE
    uint32_t before_free_heap = ESP.getFreeHeap();
#endif

    emsesp::Command device(2);

    device.reserve(NUM_ENTRIES, 255, 10); // grow by 10, max size 255

    // fill container
    show_mem("before");

    for (uint8_t i = 1; i <= NUM_ENTRIES; i++) {
#if STRUCT_NUM == 3
        if (i < 20) {
            device.register_mqtt_cmd(i, 10, F("hi"), FL_(v5), F("tf3"), myFunction);
        } else if ((i > 20) && (i < 40)) {
            device.register_mqtt_cmd(i, 10, F("hi"), FL_(v1), F("tf3"), myFunction);
        } else {
            device.register_mqtt_cmd(i, 10, F("hi"), nullptr, F("tf3"), myFunction);
        }

#endif

#if STRUCT_NUM == 2
        // register_mqtt_cmd(i, 10, F("hi"), F("tf2"), myFunction);
        // register_mqtt_cmd(i, 10, F("hi"), F("tf2"), std::bind(&myFunction, std::placeholders::_1, std::placeholders::_2));
        // device.register_mqtt_cmd(i, 10, F("hi"), F("tf2"), [](const char * data, const int8_t id) { myFunction(data, id); });

        if (i < 20) {
            device.register_mqtt_cmd(i, 10, F("hi"), FL_(v5), F("tf3"), myFunction);
        } else if ((i > 20) && (i < 40)) {
            device.register_mqtt_cmd(i, 10, F("hi"), FL_(v1), F("tf3"), myFunction);
        } else {
            device.register_mqtt_cmd(i, 10, F("hi"), nullptr, F("tf3"), myFunction);
        }


#endif
    }

    Serial.println();
    show_mem("after");

#ifndef STANDALONE
    uint32_t after_free_heap = ESP.getFreeHeap();
    device.print(before_free_heap - after_free_heap);
#else
    device.print(0);
#endif

    queue_test();

    // device.show_device_values();

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
