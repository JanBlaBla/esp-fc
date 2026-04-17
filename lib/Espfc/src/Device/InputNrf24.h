#pragma once

#if defined(ESP32) && defined(ESPFC_NRF24)

#include <RF24.h>
#include "Device/InputDevice.h"
#include "../../../../include/JanDroneNrf24Protocol.h"

namespace Espfc {
namespace Device {

class InputNrf24: public InputDevice
{
public:
  int begin();
  InputStatus update() override;
  uint16_t get(uint8_t i) const override;
  void get(uint16_t* data, size_t len) const override;
  size_t getChannelCount() const override;
  bool needAverage() const override;

private:
  RF24 _radio{JanDroneNrf24::kCePin, JanDroneNrf24::kCsnPin};
  uint16_t _channels[JanDroneNrf24::kChannelCount] = {1500, 1500, 1000, 1500, 1000, 1000, 1000};
  uint16_t _lastSequence = 0;
  uint32_t _lastFrameMs = 0;
  bool _haveFrame = false;
};

} // namespace Device
} // namespace Espfc

#endif
