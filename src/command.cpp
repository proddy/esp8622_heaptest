#include "command.h"

namespace uuid {
std::string read_flash_string(const __FlashStringHelper * flash_str) {
    std::string str(::strlen_P(reinterpret_cast<PGM_P>(flash_str)), '\0');
    ::strncpy_P(&str[0], reinterpret_cast<PGM_P>(flash_str), str.capacity() + 1);
    return str;
}
} // namespace uuid

namespace emsesp {

static emsesp::queue<Command::MQTTCmdFunction> mqtt_cmdfunctions_ = emsesp::queue<Command::MQTTCmdFunction>(200);

void Command::register_mqtt_cmd(uint8_t                             device_type,
                                uint8_t                             dummy1,
                                const __FlashStringHelper *         dummy2,
                                const __FlashStringHelper * const * options,
                                const __FlashStringHelper *         cmd,
                                mqtt_cmdfunction_p                  f) {
    MQTTCmdFunction mf;
    mf.device_type_      = device_type;
    mf.dummy1_           = dummy1;
    mf.dummy2_           = dummy2;
    mf.cmd_              = cmd;
    mf.mqtt_cmdfunction_ = f; // 2 - with using std::function or 3 with normal C function pointer

    mf.options_      = options;
    mf.options_size_ = 0;
    if (options != nullptr) {
        // count #options
        uint8_t i = 0;
        while (options[i++]) {
            mf.options_size_++;
        };
        Serial.print("Got options size:");
        Serial.print(mf.options_size_);
        Serial.print(" ");
    }

    // emplaces's
    // mqtt_cmdfunctions_.emplace_back(device_type, dummy1, dummy2, cmd, f); // std::list and std::vector
    // mqtt_cmdfunctions_.emplace(device_type, dummy1, dummy2, cmd, f); // std::queue
    // mqtt_cmdfunctions_.emplace_back(device_type, dummy1, dummy2, cmd, f); // std::deque

    // mqtt_cmdfunctions_.push_back(mf); // std::vector std::list
    // mqtt_cmdfunctions_.push_front(mf); // std::deque

    // mqtt_cmdfunctions().push(mf); // emsesp::queue, emsesp::array, std::queue

    mqtt_cmdfunctions_->push(mf); // emsesp::array

    // mqtt_cmdfunctions_.push(mf); // emsesp::queue

    // mqtt_cmdfunctions_.push_back(mf); // std::queue
}

// print stuff
void Command::print(uint32_t mem_used) {
    Serial.println();

    // uint8_t size_elements = mqtt_cmdfunctions_.size(); // std::vector
    uint8_t size_elements = mqtt_cmdfunctions_->size(); // emsesp::array

    if (size_elements == 0) {
        return;
    }

    // for (const MQTTCmdFunction & mf : mqtt_cmdfunctions()) { // using iterator
    for (uint8_t i = 0; i < size_elements; i++) {
        // auto mf = (mqtt_cmdfunctions_)[i]; // emsesp::queue std::queue
        auto mf = (*mqtt_cmdfunctions_)[i]; // emsesp::array
        Serial.print("(");

        (mf.mqtt_cmdfunction_)(uuid::read_flash_string(mf.cmd_).c_str(), mf.device_type_);

        if (mf.options_) {
            // see if we have options
            for (uint8_t j = 0; j < mf.options_size_; j++) {
                Serial.print("[");
                Serial.print(uuid::read_flash_string(mf.options_[j]).c_str());
                Serial.print("]");
            }
        }

        Serial.print(") ");
    }

    Serial.println();
    Serial.println();
    Serial.print("number of elements = ");
    Serial.print(size_elements);
    Serial.print(", total mem used = ");
    Serial.print(mem_used);
    Serial.print(", size of struct = ");
    Serial.print(sizeof(MQTTCmdFunction));
#ifndef STANDALONE
    Serial.print(", size per element = ");
    Serial.print(mem_used / size_elements);
#endif
    Serial.println();
    Serial.println();
}

void Command::show_device_values() {
    uint8_t total_s = 0;
    uint8_t count   = 0;
    char    ss[100];
    for (const auto & dv : *(mqtt_cmdfunctions_)) {
        uint8_t s = sizeof(dv);
        snprintf_P(ss, 100, PSTR("[%s] %d"), uuid::read_flash_string(dv.cmd_).c_str(), s);
        Serial.println(ss);
        total_s += s;
        count++;
    }
    snprintf_P(ss, 100, "Total size of %d elements: %d", count, total_s);
    Serial.println(ss);
}


} // namespace emsesp
