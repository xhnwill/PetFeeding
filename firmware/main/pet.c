#include "pet.h"
#include <limits.h>
void pet_anchor(pet_t *p, int64_t wall, int64_t logical) {
  int64_t shift = wall - logical;
  p->born += shift;
  p->last += shift;
  p->stage_at += shift;
  if (p->sleeping)
    p->sleep_start += shift;
}
#include <string.h>
static int clamp(int v) { return v < 0 ? 0 : v > 100 ? 100 : v; }
void pet_new(pet_t *p, const uint32_t seed[4], int64_t now) {
  memset(p, 0, sizeof(*p));
  p->schema = PET_VERSION;
  memcpy(p->genes, seed, 16);
  p->born = p->last = p->stage_at = now;
  p->hunger = 80;
  p->mood = 80;
  p->energy = 80;
  p->clean = 100;
  p->gift_day = -1;
}
void pet_tick(pet_t *p, int64_t now) {
  if (now <= p->last || now < 0)
    return;
  /* Fixed birth-relative windows. Units: one full day = 2*86400*16. */
  const int64_t unit = PET_DAY * 32, threshold = 30 * unit;
  while (p->last < now) {
    int64_t win = (p->last - p->born) / PET_DAY;
    if (win != p->window) {
      p->window = win;
      p->sleep_seconds = 0;
    }
    int64_t end = p->born + (win + 1) * PET_DAY;
    if (end > now)
      end = now;
    int64_t dt = end - p->last, bonus = 0;
    if (p->sleeping) {
      bonus = 16 * 3600 - p->sleep_seconds;
      if (bonus > dt)
        bonus = dt;
      if (bonus < 0)
        bonus = 0;
      p->sleep_seconds += (uint32_t)bonus;
    }
    p->growth_units += dt * 16 + bonus * 24;
    /* Absolute cumulative thresholds; retain historical milestone times. */
    while (p->stage < 4 && p->growth_units >= (p->stage + 1) * threshold &&
           end - p->stage_at >= 30 * PET_DAY) {
      p->stage++;
      p->stage_at = end;
    }
    /* Quantize decay by crossed hourly boundaries, independent of tick cadence.
     */
    int64_t hours = (end - p->born) / 3600 - (p->last - p->born) / 3600;
    p->hunger = clamp(p->hunger - (int)(hours * (p->sleeping ? 1 : 4)));
    if (p->sleeping)
      p->energy = clamp(p->energy + (int)hours * 5);
    else {
      p->energy = clamp(p->energy - (int)hours * 2);
      p->mood = clamp(p->mood - (int)hours * 2);
    }
    p->last = end;
  }
}
bool pet_action(pet_t *p, const char *a, int64_t now) {
  pet_tick(p, now);
  if (!strcmp(a, "sleep")) {
    p->sleeping = true;
    p->sleep_start = now;
    return true;
  }
  if (!strcmp(a, "wake")) {
    p->sleeping = false;
    return true;
  }
  if (!strcmp(a, "feed")) {
    if (p->hunger > 90)
      return false;
    p->sleeping = false;
    p->hunger = clamp(p->hunger + 25);
    p->bond = clamp(p->bond + 1);
    if (++p->meals % 3 == 0 && p->poop < 3) {
      p->poop++;
      p->clean = clamp(p->clean - 20);
    }
    return true;
  }
  if (!strcmp(a, "clean")) {
    p->poop = 0;
    p->clean = 100;
    return true;
  }
  if (!strcmp(a, "play")) {
    if (p->energy < 10)
      return false;
    p->sleeping = false;
    p->energy = clamp(p->energy - 10);
    p->mood = clamp(p->mood + 15);
    p->bond = clamp(p->bond + 2);
    return true;
  }
  if (!strcmp(a, "pet")) {
    p->mood = clamp(p->mood + 3);
    return true;
  }
  return false;
}
bool pet_visit(pet_t *p, uint32_t id, int64_t now) {
  pet_tick(p, now);
  int64_t day = (now - p->born) / PET_DAY;
  /* One non-scarce keepsake per local growth day, even across different
   * friends. */
  if (p->gift_day == day)
    return false;
  p->gift_day = day;
  p->last_friend = id;
  p->gifts++;
  p->mood = clamp(p->mood + 10);
  return true;
}
uint32_t pet_checksum(const pet_t *p) {
  const uint8_t *b = (const uint8_t *)p;
  uint32_t crc = 2166136261u;
  for (unsigned i = 0; i < sizeof(*p); i++)
    crc = (crc ^ b[i]) * 16777619u;
  return crc;
}
