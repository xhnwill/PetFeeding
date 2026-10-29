#pragma once
#include <stdbool.h>
#include <stdint.h>
#define PET_VERSION 2
#define PET_DAY 86400LL
typedef struct {
  uint32_t schema, sequence, genes[4];
  int64_t born, last, sleep_start, growth_units, stage_at, window;
  uint32_t sleep_seconds;
  int32_t hunger, mood, energy, clean, bond, poop, meals, stage, gifts;
  bool sleeping;
  uint32_t last_friend;
  uint32_t friend_species, friend_color;
  int64_t gift_day;
} pet_t;
void pet_new(pet_t *p, const uint32_t seed[4], int64_t now);
void pet_tick(pet_t *p, int64_t now);
void pet_anchor(pet_t *p, int64_t wall, int64_t logical);
bool pet_action(pet_t *p, const char *action, int64_t now);
bool pet_visit(pet_t *p, uint32_t friend_id, int64_t now);
uint32_t pet_checksum(const pet_t *p);
