#include "../firmware/main/pet_animation.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int main(void) {
  pet_anim_t a = {0};
  assert(pet_anim_select(&a, false, false, 10) == PET_ANIM_IDLE);
  pet_anim_trigger(&a, PET_ANIM_EAT, 100);
  assert(pet_anim_select(&a, false, false, 100) == PET_ANIM_EAT);
  assert(pet_anim_frame(&a, PET_ANIM_EAT, 320) == 1);
  assert(pet_anim_frame(&a, PET_ANIM_EAT, 980) == 0);
  assert(pet_anim_select(&a, true, true, 500) == PET_ANIM_SLEEP);
  assert(pet_anim_select(&a, false, false, 3620) == PET_ANIM_IDLE);
  pet_anim_trigger(&a, PET_ANIM_PLAY, UINT32_MAX - 100);
  assert(pet_anim_select(&a, false, false, 50) == PET_ANIM_PLAY);
  assert(pet_anim_frame(&a, PET_ANIM_PLAY, 50) == 0);
  assert(pet_anim_select(&a, false, true, 10000) == PET_ANIM_PLAY);
  assert(pet_anim_select(&a, false, false, 10000) == PET_ANIM_IDLE);
  puts("animation: transitions, loop frames, expiration, sleep priority, "
       "active game and timer wrap passed");
}
