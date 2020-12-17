#include <Arduino.h>
#include <functional>

#include <list>
#include <array>
#include <vector>
// #include <Vector.h>

// 1st implementation

using mqtt_cmdfunction_p = std::function<void(const char * data, const int8_t id)>;
using namespace std::placeholders;

struct MQTTCmdFunction {
    uint8_t                     device_type_;
    uint8_t                     dummy1;
    const __FlashStringHelper * cmd_;
    mqtt_cmdfunction_p          mqtt_cmdfunction_;

    MQTTCmdFunction(uint8_t device_type, const __FlashStringHelper * cmd, mqtt_cmdfunction_p mqtt_cmdfunction)
        : device_type_(device_type)
        , cmd_(cmd)
        , mqtt_cmdfunction_(mqtt_cmdfunction) {
    }
};

static std::vector<MQTTCmdFunction> mqtt_cmdfunctions_; // list of commands
// static Vector<MQTTCmdFunction> mqtt_cmdfunctions_; // list of commands
// static std::array<MQTTCmdFunction3, 210> mqtt_cmdfunctions_; // list of commands

void register_mqtt_cmd(uint8_t device_type, const __FlashStringHelper * cmd, mqtt_cmdfunction_p f) {
    mqtt_cmdfunctions_.emplace_back(device_type, cmd, f);
    // MQTTCmdFunction cmd_f(device_type, cmd, cb);
    // mqtt_cmdfunctions_.push_back(cmd_f);
}

// 2nd functions

typedef std::function<void(const char * data, const int8_t id)> StateUpdateCallback;
typedef struct StateUpdateHandlerInfo {
    uint8_t                     device_type_;
    const __FlashStringHelper * cmd_;
    StateUpdateCallback         _cb;

    StateUpdateHandlerInfo(uint8_t device_type, const __FlashStringHelper * cmd, StateUpdateCallback cb)
        : device_type_(device_type)
        , cmd_(cmd)
        , _cb(cb){};
} StateUpdateHandlerInfo_t;

// std::list
std::list<StateUpdateHandlerInfo_t> updateHandlers_;

void addUpdateHandler(uint8_t device_type, const __FlashStringHelper * cmd, StateUpdateCallback cb) {
    StateUpdateHandlerInfo_t updateHandler(device_type, cmd, cb);
    updateHandlers_.push_back(updateHandler);
}

static void myFunction1_static(const char * data, const int8_t id) {
    Serial.println("My function1");
}

void myFunction2(const char * data, const int8_t id) {
    Serial.println("My function2");
}

static uint32_t heap_start_;
void            show_mem(const char * note) {
#ifndef STANDALONE
    static uint32_t old_free_heap = 0;
    static uint8_t  old_heap_frag = 0;

    uint32_t free_heap = ESP.getFreeHeap();
    uint8_t  heap_frag = ESP.getHeapFragmentation();
    Serial.printf("(%s) Free heap: %d%% (%d) (~%d), frag:%d%% (~%d)",
                  note,
                  (100 * free_heap / heap_start_),
                  free_heap,
                  (uint32_t)abs(free_heap - old_free_heap),
                  heap_frag,
                  (uint8_t)abs(heap_frag - old_heap_frag));
    old_free_heap = free_heap;
    old_heap_frag = heap_frag;
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

    mqtt_cmdfunctions_.reserve(200);

    for (uint8_t i = 0; i < 200; i++) {
        // addUpdateHandler(0, F("testfunction"), [&](const char * data, const int8_t id) { myFunction2(data, id); });

        // register_mqtt_cmd(F("testfunction"), myFunction1);
        // register_mqtt_cmd(F("testfunction"), std::bind(&myFunction1, _1, _2));
        register_mqtt_cmd(i, F("testfunction"), [](const char * data, const int8_t id) { myFunction1_static(data, id); });
    }
    delay(1000);
    show_mem("after");
}

void loop() {
#ifndef STANDALONE
    // see if memory dissapears
    static uint32_t last_memcheck_ = 0;
    if (!last_memcheck_ || (millis() - last_memcheck_ > 2000)) {
        last_memcheck_ = millis();
        show_mem("loop");
    }
#endif
}
