#if defined(ESP32) && defined(ESPFC_NRF24)

#include "InputNrf24.h"

#include <SPI.h>

namespace Espfc {
namespace Device {

int InputNrf24::begin()
{
  SPI.begin(18, 19, 23, JanDroneNrf24::kCsnPin);

  if(!_radio.begin(&SPI))
  {
    return 0;
  }

  _radio.setAutoAck(true);
  _radio.setRetries(2, 5);
  _radio.setCRCLength(RF24_CRC_16);
  _radio.setPayloadSize(sizeof(JanDroneNrf24::ControlPacket));
  _radio.setAddressWidth(5);
  _radio.setChannel(JanDroneNrf24::kRadioChannel);
  _radio.setDataRate(RF24_250KBPS);
  _radio.setPALevel(RF24_PA_LOW);
  _radio.openReadingPipe(1, JanDroneNrf24::kPipeAddress);
  _radio.flush_rx();
  _radio.startListening();

  _lastFrameMs = millis();
  _haveFrame = false;
  return 1;
}

InputStatus InputNrf24::update()
{
  JanDroneNrf24::ControlPacket packet;
  bool received = false;

  while(_radio.available())
  {
    _radio.read(&packet, sizeof(packet));
    if(!JanDroneNrf24::isPacketValid(packet))
    {
      continue;
    }

    for(size_t i = 0; i < JanDroneNrf24::kChannelCount; ++i)
    {
      _channels[i] = packet.channels[i];
    }

    _lastSequence = packet.sequence;
    _lastFrameMs = millis();
    _haveFrame = true;
    received = true;

    if(packet.flags & JanDroneNrf24::FLAG_FAILSAFE)
    {
      return INPUT_FAILSAFE;
    }
  }

  if(received)
  {
    return INPUT_RECEIVED;
  }

  if(!_haveFrame)
  {
    return INPUT_IDLE;
  }

  const uint32_t ageMs = millis() - _lastFrameMs;
  if(ageMs >= JanDroneNrf24::kFailsafeTimeoutMs)
  {
    return INPUT_FAILSAFE;
  }
  if(ageMs >= JanDroneNrf24::kFrameTimeoutMs)
  {
    return INPUT_LOST;
  }

  return INPUT_IDLE;
}

uint16_t InputNrf24::get(uint8_t i) const
{
  return i < JanDroneNrf24::kChannelCount ? _channels[i] : 1500;
}

void InputNrf24::get(uint16_t* data, size_t len) const
{
  for(size_t i = 0; i < len; ++i)
  {
    data[i] = i < JanDroneNrf24::kChannelCount ? _channels[i] : 1500;
  }
}

size_t InputNrf24::getChannelCount() const
{
  return JanDroneNrf24::kChannelCount;
}

bool InputNrf24::needAverage() const
{
  return false;
}

} // namespace Device
} // namespace Espfc

#endif
