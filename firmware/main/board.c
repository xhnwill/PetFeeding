#include "app.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "font.h"
#include "pet_art.h"
#include "pet_motion.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
/* RCQB_ESP32C3: ST7789V3 1.69", two resistor ladders. */
#define W 240
#define H 280
static spi_device_handle_t lcd;
static adc_oneshot_unit_handle_t adc;
static uint16_t pixels[W * 20];
static int strip_y;
static void tx(int dc, const void *data, int len) {
  gpio_set_level(7, dc);
  spi_transaction_t t = {.length = len * 8, .tx_buffer = data};
  ESP_ERROR_CHECK(spi_device_polling_transmit(lcd, &t));
}
static void cmd(uint8_t c, const uint8_t *data, int n) {
  tx(0, &c, 1);
  if (n)
    tx(1, data, n);
}
void board_screen(bool on) { gpio_set_level(4, on); }
void board_init(void) {
  gpio_config_t pins = {.pin_bit_mask = (1ULL << 4) | (1ULL << 7) |
                                        (1ULL << 8) | (1ULL << 18),
                        .mode = GPIO_MODE_OUTPUT};
  ESP_ERROR_CHECK(gpio_config(&pins));
  gpio_set_level(18, 0);
  gpio_set_level(4, 0);
  gpio_config_t wake = {
      .pin_bit_mask = 1ULL << 2, .mode = GPIO_MODE_INPUT, .pull_up_en = 1};
  gpio_config(&wake);
  spi_bus_config_t bus = {.mosi_io_num = 10,
                          .miso_io_num = -1,
                          .sclk_io_num = 5,
                          .quadwp_io_num = -1,
                          .quadhd_io_num = -1,
                          .max_transfer_sz = sizeof(pixels)};
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
  spi_device_interface_config_t dev = {.clock_speed_hz = 40000000,
                                       .mode = 0,
                                       .spics_io_num = 6,
                                       .queue_size = 1};
  ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev, &lcd));
  gpio_set_level(8, 0);
  vTaskDelay(pdMS_TO_TICKS(100));
  gpio_set_level(8, 1);
  vTaskDelay(pdMS_TO_TICKS(100));
  cmd(0x11, NULL, 0);
  vTaskDelay(pdMS_TO_TICKS(120));
  uint8_t mad = 0x60, format = 0x55;
  cmd(0x36, &mad, 1);
  cmd(0x3a, &format, 1);
  cmd(0x21, NULL, 0);
  cmd(0x29, NULL, 0);
  adc_oneshot_unit_init_cfg_t ac = {.unit_id = ADC_UNIT_1};
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&ac, &adc));
  adc_oneshot_chan_cfg_t ch = {.atten = ADC_ATTEN_DB_12,
                               .bitwidth = ADC_BITWIDTH_12};
  adc_oneshot_config_channel(adc, ADC_CHANNEL_0, &ch);
  adc_oneshot_config_channel(adc, ADC_CHANNEL_1, &ch);
  board_screen(true);
}
int board_key(void) {
  int a = 0, b = 0;
  for (int i = 0; i < 4; i++) {
    int x, y;
    adc_oneshot_read(adc, ADC_CHANNEL_0, &x);
    adc_oneshot_read(adc, ADC_CHANNEL_1, &y);
    a += x;
    b += y;
  }
  a /= 4;
  b /= 4;
  int key = 0;
  if (a >= 2965 && a <= 3165)
    key = 1;
  else if (a >= 2154 && a <= 2354)
    key = 2;
  else if (a >= 1479 && a <= 1679)
    key = 3;
  else if (a >= 888 && a <= 1088)
    key = 4;
  if (b >= 2164 && b <= 2364)
    key = 7;
  else if (b >= 1482 && b <= 1682)
    key = 5;
  else if (b >= 888 && b <= 1088)
    key = 6;
  static int candidate, stable, last;
  static int count;
  if (candidate == key)
    count++;
  else {
    candidate = key;
    count = 0;
  }
  if (count >= 1)
    stable = candidate;
  int event = stable != last ? stable : 0;
  last = stable;
  return event;
}
static uint16_t rgb(int r, int g, int b) {
  uint16_t c = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
  return (c >> 8) | (c << 8);
}
static void rect(int x, int y, int w, int h, uint16_t c) {
  int end = y + h;
  if (y < strip_y)
    y = strip_y;
  if (end > strip_y + 20)
    end = strip_y + 20;
  int right = x + w;
  if (x < 0)
    x = 0;
  if (right > W)
    right = W;
  for (int j = y; j < end; j++)
    for (int i = x; i < right; i++)
      pixels[(j - strip_y) * W + i] = c;
}
static void ellipse(int x, int y, int rx, int ry, uint16_t c) {
  for (int j = y - ry; j <= y + ry; j++)
    if (j >= strip_y && j < strip_y + 20) {
      int dx = (int)(rx * sqrtf(1.0f - (float)((j - y) * (j - y)) / (ry * ry)));
      rect(x - dx, j, dx * 2 + 1, 1, c);
    }
}
static uint32_t rune(const char **s) {
  const unsigned char *p = (const unsigned char *)*s;
  uint32_t c = *p++;
  if (c >= 0xe0) {
    c = ((c & 15) << 12) | ((p[0] & 63) << 6) | (p[1] & 63);
    p += 2;
  } else if (c >= 0xc0) {
    c = ((c & 31) << 6) | (p[0] & 63);
    p++;
  }
  *s = (const char *)p;
  return c;
}
static void text(int x, int y, const char *s, uint16_t color) {
  while (*s && x < W - 8) {
    uint32_t c = rune(&s);
    int index = -1;
    for (unsigned i = 0; i < FONT_COUNT; i++)
      if (font_codes[i] == c) {
        index = i;
        break;
      }
    if (index >= 0)
      for (int j = 0; j < 18; j++)
        for (int i = 0; i < 16; i++)
          if (font_bits[index][j] & (1u << (15 - i)))
            rect(x + i, y + j, 1, 1, color);
    x += c < 128 ? 9 : 16;
  }
}
static void sprite(unsigned species, int x, int y, int size,
                   pet_anim_action_t action, unsigned frame) {
  bool packed = action != PET_ANIM_IDLE;
  const uint8_t *data =
      packed ? pet_motion_pixels[species % 3][action - 1][frame % 4]
             : pet_art_pixels[species % 3];
  const uint16_t *palette = packed
                                ? pet_motion_palettes[species % 3][action - 1]
                                : pet_art_palettes[species % 3];
  int top = y > strip_y ? y : strip_y,
      bottom = y + size < strip_y + 20 ? y + size : strip_y + 20;
  for (int row = top; row < bottom; row++) {
    int sy = (row - y) * PET_ART_SIZE / size;
    for (int col = 0; col < size; col++) {
      unsigned pixel = sy * PET_ART_SIZE + col * PET_ART_SIZE / size;
      unsigned index = packed ? ((data[pixel / 2] >> (pixel % 2 ? 0 : 4)) & 15)
                              : data[pixel];
      if (index)
        rect(x + col, row, 1, 1, palette[index]);
    }
  }
}
void board_render(int selection, int phase, const char *note,
                  const char *detail) {
  uint16_t ink = rgb(44, 60, 66), white = rgb(255, 255, 255);
  uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
  pet_anim_action_t action =
      pet_anim_select(&pet_animation, pet.sleeping, pet_game_active, now);
  unsigned frame = pet_anim_frame(&pet_animation, action, now);
  for (strip_y = 0; strip_y < H; strip_y += 20) {
    rect(0, strip_y, W, 20, rgb(249, 245, 235));
    text(12, 10, "我的小伙伴", ink);
    char info[64];
    snprintf(info, sizeof(info), "Lv.%d  %lld天", (int)pet.stage + 1,
             (long long)((pet.last - pet.born) / PET_DAY));
    text(12, 32, info, ink);
    ellipse(120, 177, 60, 9, rgb(216, 223, 208));
    const int colors[4][3] = {
        {139, 181, 163}, {228, 162, 137}, {167, 159, 205}, {222, 190, 110}};
    const int *col = colors[pet.genes[3] % 4];
    uint16_t body = rgb(col[0], col[1], col[2]);
    int bob = action == PET_ANIM_IDLE ? (phase % 8 < 4 ? 0 : 3) : 0,
        size = 96 + pet.stage * 6;
    sprite(pet.genes[2] % 3, 120 - size / 2, 174 - size + bob, size, action,
           frame);
    ellipse(215, 39, 4, 4, body);
    if (pet.gifts)
      sprite(pet.friend_species % 3, 172, 132, 44, PET_ANIM_IDLE, 0);
    if (pet.sleeping)
      text(170, 78, "Zzz", ink);
    for (int i = 0; i < pet.poop; i++)
      ellipse(185 + i * 12, 178, 5, 5, rgb(140, 101, 70));
    int vals[] = {pet.hunger, pet.mood, pet.energy};
    const char *labels[] = {"饭", "乐", "力"};
    for (int i = 0; i < 3; i++) {
      text(8 + i * 78, 199, labels[i], ink);
      rect(28 + i * 78, 205, 42, 5, rgb(220, 225, 219));
      rect(28 + i * 78, 205, 42 * vals[i] / 100, 5, body);
    }
    const char *labels2[] = {"吃饭", "玩耍", "清洁", "睡觉", "朋友", "摸摸"};
    rect(0, 222, W, 25, white);
    text(13, 225, labels2[selection], ink);
    text(74, 225, pet.sleeping ? "正在睡觉" : "陪你玩呀", ink);
    text(8, 249, note, ink);
    text(8, 262, detail, ink);
    uint16_t y0 = strip_y + 20, y1 = strip_y + 39;
    uint8_t xs[] = {0, 0, 0, 239},
            ys[] = {y0 >> 8, y0 & 255, y1 >> 8, y1 & 255};
    cmd(0x2a, xs, 4);
    cmd(0x2b, ys, 4);
    uint8_t ram = 0x2c;
    tx(0, &ram, 1);
    tx(1, pixels, sizeof(pixels));
  }
}
void board_sleep(void) {
  board_screen(false);
  cmd(0x10, NULL, 0);
  esp_deep_sleep_enable_gpio_wakeup(1ULL << 2, ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();
}
