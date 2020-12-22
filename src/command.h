#ifndef EMSESP_COMMAND_H
#define EMSESP_COMMAND_H

// 2 - uses std::function
// 3 - uses C void * function pointer
#define STRUCT_NUM 3

#define NUM_ENTRIES 200

#include <Arduino.h>

#include "containers.h"

#include <vector> // for flash_vectors
using flash_string_vector = std::vector<const __FlashStringHelper *>;

// for all the tests
#include <list>
#include <queue>
#include <deque>

namespace emsesp {

class Command {
  public:
    ~Command() = default;

    Command(uint8_t style)
        : style_(style){};

#if STRUCT_NUM == 2
#include <functional>
    using mqtt_cmdfunction_p = std::function<void(const char * data, const int8_t id)>;
    // no constructor, with std::function
    // size on ESP8266 - 28 bytes (ubuntu 56, osx 80)
    struct MQTTCmdFunction {
        uint8_t                     device_type_;      // 1 byte
        uint8_t                     dummy1_;           // 1 byte
        const __FlashStringHelper * dummy2_;           // 4
        flash_string_vector         options_;          //
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
        flash_string_vector         options_;          // variable
        const __FlashStringHelper * cmd_;              // 4
        mqtt_cmdfunction_p          mqtt_cmdfunction_; // 6
    };
#endif

    void register_mqtt_cmd(uint8_t                     device_type,
                           uint8_t                     dummy1,
                           const __FlashStringHelper * dummy2,
                           const flash_string_vector & options_,
                           const __FlashStringHelper * cmd,
                           mqtt_cmdfunction_p          f);

    void print(uint32_t mem_used);

    // note assignment must be static
    void reserve(uint8_t elements, uint8_t max, uint8_t grow) {
        static auto a = emsesp::array<MQTTCmdFunction>(elements, max, grow); // emsesp::array
        // static auto a = std::vector<MQTTCmdFunction>();                      // std::vector
        // static auto a = std::queue<MQTTCmdFunction>(elements);               // std::queue
        // static auto a = emsesp::queue<MQTTCmdFunction>(elements);            // emsesp::vector

        mqtt_cmdfunctions_ = &a;
    }

    //  emsesp::array<MQTTCmdFunction> mqtt_cmdfunctions()  {
    //     return *mqtt_cmdfunctions_;
    // }

    // const emsesp::queue<MQTTCmdFunction> mqtt_cmdfunctions() const {
    //     return mqtt_cmdfunctions_;
    // }

    // const emsesp::queue<MQTTCmdFunction> mqtt_cmdfunctions() const {
    //     return *mqtt_cmdfunctions_;
    // }


  private:
    uint8_t style_ = STRUCT_NUM;

    // 3: 200,255,16  5640, 28 bytes per element
    emsesp::array<MQTTCmdFunction> * mqtt_cmdfunctions_;

    // 3: empty, 7208, 36 bytes per element
    // std::vector<MQTTCmdFunction> * mqtt_cmdfunctions_;

    // 3: ?, ?, ? bytes per element
    // emsesp::queue<MQTTCmdFunction> mqtt_cmdfunctions_;
};

} // namespace emsesp

#endif
