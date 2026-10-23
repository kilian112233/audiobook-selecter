#pragma once

// Live microphone streaming for the audiobook controller.
// Mic (analog) is on GPIO3 = ADC1_CH2 on the ESP32-S3.
// Samples continuously at 16 kHz and serves an endless WAV stream at
// http://<device-ip>:8080/mic  (PCM 16-bit mono 16 kHz).

#include <cstdint>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_continuous.h"
#include "lwip/sockets.h"

namespace mic_stream {

static const int kPort = 8080;
static const int kSampleRate = 16000;
static const int kFrameBytes = 1024;  // 256 samples x 4 raw bytes

static adc_continuous_handle_t s_adc = nullptr;

static bool init_adc() {
  adc_continuous_handle_cfg_t hcfg = {};
  hcfg.max_store_buf_size = 4096;
  hcfg.conv_frame_size = kFrameBytes;
  if (adc_continuous_new_handle(&hcfg, &s_adc) != ESP_OK)
    return false;

  static adc_digi_pattern_config_t pat = {};
  pat.atten = ADC_ATTEN_DB_12;
  pat.channel = ADC_CHANNEL_2;  // GPIO3 on ESP32-S3
  pat.unit = ADC_UNIT_1;
  pat.bit_width = ADC_BITWIDTH_12;

  adc_continuous_config_t ccfg = {};
  ccfg.pattern_num = 1;
  ccfg.adc_pattern = &pat;
  ccfg.sample_freq_hz = kSampleRate;
  if (adc_continuous_config(s_adc, &ccfg) != ESP_OK)
    return false;
  return adc_continuous_start(s_adc) == ESP_OK;
}

// WAV header: PCM, mono, 16 kHz, 16-bit. Sizes left maxed for an endless stream.
static const char kWavHeader[44] = {
    'R', 'I', 'F', 'F', (char) 0xFF, (char) 0xFF, (char) 0xFF, (char) 0xFF,
    'W', 'A', 'V', 'E', 'f', 'm', 't', ' ',
    16, 0, 0, 0,            // fmt chunk size
    1, 0,                   // PCM
    1, 0,                   // mono
    (char) 0x80, (char) 0x3E, 0, 0,  // sample rate 16000
    0, (char) 0x7D, 0, 0,   // byte rate 32000
    2, 0,                   // block align
    16, 0,                  // bits per sample
    'd', 'a', 't', 'a', (char) 0xFF, (char) 0xFF, (char) 0xFF, (char) 0xFF};

static void stream_task(void *arg) {
  init_adc();

  int srv = ::socket(AF_INET, SOCK_STREAM, 0);
  int one = 1;
  ::setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ::htonl(INADDR_ANY);
  addr.sin_port = ::htons(kPort);
  ::bind(srv, (sockaddr *) &addr, sizeof(addr));
  ::listen(srv, 1);

  static uint8_t raw[kFrameBytes];
  static int16_t pcm[kFrameBytes / 4];
  uint32_t got = 0;

  for (;;) {
    int fd = ::accept(srv, nullptr, nullptr);
    if (fd < 0)
      continue;

    char req[256] = {};
    ::recv(fd, req, sizeof(req) - 1, 0);

    if (strncmp(req, "GET /mic", 8) != 0) {
      const char *nf = "HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
      ::send(fd, nf, strlen(nf), 0);
      ::close(fd);
      continue;
    }

    timeval tv = {3, 0};
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    int nd = 1;
    ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nd, sizeof(nd));

    const char *hdr = "HTTP/1.1 200 OK\r\nContent-Type: audio/wav\r\n"
                      "Cache-Control: no-store\r\nConnection: close\r\n\r\n";
    if (::send(fd, hdr, strlen(hdr), 0) <= 0 || ::send(fd, kWavHeader, 44, 0) <= 0) {
      ::close(fd);
      continue;
    }

    // Drop stale buffered samples so the stream starts live.
    while (adc_continuous_read(s_adc, raw, kFrameBytes, &got, 0) == ESP_OK && got == kFrameBytes) {
    }

    while (true) {
      if (adc_continuous_read(s_adc, raw, kFrameBytes, &got, portMAX_DELAY) != ESP_OK || got < 4)
        break;
      int n = got / 4;
      for (int i = 0; i < n; i++) {
        uint32_t w;
        memcpy(&w, raw + i * 4, 4);
        int v = (int) (w & 0xFFF) - 2048;  // 12-bit sample, midscale bias ~2048
        pcm[i] = (int16_t) (v << 4);
      }
      char *p = (char *) pcm;
      int len = n * 2;
      bool ok = true;
      while (len > 0 && ok) {
        int s = ::send(fd, p, len, 0);
        if (s <= 0)
          ok = false;
        else {
          p += s;
          len -= s;
        }
      }
      if (!ok)
        break;  // client went away
    }
    ::close(fd);
  }
}

inline void start() {
  xTaskCreatePinnedToCore(stream_task, "mic_stream", 8192, nullptr, 2, nullptr, 0);
}

}  // namespace mic_stream
