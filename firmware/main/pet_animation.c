#include "pet_animation.h"
void pet_anim_trigger(pet_anim_t *a, pet_anim_action_t action, uint32_t now) {
  a->transient = action;
  a->started_ms = now;
  a->duration_ms = action == PET_ANIM_EAT    ? 3520
                   : action == PET_ANIM_PLAY ? 2880
                                             : 0;
}
pet_anim_action_t pet_anim_select(const pet_anim_t *a, bool sleeping,
                                  bool playing, uint32_t now) {
  if (sleeping)
    return PET_ANIM_SLEEP;
  if (playing)
    return PET_ANIM_PLAY;
  if ((uint32_t)(now - a->started_ms) < a->duration_ms)
    return a->transient;
  return PET_ANIM_IDLE;
}
unsigned pet_anim_frame(const pet_anim_t *a, pet_anim_action_t action,
                        uint32_t now) {
  static const unsigned duration[] = {1000, 700, 220, 180};
  if (action == PET_ANIM_IDLE)
    return 0;
  uint32_t elapsed =
      action == PET_ANIM_SLEEP ? now : (uint32_t)(now - a->started_ms);
  return (elapsed / duration[action]) % 4;
}
const char *pet_anim_name(pet_anim_action_t action) {
  static const char *names[] = {"idle", "sleep", "eat", "play"};
  return names[action];
}
