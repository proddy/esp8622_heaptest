#include "command.h"
#include "containers.h"

namespace uuid {
std::string read_flash_string(const __FlashStringHelper * flash_str) {
    std::string str(::strlen_P(reinterpret_cast<PGM_P>(flash_str)), '\0');
    ::strncpy_P(&str[0], reinterpret_cast<PGM_P>(flash_str), str.capacity() + 1);
    return str;
}
} // namespace uuid

namespace emsesp {

void Command::register_mqtt_cmd(uint8_t                     device_type,
                                uint8_t                     dummy1,
                                const __FlashStringHelper * dummy2,
                                const flash_string_vector & options,
                                const __FlashStringHelper * cmd,
                                mqtt_cmdfunction_p          f) {
    MQTTCmdFunction mf;
    mf.device_type_      = device_type;
    mf.dummy1_           = dummy1;
    mf.dummy2_           = dummy2;
    mf.options_          = options;
    mf.cmd_              = cmd;
    mf.mqtt_cmdfunction_ = f; // 2 - with using std::function or 3 with normal C function pointer

    // emplaces's
    // mqtt_cmdfunctions_.emplace_back(device_type, dummy1, dummy2, cmd, f); // std::list and std::vector
    // mqtt_cmdfunctions_.emplace(device_type, dummy1, dummy2, cmd, f); // std::queue
    // mqtt_cmdfunctions_.emplace_back(device_type, dummy1, dummy2, cmd, f); // std::deque

    // mqtt_cmdfunctions_.push_back(mf); // std::vector std::list
    // mqtt_cmdfunctions_.push_front(mf); // std::deque

    // mqtt_cmdfunctions().push(mf); // emsesp::queue, emsesp::array, std::queue

    mqtt_cmdfunctions_->push(mf);

    // mqtt_cmdfunctions_->push_back(mf); // std::queue
}

// print stuff
void Command::print(uint32_t mem_used) {
    Serial.println();

    uint8_t size_elements = mqtt_cmdfunctions_->size();

    for (uint8_t i = 0; i < size_elements; i++) {
        auto mf = (*mqtt_cmdfunctions_)[i];
        // for (const MQTTCmdFunction & mf : mqtt_cmdfunctions()) { // using iterator
        (mf.mqtt_cmdfunction_)(uuid::read_flash_string(mf.cmd_).c_str(), mf.device_type_);
        // see if we have options
        if (mf.options_.size() != 0) {
            for (uint8_t i = 0; i < mf.options_.size(); i++) {
                auto a = mf.options_;
                Serial.print("[");
                Serial.print(uuid::read_flash_string(a[i]).c_str());
                Serial.print("]");
            }
        }
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


} // namespace emsesp
