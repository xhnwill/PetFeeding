#include "app.h"
#include "cJSON.h"
#include "esp_ota_ops.h"
#include "esp_random.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
QueueHandle_t commands;
SemaphoreHandle_t state_lock;
pet_t pet;
char parent_token[17], status_text[96], ack_id[40], ack_error[96],
    device_ip[24];
bool clock_valid;
static nvs_handle_t storage;
static int selected;
static int64_t activity, boot_us, boot_pet, last_save;
static RTC_DATA_ATTR int64_t sleep_wall, sleep_logical;
static RTC_DATA_ATTR uint32_t sleep_cookie;
typedef struct {
  pet_t state;
  uint32_t checksum;
} snapshot_t;
static bool save(void) {
  snapshot_t s = {0};
  s.state = pet;
  s.state.sequence++;
  s.checksum = pet_checksum(&s.state);
  const char *key = (s.state.sequence & 1) ? "slot1" : "slot0";
  if (nvs_set_blob(storage, key, &s, sizeof(s)) != ESP_OK ||
      nvs_commit(storage) != ESP_OK)
    return false;
  pet.sequence = s.state.sequence;
  last_save = esp_timer_get_time();
  return true;
}
static bool load(void) {
  snapshot_t a = {0}, b = {0};
  size_t n = sizeof(a);
  bool va = nvs_get_blob(storage, "slot0", &a, &n) == ESP_OK &&
            n == sizeof(a) && a.state.schema == PET_VERSION &&
            a.checksum == pet_checksum(&a.state);
  n = sizeof(b);
  bool vb = nvs_get_blob(storage, "slot1", &b, &n) == ESP_OK &&
            n == sizeof(b) && b.state.schema == PET_VERSION &&
            b.checksum == pet_checksum(&b.state);
  if (!va && !vb)
    return false;
  pet = vb && (!va || b.state.sequence > a.state.sequence) ? b.state : a.state;
  return true;
}
static int64_t now_pet(void) {
  time_t now = time(NULL);
  if (now > 1700000000 && now < pet.last) {
    clock_valid = false;
    return pet.last;
  }
  if (now > 1700000000) {
    if (pet.born < 1700000000)
      pet_anchor(&pet, now,
                 boot_pet + (esp_timer_get_time() - boot_us) / 1000000);
    clock_valid = true;
    return now;
  }
  clock_valid = false;
  return boot_pet + (esp_timer_get_time() - boot_us) / 1000000;
}
bool app_queue(const char *json, bool parent) {
  if (strlen(json) >= sizeof(((command_t *)0)->json))
    return false;
  command_t c = {.parent = parent};
  strcpy(c.json, json);
  return xQueueSend(commands, &c, 0) == pdTRUE;
}
void app_state(char *out, size_t len, bool privileged) {
  xSemaphoreTake(state_lock, portMAX_DELAY);
  int64_t now = pet.last;
  snprintf(out, len,
           "{\"version\":\"%s\",\"build\":\"%s\",\"id\":\"%08lx%08lx\","
           "\"species\":%lu,\"color\":%lu,\"stage\":%d,\"days\":%lld,"
           "\"growth\":%.3f,\"hunger\":%ld,\"mood\":%ld,\"energy\":%ld,"
           "\"clean\":%ld,\"bond\":%ld,\"poop\":%ld,\"gifts\":%ld,\"sleeping\":"
           "%s,\"clockValid\":%s,\"ip\":\"%s\",\"pairing\":%u,\"friendFound\":%"
           "s,\"ack\":\"%s\",\"error\":\"%s\",\"token\":\"%s\","
           "\"friendSpecies\":%lu,\"friendColor\":%lu}",
           APP_VERSION, APP_BUILD_DATE, (unsigned long)pet.genes[0],
           (unsigned long)pet.genes[1], (unsigned long)(pet.genes[2] % 3),
           (unsigned long)(pet.genes[3] % 4), (int)pet.stage,
           (long long)((now - pet.born) / PET_DAY),
           (double)pet.growth_units / (PET_DAY * 32), (long)pet.hunger,
           (long)pet.mood, (long)pet.energy, (long)pet.clean, (long)pet.bond,
           (long)pet.poop, (long)pet.gifts, pet.sleeping ? "true" : "false",
           clock_valid ? "true" : "false", device_ip, pairing_number,
           friend_found ? "true" : "false", ack_id, ack_error,
           privileged ? parent_token : "", (unsigned long)pet.friend_species,
           (unsigned long)pet.friend_color);
  xSemaphoreGive(state_lock);
}
static const char *str(cJSON *j, const char *key) {
  cJSON *v = cJSON_GetObjectItemCaseSensitive(j, key);
  return cJSON_IsString(v) ? v->valuestring : "";
}
static void command(command_t *c) {
  cJSON *j = cJSON_Parse(c->json);
  if (!j)
    return;
  const char *a = str(j, "action"), *id = str(j, "request");
  /* IDs are echoed as JSON literals only after a strict safe-character check.
   */
  bool safe = true;
  for (const char *s = id; *s; s++)
    if (!((*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9') || *s == '-'))
      safe = false;
  if (!safe || strlen(id) >= sizeof(ack_id)) {
    cJSON_Delete(j);
    return;
  }
  if (*id && !strcmp(id, ack_id)) {
    cJSON_Delete(j);
    return;
  }
  strcpy(ack_id, id);
  ack_error[0] = 0;
  int64_t now = now_pet();
  bool ok = false;
  if (!strcmp(a, "visit") && !c->parent) {
    cJSON *fid = cJSON_GetObjectItem(j, "friend"),
          *sp = cJSON_GetObjectItem(j, "species"),
          *col = cJSON_GetObjectItem(j, "color");
    if (cJSON_IsNumber(fid) && fid->valuedouble >= 0 &&
        fid->valuedouble <= UINT32_MAX && cJSON_IsNumber(sp) &&
        sp->valuedouble >= 0 && sp->valuedouble < 3 && cJSON_IsNumber(col) &&
        col->valuedouble >= 0 && col->valuedouble < 4) {
      ok = pet_visit(&pet, (uint32_t)fid->valuedouble, now);
      if (ok) {
        pet.friend_species = (uint32_t)sp->valuedouble;
        pet.friend_color = (uint32_t)col->valuedouble;
        strcpy(status_text, "朋友来啦，合照留念");
      }
    }
    if (!ok)
      strcpy(ack_error, "daily_gift_limit");
  } else if (c->parent) {
    if (!strcmp(a, "time")) {
      cJSON *t = cJSON_GetObjectItem(j, "epoch");
      int64_t target = cJSON_IsNumber(t) && t->valuedouble >= 1700000000 &&
                               t->valuedouble <= 4102444800.0
                           ? (int64_t)t->valuedouble
                           : 0;
      if (target >= 1700000000 && target <= 4102444800LL &&
          target >= pet.last) {
        if (pet.born < 1700000000) {
          pet_anchor(&pet, target, now);
        }
        struct timeval tv = {.tv_sec = target};
        settimeofday(&tv, NULL);
        clock_valid = true;
        ok = true;
      }
    } else if (!strcmp(a, "wifi"))
      ok = network_wifi(str(j, "ssid"), str(j, "password"));
    else if (!strcmp(a, "friends")) {
      network_stop();
      radio_window(true);
      ok = true;
    } else if (!strcmp(a, "ota")) {
      const char *url = str(j, "url");
      if (clock_valid && !strncmp(url, "https://", 8) && strlen(url) < 256 &&
          strlen(device_ip)) {
        save();
        network_ota(url);
        ok = true;
      }
    } else
      ok = pet_action(&pet, a, now);
  }
  if (!ok && !ack_error[0])
    strcpy(ack_error, "action_rejected");
  if (ok) {
    activity = esp_timer_get_time();
    if (!save())
      strcpy(ack_error, "save_failed");
  }
  cJSON_Delete(j);
}
void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(nvs_flash_init_partition("petdata"));
  ESP_ERROR_CHECK(
      nvs_open_from_partition("petdata", "mypat", NVS_READWRITE, &storage));
  state_lock = xSemaphoreCreateMutex();
  commands = xQueueCreate(8, sizeof(command_t));
  /* Start radio/WiFi before using hardware randomness. */
  network_init();
  radio_init();
  vTaskDelay(pdMS_TO_TICKS(300));
  if (!load()) {
    size_t n = 0;
    if (nvs_get_blob(storage, "slot0", NULL, &n) == ESP_OK ||
        nvs_get_blob(storage, "slot1", NULL, &n) == ESP_OK) {
      /* Preserve corrupted records for recovery; never silently replace a pet.
       */
      board_init();
      while (1) {
        board_render(0, 0, "存档需要恢复", "请联系家长");
        vTaskDelay(pdMS_TO_TICKS(1000));
      }
    }
    uint32_t seed[4];
    esp_fill_random(seed, sizeof(seed));
    pet_new(&pet, seed, time(NULL) > 1700000000 ? time(NULL) : 0);
    if (!save())
      abort();
  }
  size_t size = sizeof(parent_token);
  if (nvs_get_str(storage, "token", parent_token, &size) != ESP_OK) {
    snprintf(parent_token, sizeof(parent_token), "%08lx%08lx",
             (unsigned long)esp_random(), (unsigned long)esp_random());
    ESP_ERROR_CHECK(nvs_set_str(storage, "token", parent_token));
    ESP_ERROR_CHECK(nvs_commit(storage));
  }
  boot_us = esp_timer_get_time();
  boot_pet = pet.last;
  activity = boot_us;
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO &&
      sleep_cookie == 0x50455432) {
    int64_t elapsed = time(NULL) - sleep_wall;
    if (elapsed > 0 && time(NULL) < 1700000000)
      boot_pet = sleep_logical + elapsed;
  }
  sleep_cookie = 0;
  board_init();
  network_resume();
  strcpy(status_text, "你好呀，一起长大");
  /* Confirm this OTA image after initialization and valid save restore. */
  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t image_state;
  if (esp_ota_get_state_partition(running, &image_state) == ESP_OK &&
      image_state == ESP_OTA_IMG_PENDING_VERIFY)
    esp_ota_mark_app_valid_cancel_rollback();
  int phase = 0;
  bool parent_page = false;
  int game = 0, ball = 0, score = 0;
  int64_t game_start = 0;
  while (1) {
    xSemaphoreTake(state_lock, portMAX_DELAY);
    int64_t now = now_pet();
    pet_tick(&pet, now);
    command_t c;
    while (xQueueReceive(commands, &c, 0))
      command(&c);
    int key = board_key();
    if (key) {
      activity = esp_timer_get_time();
      board_screen(true);
    }
    if (pairing_pending) {
      if (key == 6)
        radio_confirm(true);
      if (key == 1)
        radio_confirm(false);
    } else if (game) {
      if (key == 2)
        ball--;
      if (key == 4)
        ball++;
      if (ball < 0)
        ball = 0;
      if (ball > 4)
        ball = 4;
      if (phase % 50 == 0 && ball == (phase / 50) % 5)
        score++;
      snprintf(status_text, sizeof(status_text), "星%d 篮%d 分%d",
               (phase / 50) % 5 + 1, ball + 1, score);
      if (key == 1 || esp_timer_get_time() - game_start > 30000000) {
        game = 0;
        pet_action(&pet, "play", now);
        save();
      }
    } else if (key == 7) {
      parent_page = !parent_page;
      if (parent_page) {
        radio_window(false);
        strcpy(status_text, "网页连接后请确认配对");
      } else
        radio_stop();
    } else if (key == 1) {
      parent_page = false;
      radio_stop();
      strcpy(status_text, "回家啦");
    } else if (key == 2 || key == 3)
      selected = (selected + 5) % 6;
    else if (key == 4 || key == 5)
      selected = (selected + 1) % 6;
    else if (key == 6) {
      const char *actions[] = {"feed",  "play",    "clean",
                               "sleep", "friends", "pet"};
      if (selected == 4) {
        if (friend_found)
          radio_connect_friend();
        else {
          network_stop();
          radio_window(true);
        }
        strcpy(status_text, "请朋友也打开寻找");
      } else if (selected == 1) {
        game = 1;
        score = 0;
        ball = 2;
        game_start = esp_timer_get_time();
      } else {
        bool ok = pet_action(
            &pet, selected == 3 && pet.sleeping ? "wake" : actions[selected],
            now);
        strcpy(status_text, ok ? "收到啦，谢谢你" : "现在不需要哦");
        save();
      }
    }
    radio_poll();
    if (esp_timer_get_time() - activity > 300000000LL && !pet.sleeping) {
      pet_action(&pet, "sleep", now);
      save();
    }
    if (esp_timer_get_time() - last_save > 300000000LL)
      save();
    char detail[96];
    if (pairing_number)
      snprintf(detail, sizeof(detail), "PAIR %06u OK", pairing_number);
    else if (parent_page)
      snprintf(detail, sizeof(detail), "%s", parent_token);
    else
      snprintf(detail, sizeof(detail), "v%s %s", APP_VERSION,
               clock_valid ? "" : "请网页校时");
    if (phase % 5 == 0)
      board_render(selected, phase / 5, parent_page ? device_ip : status_text,
                   detail);
    phase++;
    if (esp_timer_get_time() - activity > 60000000LL)
      board_screen(false);
    if (esp_timer_get_time() - activity > 330000000LL && !radio_busy()) {
      save();
      sleep_wall = time(NULL);
      sleep_logical = pet.last;
      sleep_cookie = 0x50455432;
      network_stop();
      radio_stop();
      board_sleep();
    }
    xSemaphoreGive(state_lock);
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
