#include <Bluepad32.h>
#include <RF24.h>
#include <SPI.h>

#include "../../include/JanDroneNrf24Protocol.h"

ControllerPtr g_controller;
RF24 g_radio(JanDroneNrf24::kCePin, JanDroneNrf24::kCsnPin);
JanDroneNrf24::ControlPacket g_packet;

static constexpr uint32_t kSendIntervalMs = 10;
static constexpr int32_t kAxisDeadband = 24;
static constexpr int32_t kAxisMin = -511;
static constexpr int32_t kAxisMax = 512;

uint16_t clampPwm(long value)
{
  if(value < 1000) return 1000;
  if(value > 2000) return 2000;
  return static_cast<uint16_t>(value);
}

uint16_t axisToPwm(int32_t value, bool invert = false)
{
  if(invert) value = -value;
  if(value > -kAxisDeadband && value < kAxisDeadband) value = 0;
  return clampPwm(map(value, kAxisMin, kAxisMax, 1000, 2000));
}

uint16_t throttleToPwm(int32_t value)
{
  return axisToPwm(value, true);
}

void fillFailsafePacket()
{
  g_packet.flags = JanDroneNrf24::FLAG_FAILSAFE;
  g_packet.channels[JanDroneNrf24::CHANNEL_ROLL] = 1500;
  g_packet.channels[JanDroneNrf24::CHANNEL_PITCH] = 1500;
  g_packet.channels[JanDroneNrf24::CHANNEL_THROTTLE] = 1000;
  g_packet.channels[JanDroneNrf24::CHANNEL_YAW] = 1500;
  g_packet.channels[JanDroneNrf24::CHANNEL_ARM] = 1000;
  g_packet.channels[JanDroneNrf24::CHANNEL_MODE] = 1000;
  g_packet.channels[JanDroneNrf24::CHANNEL_BUZZER] = 1000;
}

void fillFromController(ControllerPtr ctl)
{
  g_packet.flags = 0;
  g_packet.channels[JanDroneNrf24::CHANNEL_ROLL] = axisToPwm(ctl->axisRX());
  g_packet.channels[JanDroneNrf24::CHANNEL_PITCH] = axisToPwm(ctl->axisRY(), true);
  g_packet.channels[JanDroneNrf24::CHANNEL_THROTTLE] = throttleToPwm(ctl->axisY());
  g_packet.channels[JanDroneNrf24::CHANNEL_YAW] = axisToPwm(ctl->axisX());

  const bool arm = ctl->r1();
  const bool angle = ctl->l1();
  const bool buzzer = ctl->b();

  if(arm) g_packet.flags |= JanDroneNrf24::FLAG_ARM;
  if(angle) g_packet.flags |= JanDroneNrf24::FLAG_ANGLE;
  if(buzzer) g_packet.flags |= JanDroneNrf24::FLAG_BUZZER;

  g_packet.channels[JanDroneNrf24::CHANNEL_ARM] = JanDroneNrf24::pwmFromSwitch(arm);
  g_packet.channels[JanDroneNrf24::CHANNEL_MODE] = JanDroneNrf24::pwmFromSwitch(angle);
  g_packet.channels[JanDroneNrf24::CHANNEL_BUZZER] = JanDroneNrf24::pwmFromSwitch(buzzer);
}

void onConnectedController(ControllerPtr ctl)
{
  if(g_controller) return;
  g_controller = ctl;
}

void onDisconnectedController(ControllerPtr ctl)
{
  if(g_controller == ctl)
  {
    g_controller = nullptr;
  }
}

void setup()
{
  Serial.begin(115200);

  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.enableVirtualDevice(false);

  SPI.begin(18, 19, 23, JanDroneNrf24::kCsnPin);

  if(!g_radio.begin(&SPI))
  {
    Serial.println("nRF24 init failed");
    while(true) delay(1000);
  }

  g_radio.setAutoAck(true);
  g_radio.setRetries(2, 5);
  g_radio.setCRCLength(RF24_CRC_16);
  g_radio.setPayloadSize(sizeof(JanDroneNrf24::ControlPacket));
  g_radio.setAddressWidth(5);
  g_radio.setChannel(JanDroneNrf24::kRadioChannel);
  g_radio.setDataRate(RF24_250KBPS);
  g_radio.setPALevel(RF24_PA_LOW);
  g_radio.openWritingPipe(JanDroneNrf24::kPipeAddress);
  g_radio.stopListening();

  fillFailsafePacket();
  JanDroneNrf24::finalizePacket(g_packet);
}

void loop()
{
  static uint32_t lastSendMs = 0;

  BP32.update();

  if(millis() - lastSendMs < kSendIntervalMs)
  {
    return;
  }
  lastSendMs = millis();

  g_packet.sequence++;
  g_packet.txMillis = millis();

  if(g_controller && g_controller->isConnected())
  {
    fillFromController(g_controller);
  }
  else
  {
    fillFailsafePacket();
  }

  JanDroneNrf24::finalizePacket(g_packet);
  g_radio.write(&g_packet, sizeof(g_packet));
}
