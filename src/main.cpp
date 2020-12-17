#include <Arduino.h>
#include <list>
#include <array>
#include <vector>
#include <functional>

// #include <Vector.h>

// 1st implementation

using mqtt_cmdfunction_p = std::function<void(const char * data, const int8_t id)>;
using namespace std::placeholders;

// static void add_command(uint8_t device_type, const __FlashStringHelper *cmd, mqtt_cmdfunction_p cb);
// void register_mqtt_cmd(const __FlashStringHelper *cmd, mqtt_cmdfunction_p f);

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

void add_command(uint8_t device_type, const __FlashStringHelper * cmd, mqtt_cmdfunction_p cb) {
    // mqtt_cmdfunctions_.emplace_back(device_type, cmd, cb);

    MQTTCmdFunction cmd_f(device_type, cmd, cb);
    mqtt_cmdfunctions_.push_back(cmd_f);
}

void register_mqtt_cmd(const __FlashStringHelper * cmd, mqtt_cmdfunction_p f) {
    add_command(0, cmd, f);
}

// 2nd implementation

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
    Serial.printf("My function1");
}

void myFunction2(const char * data, const int8_t id) {
    Serial.printf("My function2");
}

static uint32_t heap_start_;
void            show_mem(const char * s) {
    uint32_t free_memory = ESP.getFreeHeap();
    uint8_t  per         = (100 * free_memory / heap_start_);
    uint8_t  frag        = ESP.getHeapFragmentation();
    Serial.printf("(%s) used=%d, Free heap: %d%% (%d), frag:%u%%\n", s, (heap_start_ - free_memory), per, free_memory, frag);
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    heap_start_ = ESP.getFreeHeap();
    show_mem("start");

    mqtt_cmdfunctions_.reserve(200);

    for (uint8_t i = 0; i < 200; i++) {
        // addUpdateHandler(0, F("testfunction"), [&](const char * data, const int8_t id) { myFunction2(data, id); });

        // register_mqtt_cmd(F("testfunction"), myFunction1);
        // register_mqtt_cmd(F("testfunction"), std::bind(&myFunction1, _1, _2));
        register_mqtt_cmd(F("testfunction"), [](const char * data, const int8_t id) { myFunction1_static(data, id); });
    }
}

void loop() {
    static uint32_t last_memcheck_ = 0;
    if (!last_memcheck_ || (millis() - last_memcheck_ > 2000)) {
        last_memcheck_ = millis();
        show_mem("loop");
    }
}
