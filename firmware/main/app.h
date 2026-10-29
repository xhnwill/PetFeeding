#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "pet.h"
#define APP_VERSION "0.2.0"
#define APP_BUILD_DATE "2026-10-29"
typedef struct {
  char json[512];
  bool parent;
} command_t;
extern QueueHandle_t commands;
extern SemaphoreHandle_t state_lock;
extern pet_t pet;
extern char parent_token[17], status_text[96], ack_id[40], ack_error[96],
    device_ip[24];
extern bool clock_valid;
void app_state(char *out, size_t len, bool privileged);
bool app_queue(const char *json, bool parent);
void board_init(void);
int board_key(void);
void board_render(int selection, int phase, const char *note,
                  const char *detail);
void board_screen(bool on);
void board_sleep(void);
void radio_init(void);
void radio_window(bool friends);
void radio_stop(void);
void radio_connect_friend(void);
void radio_confirm(bool accept);
bool radio_busy(void);
void radio_poll(void);
extern unsigned pairing_number;
extern bool pairing_pending, friend_found;
void network_init(void);
void network_resume(void);
bool network_wifi(const char *ssid, const char *password);
void network_stop(void);
void network_ota(const char *url);
