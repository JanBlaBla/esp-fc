#include <Bluepad32.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

ControllerPtr g_controller;

static constexpr uint32_t kSendIntervalMs = 10;
static constexpr uint32_t kScanIntervalMs = 200;
static constexpr int32_t kAxisDeadband = 24;
static constexpr int32_t kAxisMin = -511;
static constexpr int32_t kAxisMax = 512;
static constexpr uint8_t kWifiChannelMin = 1;
static constexpr uint8_t kWifiChannelMax = 13;
static constexpr uint8_t kRcChannelMin = 0;
static constexpr uint8_t kRcChannelMax = 7;
static constexpr uint16_t kPwmInputMin = 880;
static constexpr uint16_t kPwmInputCenter = 1500;
static constexpr uint16_t kPwmInputMax = 2120;
static constexpr uint16_t kPwmSwitchLow = 1000;
static constexpr uint16_t kPwmSwitchHigh = 2000;
static constexpr uint16_t kPwmAxisMin = 1000;
static constexpr uint16_t kPwmAxisCenter = 1500;
static constexpr uint16_t kPwmAxisMax = 2000;
static constexpr uint8_t kBroadcastMac[ESP_NOW_ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

enum MessageType: uint8_t {
  RC_DATA = 0x01,
  FC_ALIVE = 0x10,
  FC_DATA = 0x11,
  PAIR_REQ = 0xfe,
  PAIR_RES = 0xff,
};

struct MessageRc
{
  MessageType type = RC_DATA;
  int16_t ch1 = kPwmInputCenter;
  int16_t ch2 = kPwmInputCenter;
  int16_t ch3 = 1000;
  int16_t ch4 = kPwmInputCenter;
  int8_t ch5 = 0;
  int8_t ch6 = 0;
  int8_t ch7 = 0;
  int8_t ch8 = 0;
  uint8_t csum = 0;

  static int8_t encodeAux(int x)
  {
    x = constrain(x, static_cast<int>(kPwmInputMin), static_cast<int>(kPwmInputMax)) - kPwmInputCenter;
    const int round = x > 0 ? 2 : -2;
    return static_cast<int8_t>((x + round) / 5);
  }

  static uint16_t decodeAux(int8_t x)
  {
    return constrain(kPwmInputCenter + (x * 5), kPwmInputMin, kPwmInputMax);
  }
} __attribute__((packed));

struct MessagePairRequest
{
  MessageType type = PAIR_REQ;
  uint8_t channel = 1;
  uint8_t csum = 0;
} __attribute__((packed));

struct MessagePairResponse
{
  MessageType type = PAIR_RES;
  uint8_t csum = 0;
} __attribute__((packed));

enum LinkState: uint8_t {
  DISCOVERING,
  TRANSMITTING,
};

MessageRc g_packet;
LinkState g_linkState = DISCOVERING;
uint8_t g_peer[ESP_NOW_ETH_ALEN] = {0, 0, 0, 0, 0, 0};
uint8_t g_scanChannel = kWifiChannelMin;
uint32_t g_nextScanMs = 0;
bool g_peerAdded = false;
uint32_t g_nextDebugMs = 0;
bool g_lastControllerConnected = false;
bool g_lastPeerReady = false;
bool g_armLatched = false;
bool g_angleLatched = false;
bool g_lastArmButton = false;
bool g_lastAngleButton = false;

void printMac(const uint8_t *mac)
{
  if(!mac) return;
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void printPacketSummary(const char *label)
{
  Serial.printf(
    "%s ch=[%d,%d,%d,%d,%d,%d,%d,%d] state=%s controller=%s peer=",
    label,
    g_packet.ch1,
    g_packet.ch2,
    g_packet.ch3,
    g_packet.ch4,
    MessageRc::decodeAux(g_packet.ch5),
    MessageRc::decodeAux(g_packet.ch6),
    MessageRc::decodeAux(g_packet.ch7),
    MessageRc::decodeAux(g_packet.ch8),
    g_linkState == TRANSMITTING ? "tx" : "scan",
    (g_controller && g_controller->isConnected()) ? "connected" : "disconnected"
  );
  if(g_peerAdded) printMac(g_peer);
  else Serial.print("none");
  Serial.println();
}

void handleRxData(const uint8_t *mac, const uint8_t *data, int len)
{
  if(!mac || !data || len < 2) return;
  if(checksum(data, len - 1) != data[len - 1]) return;

  if(data[0] == PAIR_REQ && len >= static_cast<int>(sizeof(MessagePairRequest)))
  {
    const auto *pairRequest = reinterpret_cast<const MessagePairRequest*>(data);
    const uint8_t channel = constrain(pairRequest->channel, kWifiChannelMin, kWifiChannelMax);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    if(addPeer(mac))
    {
      MessagePairResponse pairResponse;
      sendMessage(g_peer, pairResponse);
      g_linkState = TRANSMITTING;
      Serial.print("ESP-NOW paired with ");
      printMac(g_peer);
      Serial.println();
    }
  }
}

uint8_t checksum(const uint8_t *data, size_t len)
{
  uint8_t csum = 0x55;
  for(size_t i = 0; i < len; ++i)
  {
    csum ^= data[i];
  }
  return csum;
}

template<typename M>
uint8_t checksum(const M& message)
{
  return checksum(reinterpret_cast<const uint8_t*>(&message), sizeof(M) - 1);
}

bool addPeer(const uint8_t* peer)
{
  if(g_peerAdded)
  {
    esp_now_del_peer(g_peer);
    g_peerAdded = false;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peer, ESP_NOW_ETH_ALEN);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if(esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    return false;
  }

  memcpy(g_peer, peer, ESP_NOW_ETH_ALEN);
  g_peerAdded = true;
  return true;
}

template<typename M>
bool sendMessage(const uint8_t* peer, M& message)
{
  message.csum = checksum(message);
  return esp_now_send(peer, reinterpret_cast<const uint8_t*>(&message), sizeof(M)) == ESP_OK;
}

uint16_t clampPwm(long value)
{
  if(value < kPwmInputMin) return kPwmInputMin;
  if(value > kPwmInputMax) return kPwmInputMax;
  return static_cast<uint16_t>(value);
}

uint16_t axisToPwm(int32_t value, bool invert = false)
{
  if(invert) value = -value;
  if(value > -kAxisDeadband && value < kAxisDeadband) value = 0;
  return clampPwm(map(value, kAxisMin, kAxisMax, kPwmAxisMin, kPwmAxisMax));
}

uint16_t throttleToPwm(int32_t value)
{
  value = -value;
  if(value < kAxisDeadband) value = 0;
  return clampPwm(map(value, 0, kAxisMax, kPwmAxisMin, kPwmAxisMax));
}

void fillSafeChannels()
{
  g_packet.ch1 = kPwmAxisCenter;
  g_packet.ch2 = kPwmAxisCenter;
  g_packet.ch3 = kPwmSwitchLow;
  g_packet.ch4 = kPwmAxisCenter;
  g_packet.ch5 = MessageRc::encodeAux(kPwmSwitchLow);
  g_packet.ch6 = MessageRc::encodeAux(kPwmSwitchLow);
  g_packet.ch7 = MessageRc::encodeAux(kPwmSwitchLow);
  g_packet.ch8 = MessageRc::encodeAux(kPwmSwitchLow);
}

void fillFromController(ControllerPtr ctl)
{
  const bool armButton = ctl->r1();
  const bool angleButton = ctl->l1();
  const bool buzzer = ctl->b();

  if(armButton && !g_lastArmButton)
  {
    g_armLatched = !g_armLatched;
    Serial.printf("Arm latch: %s\n", g_armLatched ? "on" : "off");
  }

  if(angleButton && !g_lastAngleButton)
  {
    g_angleLatched = !g_angleLatched;
    Serial.printf("Angle latch: %s\n", g_angleLatched ? "on" : "off");
  }

  g_lastArmButton = armButton;
  g_lastAngleButton = angleButton;

  g_packet.ch1 = axisToPwm(ctl->axisRX());
  g_packet.ch2 = axisToPwm(ctl->axisRY(), true);
  g_packet.ch3 = throttleToPwm(ctl->axisY());
  g_packet.ch4 = axisToPwm(ctl->axisX());
  g_packet.ch5 = MessageRc::encodeAux(g_armLatched ? kPwmSwitchHigh : kPwmSwitchLow);
  g_packet.ch6 = MessageRc::encodeAux(g_angleLatched ? kPwmSwitchHigh : kPwmSwitchLow);
  g_packet.ch7 = MessageRc::encodeAux(buzzer ? kPwmSwitchHigh : kPwmSwitchLow);
  g_packet.ch8 = MessageRc::encodeAux(kPwmSwitchLow);
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
  if(!info) return;
  handleRxData(info->src_addr, data, len);
}
#else
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len)
{
  handleRxData(mac, data, len);
}
#endif

void updateDiscovery()
{
  const uint32_t now = millis();
  if(now < g_nextScanMs) return;

  g_scanChannel++;
  if(g_scanChannel > kWifiChannelMax) g_scanChannel = kWifiChannelMin;
  esp_wifi_set_channel(g_scanChannel, WIFI_SECOND_CHAN_NONE);
  g_nextScanMs = now + kScanIntervalMs;
}

void onConnectedController(ControllerPtr ctl)
{
  if(g_controller) return;
  g_controller = ctl;
  Serial.print("Controller connected");
  if(ctl)
  {
    const ControllerProperties properties = ctl->getProperties();
    Serial.printf(": model=%s vid=0x%04x pid=0x%04x\n", ctl->getModelName().c_str(), properties.vendor_id, properties.product_id);
  }
  else
  {
    Serial.println();
  }
}

void onDisconnectedController(ControllerPtr ctl)
{
  if(g_controller == ctl)
  {
    g_controller = nullptr;
    g_armLatched = false;
    g_angleLatched = false;
    g_lastArmButton = false;
    g_lastAngleButton = false;
    Serial.println("Controller disconnected");
  }
}

void setup()
{
  Serial.begin(115200);

  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.enableVirtualDevice(false);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if(esp_now_init() != ESP_OK)
  {
    Serial.println("ESP-NOW init failed");
    while(true) delay(1000);
  }

  if(esp_now_register_recv_cb(onDataRecv) != ESP_OK)
  {
    Serial.println("ESP-NOW RX callback failed");
    while(true) delay(1000);
  }

  fillSafeChannels();
  esp_wifi_set_channel(g_scanChannel, WIFI_SECOND_CHAN_NONE);
  Serial.printf("ESP-NOW scanning started on channel %u\n", g_scanChannel);
}

void loop()
{
  static uint32_t lastSendMs = 0;

  BP32.update();

  if(g_linkState == DISCOVERING)
  {
    updateDiscovery();
  }

  const bool controllerConnected = g_controller && g_controller->isConnected();
  if(controllerConnected != g_lastControllerConnected)
  {
    Serial.printf("Controller state changed: %s\n", controllerConnected ? "connected" : "disconnected");
    g_lastControllerConnected = controllerConnected;
  }

  if(g_peerAdded != g_lastPeerReady)
  {
    Serial.printf("Peer state changed: %s\n", g_peerAdded ? "paired" : "not paired");
    g_lastPeerReady = g_peerAdded;
  }

  if(millis() - lastSendMs < kSendIntervalMs)
  {
    if(millis() >= g_nextDebugMs)
    {
      printPacketSummary("debug");
      g_nextDebugMs = millis() + 1000;
    }
    return;
  }
  lastSendMs = millis();

  if(g_controller && g_controller->isConnected())
  {
    fillFromController(g_controller);
  }
  else
  {
    g_lastArmButton = false;
    g_lastAngleButton = false;
    fillSafeChannels();
  }

  if(g_linkState == TRANSMITTING && g_peerAdded)
  {
    sendMessage(g_peer, g_packet);
  }

  if(millis() >= g_nextDebugMs)
  {
    printPacketSummary("debug");
    g_nextDebugMs = millis() + 1000;
  }
}
