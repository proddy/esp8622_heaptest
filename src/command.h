#ifndef EMSESP_COMMAND_H
#define EMSESP_COMMAND_H

// 2 - uses std::function
// 3 - uses C void * function pointer
#define STRUCT_NUM 3

#include <Arduino.h>

#include "containers.h"

#include <vector> // for flash_vectors
using flash_string_vector = std::vector<const __FlashStringHelper *>;

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

    void reserve(uint8_t elements, uint8_t max, uint8_t grow);

    const emsesp::array<MQTTCmdFunction> * get_container() const {
        return mqtt_cmdfunctions_;
    }


  private:
    uint8_t style_ = STRUCT_NUM;

    // 3: 200,255,16 , 3208, 16 bytes per element
    emsesp::array<MQTTCmdFunction> * mqtt_cmdfunctions_;
};

} // namespace emsesp

#endif
