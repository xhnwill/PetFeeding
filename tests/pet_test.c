#include "../firmware/main/pet.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static pet_t fresh(void) {
  pet_t p;
  uint32_t seed[4] = {1, 2, 3, 4};
  pet_new(&p, seed, 0);
  return p;
}
int main(void) {
  pet_t p = fresh();
  pet_tick(&p, 10);
  int64_t growth = p.growth_units;
  pet_anchor(&p, 1793289600, 10);
  pet_tick(&p, 1793289600);
  assert(p.stage == 0 && p.growth_units == growth && p.born == 1793289590);
  p = fresh();
  pet_action(&p, "sleep", 0);
  pet_tick(&p, PET_DAY);
  assert(p.growth_units == PET_DAY * 32);
  assert(p.sleep_seconds == 57600);
  assert(p.stage == 0);
  pet_tick(&p, PET_DAY * 180);
  assert(p.stage == 4);
  assert(p.poop <= 3);
  p = fresh();
  pet_tick(&p, PET_DAY * 30);
  assert(p.stage == 0);
  assert(p.growth_units == PET_DAY * 16 * 30);
  pet_tick(&p, PET_DAY * 60);
  assert(p.stage == 1);
  pet_t saved = p;
  pet_tick(&p, PET_DAY * 20);
  assert(!memcmp(&p, &saved, sizeof(p)));
  pet_t a = fresh(), b = fresh();
  pet_action(&a, "sleep", 0);
  pet_action(&b, "sleep", 0);
  for (int i = 1; i <= 172800; i++)
    pet_tick(&a, i);
  pet_tick(&b, 172800);
  assert(a.growth_units == b.growth_units);
  assert(a.hunger == b.hunger);
  assert(a.energy == b.energy);
  p = fresh();
  pet_action(&p, "sleep", 0);
  pet_tick(&p, 28800);
  pet_action(&p, "wake", 28800);
  pet_tick(&p, PET_DAY);
  assert(p.growth_units == PET_DAY * 24);
  p = fresh();
  assert(pet_visit(&p, 1, 0));
  assert(!pet_visit(&p, 2, 1));
  assert(pet_visit(&p, 2, PET_DAY));
  assert(p.gifts == 2);
  uint32_t checksum = pet_checksum(&p);
  p.gifts++;
  assert(pet_checksum(&p) != checksum);
  p = fresh();
  assert(!pet_action(&p, "nonsense", 0));
  p.hunger = 100;
  assert(!pet_action(&p, "feed", 0));
  for (int i = 0; i < 100; i++) {
    p.hunger = 0;
    pet_action(&p, "feed", i);
  }
  assert(p.poop == 3);
  puts("pet core: growth cap, monthly gates, rollback, tick cadence, gifts, "
       "checksum and poop passed");
}
