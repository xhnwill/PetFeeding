#include "app.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "lwip/apps/sntp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static httpd_handle_t server;
static bool wifi_started;
static int retries;
static QueueHandle_t ota_queue;
extern const uint8_t html_start[] asm("_binary_index_html_start"),
    html_end[] asm("_binary_index_html_end");
extern const uint8_t js_start[] asm("_binary_app_js_start"),
    js_end[] asm("_binary_app_js_end");
extern const uint8_t css_start[] asm("_binary_style_css_start"),
    css_end[] asm("_binary_style_css_end");
static bool authorized(httpd_req_t *r) {
  char token[32];
  return httpd_req_get_hdr_value_str(r, "X-MyPat-Token", token,
                                     sizeof(token)) == ESP_OK &&
         strlen(token) == 16 && !memcmp(token, parent_token, 16);
}
static esp_err_t asset(httpd_req_t *r) {
  const uint8_t *s = html_start, *e = html_end;
  const char *type = "text/html; charset=utf-8";
  if (!strcmp(r->uri, "/app.js")) {
    s = js_start;
    e = js_end;
    type = "text/javascript";
  }
  if (!strcmp(r->uri, "/style.css")) {
    s = css_start;
    e = css_end;
    type = "text/css";
  }
  httpd_resp_set_type(r, type);
  httpd_resp_set_hdr(r, "Cache-Control", "no-store");
  httpd_resp_set_hdr(r, "X-Content-Type-Options", "nosniff");
  return httpd_resp_send(r, (const char *)s, e - s - 1);
}
static esp_err_t state_get(httpd_req_t *r) {
  if (!authorized(r))
    return httpd_resp_send_err(r, HTTPD_401_UNAUTHORIZED,
                               "Enter token from device");
  char state[1024];
  app_state(state, sizeof(state), false);
  httpd_resp_set_type(r, "application/json");
  httpd_resp_set_hdr(r, "Cache-Control", "no-store");
  return httpd_resp_sendstr(r, state);
}
static esp_err_t command_post(httpd_req_t *r) {
  if (!authorized(r))
    return httpd_resp_send_err(r, HTTPD_401_UNAUTHORIZED, "Unauthorized");
  if (r->content_len < 2 || r->content_len > 510)
    return httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "Invalid length");
  char body[512];
  int total = 0;
  while (total < r->content_len) {
    int n = httpd_req_recv(r, body + total, r->content_len - total);
    if (n <= 0)
      return ESP_FAIL;
    total += n;
  }
  body[total] = 0;
  if (!app_queue(body, true))
    return httpd_resp_send_err(r, HTTPD_500_INTERNAL_SERVER_ERROR,
                               "Queue full");
  httpd_resp_set_type(r, "application/json");
  return httpd_resp_sendstr(r, "{\"queued\":true}");
}
static void start_server(void) {
  if (server)
    return;
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.stack_size = 6144;
  if (httpd_start(&server, &cfg) != ESP_OK)
    return;
  const char *paths[] = {"/", "/app.js", "/style.css"};
  for (int i = 0; i < 3; i++) {
    httpd_uri_t u = {.uri = paths[i], .method = HTTP_GET, .handler = asset};
    httpd_register_uri_handler(server, &u);
  }
  httpd_uri_t u = {
      .uri = "/api/state", .method = HTTP_GET, .handler = state_get};
  httpd_register_uri_handler(server, &u);
  u = (httpd_uri_t){
      .uri = "/api/command", .method = HTTP_POST, .handler = command_post};
  httpd_register_uri_handler(server, &u);
}
static void event(void *arg, esp_event_base_t base, int32_t id, void *data) {
  if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START)
    esp_wifi_connect();
  if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
    strcpy(device_ip, "");
    if (wifi_started && retries++ < 4)
      esp_wifi_connect();
  }
  if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *e = data;
    snprintf(device_ip, sizeof(device_ip), IPSTR, IP2STR(&e->ip_info.ip));
    retries = 0;
    start_server();
    if (!sntp_enabled()) {
      sntp_setoperatingmode(SNTP_OPMODE_POLL);
      sntp_setservername(0, "pool.ntp.org");
      sntp_init();
    }
  }
}
static void ota_task(void *arg) {
  char url[256];
  while (xQueueReceive(ota_queue, url, portMAX_DELAY)) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    radio_stop();
    esp_http_client_config_t http = {.url = url,
                                     .crt_bundle_attach = esp_crt_bundle_attach,
                                     .timeout_ms = 15000,
                                     .keep_alive_enable = true};
    esp_https_ota_config_t config = {.http_config = &http};
    esp_err_t err = esp_https_ota(&config);
    if (err == ESP_OK)
      esp_restart();
    ESP_LOGE("OTA", "Update failed: %s", esp_err_to_name(err));
  }
}
void network_init(void) {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event, NULL);
  esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event, NULL);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ota_queue = xQueueCreate(1, 256);
  xTaskCreate(ota_task, "https_ota", 8192, NULL, 3, NULL);
}
bool network_wifi(const char *ssid, const char *password) {
  size_t s = strlen(ssid), p = strlen(password);
  if (!s || s > 32 || p > 63)
    return false;
  wifi_config_t c = {0};
  memcpy(c.sta.ssid, ssid, s);
  memcpy(c.sta.password, password, p);
  c.sta.pmf_cfg.capable = true;
  if (wifi_started) {
    wifi_started = false;
    esp_wifi_stop();
  }
  if (esp_wifi_set_config(WIFI_IF_STA, &c) != ESP_OK)
    return false;
  retries = 0;
  wifi_started = true;
  return esp_wifi_start() == ESP_OK;
}
void network_stop(void) {
  wifi_started = false;
  esp_wifi_stop();
  strcpy(device_ip, "");
}
void network_resume(void) {
  wifi_config_t config;
  if (esp_wifi_get_config(WIFI_IF_STA, &config) == ESP_OK &&
      config.sta.ssid[0]) {
    retries = 0;
    wifi_started = true;
    esp_wifi_start();
  }
}
void network_ota(const char *url) {
  char copy[256] = {0};
  if (strncmp(url, "https://", 8) || strlen(url) >= sizeof(copy))
    return;
  strcpy(copy, url);
  xQueueSend(ota_queue, copy, 0);
}
