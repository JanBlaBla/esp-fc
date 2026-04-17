#pragma once

#include <stddef.h>
#include <stdint.h>

namespace JanDroneNrf24 {

static constexpr uint8_t kMagic = 0x4a;
static constexpr uint8_t kVersion = 1;
static constexpr size_t kChannelCount = 7;
static constexpr uint16_t kFrameTimeoutMs = 120;
static constexpr uint16_t kFailsafeTimeoutMs = 350;
static constexpr uint8_t kRadioChannel = 108;
static constexpr uint8_t kCsnPin = 16;
static constexpr uint8_t kCePin = 17;
static constexpr uint64_t kPipeAddress = 0xD14F43524CULL; // "D1FCRL"

enum Flag : uint8_t {
  FLAG_ARM = 1 << 0,
  FLAG_ANGLE = 1 << 1,
  FLAG_BUZZER = 1 << 2,
  FLAG_FAILSAFE = 1 << 3,
};

enum ChannelIndex : size_t {
  CHANNEL_ROLL = 0,
  CHANNEL_PITCH = 1,
  CHANNEL_THROTTLE = 2,
  CHANNEL_YAW = 3,
  CHANNEL_ARM = 4,
  CHANNEL_MODE = 5,
  CHANNEL_BUZZER = 6,
};

#pragma pack(push, 1)
struct ControlPacket {
  uint8_t magic = kMagic;
  uint8_t version = kVersion;
  uint8_t flags = 0;
  uint8_t reserved = 0;
  uint16_t sequence = 0;
  uint32_t txMillis = 0;
  uint16_t channels[kChannelCount] = {1500, 1500, 1000, 1500, 1000, 1000, 1000};
  uint16_t crc = 0;
};
#pragma pack(pop)

static_assert(sizeof(ControlPacket) <= 32, "nRF24 payload must stay within 32 bytes");

inline uint16_t crc16Ccitt(const uint8_t* data, size_t len)
{
  uint16_t crc = 0xffff;
  for(size_t i = 0; i < len; ++i)
  {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for(uint8_t bit = 0; bit < 8; ++bit)
    {
      if(crc & 0x8000) crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
      else crc <<= 1;
    }
  }
  return crc;
}

inline void finalizePacket(ControlPacket& packet)
{
  packet.crc = 0;
  packet.crc = crc16Ccitt(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
}

inline bool isPacketValid(const ControlPacket& packet)
{
  if(packet.magic != kMagic || packet.version != kVersion) return false;
  ControlPacket copy = packet;
  const uint16_t expected = copy.crc;
  copy.crc = 0;
  return expected == crc16Ccitt(reinterpret_cast<const uint8_t*>(&copy), sizeof(copy));
}

inline uint16_t pwmFromSwitch(bool active)
{
  return active ? 2000 : 1000;
}

} // namespace JanDroneNrf24
