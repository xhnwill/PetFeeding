#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef enum {
  PET_ANIM_IDLE,
  PET_ANIM_SLEEP,
  PET_ANIM_EAT,
  PET_ANIM_PLAY
} pet_anim_action_t;
typedef struct {
  uint32_t started_ms, duration_ms;
  pet_anim_action_t transient;
} pet_anim_t;
void pet_anim_trigger(pet_anim_t *animation, pet_anim_action_t action,
                      uint32_t now_ms);
pet_anim_action_t pet_anim_select(const pet_anim_t *animation, bool sleeping,
                                  bool playing, uint32_t now_ms);
unsigned pet_anim_frame(const pet_anim_t *animation, pet_anim_action_t action,
                        uint32_t now_ms);
const char *pet_anim_name(pet_anim_action_t action);
