#include <eadk.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define GROUND_Y 180

#define MAX_ZOMBIES 10
#define MAX_OBSTACLES 3
#define MAX_CIVILIANS 3

typedef struct {
  float y;
  float vy;
  bool is_jumping;
} Zombie;

typedef struct {
  float x;
  int type; // 0 = Piège/Bombe, 1 = Trou
  int width;
  bool active;
} Obstacle;

typedef struct {
  float x;
  bool active;
} Civilian;

const eadk_color_t COLOR_GREEN = 0x3E10;  // Vert Zombie
const eadk_color_t COLOR_RED = 0xE800;    // Obstacle
const eadk_color_t COLOR_BLUE = 0x1B5F;   // Civil
const eadk_color_t COLOR_GROUND = 0x7BE0; // Sol
const eadk_color_t COLOR_SKY = 0xC679;    // Ciel

const char eadk_app_name[] eadk_app_attribute = "Zombie Tsunami";
const uint32_t eadk_api_level eadk_app_attribute = 0;

void draw_game(Zombie zombies[], int count, Obstacle obstacles[], Civilian civilians[], int score) {
  // Fond & Ciel
  eadk_display_push_rect_uniform((eadk_rect_t){0, 0, SCREEN_WIDTH, GROUND_Y}, COLOR_SKY);
  // Sol
  eadk_display_push_rect_uniform((eadk_rect_t){0, GROUND_Y, SCREEN_WIDTH, SCREEN_HEIGHT - GROUND_Y}, COLOR_GROUND);

  // Dessin des civils
  for (int i = 0; i < MAX_CIVILIANS; i++) {
    if (civilians[i].active) {
      eadk_display_push_rect_uniform((eadk_rect_t){(int)civilians[i].x, GROUND_Y - 20, 12, 20}, COLOR_BLUE);
    }
  }

  // Dessin des obstacles
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacles[i].active) {
      if (obstacles[i].type == 0) { // Bombe / Caisse
        eadk_display_push_rect_uniform((eadk_rect_t){(int)obstacles[i].x, GROUND_Y - 18, obstacles[i].width, 18}, COLOR_RED);
      } else { // Trou
        eadk_display_push_rect_uniform((eadk_rect_t){(int)obstacles[i].x, GROUND_Y, obstacles[i].width, SCREEN_HEIGHT - GROUND_Y}, COLOR_SKY);
      }
    }
  }

  // Dessin de la horde de zombies
  for (int i = 0; i < count; i++) {
    int z_x = 40 - (i * 12);
    if (z_x >= 0) {
      eadk_display_push_rect_uniform((eadk_rect_t){z_x, (int)zombies[i].y, 14, 20}, COLOR_GREEN);
    }
  }

  // Score
  char score_str[32];
  snprintf(score_str, sizeof(score_str), "Zombies: %d  Score: %d", count, score);
  eadk_display_draw_string(score_str, (eadk_point_t){10, 10}, false, eadk_color_black, COLOR_SKY);
}

int main(int argc, char *argv[]) {
  int zombie_count = 1;
  Zombie zombies[MAX_ZOMBIES];
  for (int i = 0; i < MAX_ZOMBIES; i++) {
    zombies[i].y = GROUND_Y - 20;
    zombies[i].vy = 0;
    zombies[i].is_jumping = false;
  }

  Obstacle obstacles[MAX_OBSTACLES] = {0};
  Civilian civilians[MAX_CIVILIANS] = {0};

  int score = 0;
  float speed = 3.5f;

  while (zombie_count > 0) {
    eadk_keyboard_state_t kbd = eadk_keyboard_scan();

    // Saut de la horde (Touche OK ou EXE)
    bool jump_pressed = eadk_keyboard_key_down(kbd, eadk_key_ok) || eadk_keyboard_key_down(kbd, eadk_key_exe);
    if (jump_pressed && !zombies[0].is_jumping) {
      for (int i = 0; i < zombie_count; i++) {
        zombies[i].vy = -8.5f;
        zombies[i].is_jumping = true;
      }
    }

    // Physique des zombies
    for (int i = 0; i < zombie_count; i++) {
      if (zombies[i].is_jumping) {
        zombies[i].y += zombies[i].vy;
        zombies[i].vy += 0.5f; // Gravité

        if (zombies[i].y >= GROUND_Y - 20) {
          zombies[i].y = GROUND_Y - 20;
          zombies[i].is_jumping = false;
          zombies[i].vy = 0;
        }
      }
    }

    // Spawn obstacles
    if (rand() % 80 == 0) {
      for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) {
          obstacles[i].x = SCREEN_WIDTH;
          obstacles[i].type = rand() % 2;
          obstacles[i].width = (obstacles[i].type == 0) ? 16 : 30;
          obstacles[i].active = true;
          break;
        }
      }
    }

    // Spawn civils
    if (rand() % 100 == 0) {
      for (int i = 0; i < MAX_CIVILIANS; i++) {
        if (!civilians[i].active) {
          civilians[i].x = SCREEN_WIDTH;
          civilians[i].active = true;
          break;
        }
      }
    }

    // Mise à jour des civils
    for (int i = 0; i < MAX_CIVILIANS; i++) {
      if (civilians[i].active) {
        civilians[i].x -= speed;
        // Collision avec le zombie de tête
        if (civilians[i].x <= 54 && civilians[i].x >= 30 && zombies[0].y >= GROUND_Y - 30) {
          civilians[i].active = false;
          score += 10;
          if (zombie_count < MAX_ZOMBIES) zombie_count++;
        }
        if (civilians[i].x < -20) civilians[i].active = false;
      }
    }

    // Mise à jour des obstacles
    for (int i = 0; i < MAX_OBSTACLES; i++) {
      if (obstacles[i].active) {
        obstacles[i].x -= speed;

        // Collision
        if (obstacles[i].x <= 54 && obstacles[i].x + obstacles[i].width >= 20) {
          if (obstacles[i].type == 0 && zombies[0].y >= GROUND_Y - 25) { // Bombe
            obstacles[i].active = false;
            zombie_count--;
          } else if (obstacles[i].type == 1 && zombies[0].y >= GROUND_Y - 20) { // Trou
            zombie_count = 0;
          }
        }

        if (obstacles[i].x < -40) obstacles[i].active = false;
      }
    }

    draw_game(zombies, zombie_count, obstacles, civilians, score);
    eadk_timing_msleep(20);
  }

  // Game Over
  eadk_display_push_rect_uniform((eadk_rect_t){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, eadk_color_black);
  eadk_display_draw_string("GAME OVER", (eadk_point_t){110, 110}, false, eadk_color_white, eadk_color_black);
  eadk_timing_msleep(3000);

  return 0;
}
