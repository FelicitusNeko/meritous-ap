//
//   levelblit.c
//
//   Copyright 2007, 2008 Lancer-X/ASCEAI
//
//   This file is part of Meritous.
//
//   Meritous is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Meritous is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Meritous.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <SDL.h>
#include <SDL_image.h>
#include <assert.h>

#include "mapgen.h"
#include "demon.h"
#include "gamemap.h"
#include "tiles.h"
#include "save.h"
#include "help.h"
#include "audio.h"
#include "boss.h"
#include "ending.h"
#include "itemhandler.h"
#include "stats.h"

// this is here to make VSCodium behave
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define PLAYERW 16
#define PLAYERH 24

#define MERITOUS_VERSION "v 1.4"

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

int RECORDING = 0;
int PLAYBACK = 0;

int expired_ms = 0;
int frame_len = 33;
int WriteBitmaps = 0;
int WB_StartRange = 0;
int WB_EndRange = 1000000;
int training = 0;
int game_paused = 0;
int show_ending = 0;
int voluntary_exit = 0;
int tele_select = 0;
int enter_room_x = 0, enter_room_y = 0;

int agate_knife_loc = -1;

FILE *record_file;
char record_filename[256];

void DrawLevel(int off_x, int off_y, int hide_not_visited, int fog_of_war);
void DrawPlayer(int x, int y, int pl_dir, int pl_frm);
void LoadLevel();
void ActivateRoom(int room);

void DrawCircuit();
void ReleaseCircuit();
void DrawCircle(int x, int y, int r, unsigned char c);

void DrawArtifacts();

void HandleEvents();

void text_init();
void draw_text(int x, int y, const char *str, Uint8 tcol);
unsigned char font_data[128][8][8];

void DrawShield();

void HandleGamepad (int key_held[]);

int key_held[10] = {0};
int menu_key_held[10] = { 0 };
int game_running = 1;

char typed_key = ' ';
typedef enum menuScreens {
  MENU_APENABLED,
  MENU_GAMEMODE,
  MENU_CONNECTION,
} t_menuScreens;
t_menuScreens menu_screen = MENU_APENABLED;

int player_x;
int player_y;
int player_dying;
int magic_circuit;
int circuit_range;
int release_range;
int release_x;
int release_y;
int release_str;
char release_hit_something;

int shield_hp;
int shield_recover;
int player_gems;
int checkpoints_found;
int circuit_size;
int first_game;
int player_hp;
int player_lives = 5;
int player_lives_part = 0;

int player_room;
int player_dir;
int player_wlk;
int player_walk_speed;
int wlk_wait;
int circuit_release;
int scroll_home;
int enter_pressed;

int opening_door_x, opening_door_y, opening_door_i = 0, opening_door_n;

int checkpoint_x;
int checkpoint_y;

int explored = 0;
//#define DEBUG_STATS 1

int artifacts[AF_MAXX];
SDL_Surface *artifact_spr = NULL;

int player_shield;
int circuit_fillrate;
int circuit_recoverrate;

int scroll_x, scroll_y;

int map_enabled;

int prv_player_room;

// int specialmessage;
// int specialmessagetimer;

int timer_ps = 0;
int timer_v[10];

typedef struct queued_message t_queued_message;
struct queued_message {
  int msgid;
  int duration;
  int paramcount;
  char **params;
  t_queued_message *next;
};

t_queued_message *msgqueue = NULL;

const char *cache_names[3] = {
  "Alpha",
  "Beta",
  "Gamma"
};

const char *photo_warning[] = {
  "W A R N I N G",
  "",
  "This game contains flashing imagery",
  "which may cause issues to photosensitive",
  "people. Play at your discretion.",
  "If you are streaming, consider providing",
  "viewers a warning as well.",
  "",
  "ENTER to begin, ESC to end"
};

float RandomDir()
{
  return (float)(rand()%256)*M_PI*2.0/256.0;
}

int UpgradePrice(int t);

void DisposeFrontMessage();

void PlayerDefaultStats()
{
  int i;

  player_dying = 0;
  magic_circuit = 0;
  circuit_range = 100;
  release_range = 100;
  shield_hp = 0;
  shield_recover = 0;
  player_gems = 0;
  checkpoints_found = 0;
  circuit_size = 1000;
  first_game = 1;
  player_hp = 3;

  explored = 0;

  voluntary_exit = 0;
  player_room = 0;
  player_dir = 0;
  player_wlk = 0;
  player_walk_speed = 5;
  player_lives = 5;
  player_lives_part = 0;
  wlk_wait = 8;
  circuit_release = 0;
  scroll_home = 0;
  enter_pressed = 0;
  show_ending = 0;

  game_paused = 0;

  player_shield = 0;
  circuit_fillrate = 2;
  circuit_recoverrate = 3;

  prv_player_room = -1;

  // specialmessage = 0;
  // specialmessagetimer = 0;

  opening_door_i = 0;

  map_enabled = 0;

  for (i = 0; i < AF_MAXX; i++) {
    artifacts[i] = 0;
  }

  #ifdef DEBUG_STATS

  player_shield = 24;
  circuit_fillrate = 24;
  circuit_recoverrate = 24;

  for (i = 0; i < AF_MAXX; i++) {
    artifacts[i] = 1;
  }

  #endif
}


void ScrollTo(int x, int y);
#define K_UP 0
#define K_DN 1
#define K_LT 2
#define K_RT 3
#define K_SP 4
#define K_c 5
#define K_BS 6

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Surface *screen;
SDL_Texture *texture;
SDL_Color pal[256];

void SetGreyscalePalette();
void SetTonedPalette(float pct);
void SetTitlePalette(int curve_start, int curve_end);
void SetTitlePalette2(int t);
int TouchTile(int ix, int iy);
void SpecialTile(int x, int y);
void DrawRect(int x, int y, int w, int h, unsigned char c);

void DrawCircleEx(int x, int y, int r, int r2, unsigned char c);

void ThinLine(SDL_Surface *scr, int x1, int y1, int x2, int y2, Uint8 col);
void LockDoors(int r);

void HandleTitleScreen();

#define SCREEN_W 640
#define SCREEN_H 480

//Almost certainly a better way to do this using shaders, but this functions right now
void ApplyPalette()
{
  Uint32* pixels = NULL;
  Uint32* sPixels = (Uint32 *)screen->pixels;
  int pitch = 0;
  int format;
  int i;
  int oldpix;
  SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);
  for(i = 0; i < SCREEN_W*SCREEN_H; i++)
  {
    oldpix = (sPixels[i] >> 8) & 0xFF;
    pixels[i] = SDL_MapRGBA(screen->format, pal[oldpix].r, pal[oldpix].g, pal[oldpix].b, 255);
  }
  SDL_UnlockTexture(texture);
}

void VideoUpdate()
{
  static int bmp = 0;
  char bmp_name[256];

  SDL_UpdateTexture(texture, NULL, screen->pixels, screen->pitch);
  ApplyPalette();
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
  SDL_RenderClear(renderer);
  //SDL_UpdateRect(screen, 0, 0, 0, 0);
  if (WriteBitmaps) {
    if ((bmp >= WB_StartRange)&&(bmp < WB_EndRange)) {
      sprintf(bmp_name, "v/bmp%d.bmp", bmp);
      SDL_SaveBMP(screen, bmp_name);
    }
    bmp++;
  }
}

void EndCycle(int n)
{
  static int last_ticks;
  int tick_delta;
  tick_delta = SDL_GetTicks() - last_ticks;

  if (n == 0) n = frame_len;

  if (tick_delta < n) {
    SDL_Delay(n-tick_delta);
  }

  if (!game_paused) expired_ms += n;

  last_ticks = SDL_GetTicks();
}

void WritePlayerData()
{
  int i;

  FWInt(expired_ms);
  FWInt(checkpoint_x);
  FWInt(checkpoint_y);
  FWInt(scroll_x);
  FWInt(scroll_y);
  FWInt(magic_circuit);
  FWInt(checkpoint_x);
  FWInt(checkpoint_y);
  FWInt(player_walk_speed);
  FWInt(wlk_wait);
  FWInt(circuit_fillrate);
  FWInt(circuit_recoverrate);
  FWInt(explored);
  FWInt(player_shield);
  FWInt(shield_recover);
  FWInt(shield_hp);
  FWInt(player_gems);
  FWInt(checkpoints_found);
  FWInt(player_hp);
  FWInt(player_lives);
  FWInt(player_lives_part);
  FWInt(current_boss);
  FWInt(training);
  FWInt(enemy_evolutions);
  FWInt(agate_knife_loc);

  for (i = 0; i < AF_MAXX; i++) {
    FWChar(artifacts[i]);
  }
}

void ReadPlayerData()
{
  int i;

  expired_ms = FRInt();
  player_x = FRInt();
  player_y = FRInt();
  scroll_x = FRInt();
  scroll_y = FRInt();
  magic_circuit = FRInt();
  checkpoint_x = FRInt();
  checkpoint_y = FRInt();
  player_walk_speed = FRInt();
  wlk_wait = FRInt();
  circuit_fillrate = FRInt();
  circuit_recoverrate = FRInt();
  explored = FRInt();
  player_shield = FRInt();
  shield_recover = FRInt();
  shield_hp = FRInt();
  player_gems = FRInt();
  checkpoints_found = FRInt();
  player_hp = FRInt();
  player_lives = FRInt();
  player_lives_part = FRInt();
  current_boss = FRInt();
  training = FRInt();
  enemy_evolutions = FRInt();

  agate_knife_loc = FRInt();

  for (i = 0; i < AF_MAXX; i++) {
    artifacts[i] = FRChar();
  }

  player_lives = MAX(player_lives, training ? 5 : 3);
}

void DummyEventPoll()
{
  SDL_Event e;
  SDL_PollEvent(&e);
}

int DungeonPlay(const int continueGame);

Uint8 Uint8_Bound(int c)
{
  if (c<0) return 0;
  if (c>255) return 255;
  return c;
}

int dist(int x1, int y1, int x2, int y2)
{
  int dx, dy;
  dx = x2 - x1;
  dy = y2 - y1;

  return sqrt((dx*dx)+(dy*dy));
}

void ClearInput()
{
  key_held[K_SP] = 0;
  key_held[K_UP] = 0;
  key_held[K_DN] = 0;
  key_held[K_LT] = 0;
  key_held[K_RT] = 0;
  key_held[K_c] = 0;
  key_held[K_BS] = 0;
  menu_key_held[K_UP] = 0;
  menu_key_held[K_DN] = 0;
  typed_key = ' ';
}

int main(int argc, char **argv)
{
  int on_title = 1;
  int executable_running = 1;
  SDL_Surface *asceai;
  SDL_Surface *wm_icon;
  Uint8 wm_mask[128];
  int i;
  //int light = 0;
  int x, y;
  int option = 0;

  int fullscreen = 0;
  unsigned int stime = 0;

  FILE *wm_mask_file;

  if (argc > 1) {
    for (i = 1; i < argc; i++) {
      if (!strcasecmp(argv[i], "fullscreen")) {
        fullscreen = 1;
      }
/*			if (!strcasecmp(argv[i], "record")) {
        RECORDING = 1;
        strcpy(record_filename, argv[i+1]);
      }
      if (!strcasecmp(argv[i], "play")) {
        PLAYBACK = 1;
        strcpy(record_filename, argv[i+1]);
      }
      if (!strcasecmp(argv[i], "framedelay")) {
        frame_len = atoi(argv[i+1]);
      }
      if (!strcasecmp(argv[i], "bmpwrite")) {
        WriteBitmaps = 1;
      }
      if (!strcasecmp(argv[i], "bmpstart")) {
        WB_StartRange = atoi(argv[i+1]);
      }
      if (!strcasecmp(argv[i], "bmpend")) {
        WB_EndRange = atoi(argv[i+1]);
      } */
    }
  }

  if ((RECORDING) && (PLAYBACK)) {
    exit(1);
  }
  srand(RetrieveSeed());
  if (RECORDING) {
    record_file = fopen(record_filename, "wb");
    if (record_file == NULL) {
      perror(record_filename);
    }
    else {
      stime = time(NULL);

      fputc(stime & 0x000000FF, record_file);
      fputc((stime & 0x0000FF00) >> 8, record_file);
      fputc((stime & 0x00FF0000) >> 16, record_file);
      fputc((stime & 0xFF000000) >> 24, record_file);

      srand(stime);
    }
  }
  if (PLAYBACK) {
    record_file = fopen(record_filename, "rb");
    if (record_file == NULL) {
      perror(record_filename);
    }
    else {
      stime = fgetc(record_file);
      stime |= fgetc(record_file) << 8;
      stime |= fgetc(record_file) << 16;
      stime |= fgetc(record_file) << 24;

      srand(stime);
    }
  }

  SDL_Init(SDL_INIT_EVERYTHING);
  asceai = IMG_Load("dat/i/asceai.png");
  wm_icon = IMG_Load("dat/i/icon.png");

  window = SDL_CreateWindow("~ m e r i t o u s ~ g a i d e n ~", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  renderer = SDL_CreateRenderer(window, -1, 0);
  SDL_RenderSetLogicalSize(renderer,640, 480);
  //screen = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 8, SDL_SWSURFACE | (SDL_FULLSCREEN * fullscreen));
  screen = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_W, SCREEN_H, 8, SDL_PIXELFORMAT_RGBA8888);
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 640, 480);

  // const char* icon_filename = "dat/d/icon_bitmask.dat";
  // wm_mask_file = fopen(icon_filename, "rb");
  // if (wm_mask_file == NULL) {
  //   perror (icon_filename);
  //   exit(1);
  // }
  // int wm_mask_ok = fread(wm_mask, 1, 128, wm_mask_file);
  // fclose(wm_mask_file);
  //SDL_WM_SetCaption("~ m e r i t o u s ~ g a i d e n ~", "MT");
  //if (wm_mask_ok)
    SDL_SetWindowIcon(window, wm_icon);
    //SDL_WM_SetIcon(wm_icon, wm_mask);
  SDL_ShowCursor(SDL_DISABLE);
  InitAudio();

  text_init();

  SetGreyscalePalette();

  int warning_length = sizeof(photo_warning) / sizeof(*photo_warning);

  for (size_t x = 0; x < warning_length; x++) {
    draw_text((SCREEN_W / 2) - (strlen(photo_warning[x]) * 4),
              (SCREEN_H / 2) - (warning_length * 5) + (x * 10),
              photo_warning[x], 255);
  }
  VideoUpdate();

  while (warning_length) {
    HandleEvents();

    if (enter_pressed) warning_length = 0;
    if (voluntary_exit) warning_length = executable_running = 0;

    EndCycle(20);
  }

  if (executable_running) {
    // asceai logo
    SDL_BlitSurface(asceai, NULL, screen, NULL);

    for (i = 0; i < 75; i++) {
      SetTitlePalette(i * 5 - 375, i * 5 - 120);
      VideoUpdate();
      DummyEventPoll();
      EndCycle(20);
    }
    SDL_Delay(500);
    for (i = 0; i < 50; i++) {
      SetTitlePalette(i * 5, 255 - (i * 5));
      VideoUpdate();
      DummyEventPoll();
      EndCycle(20);
    }
    SDL_Delay(500);
    for (i = 0; i < 50; i++) {
      SetTitlePalette(255, (i * 5)+5);
      VideoUpdate();
      DummyEventPoll();
      EndCycle(20);
    }

    InitRando();
  }

  while (executable_running) {
    TitleScreenMusic();

    //if (IsSaveFile()) {
    //  can_continue = 1;
    //} else {
    //  can_continue = 0;
    //}

    //maxoptions = 2 + can_continue;
    // TODO: clean up
    int can_continue = 1;

    HandleTitleScreen();

    ClearInput();

    if (executable_running == 1) {
      if ((option == 0) && can_continue) {
        DungeonPlay(1);
      } else {
        if (option == (0 + can_continue)) {
          training = 0;
          DungeonPlay(0);
        } else {
          training = 1;
          DungeonPlay(0);
        }
      }
      // clean up
      ClearInput();
      DestroyDungeon();
      DestroyThings();
      DestroyStores();
      on_title = 1;
      game_load = 0;

      game_running = 1;
    }
  }

//	if (argc >= 2) DungeonPlay(argv[1]);
//	else DungeonPlay("");
  SDL_FreeSurface(screen);
  SDL_DestroyTexture(texture);
  SDL_DestroyWindow(window);
  SDL_DestroyRenderer(renderer);
  SDL_Quit();
  return 0;
}

void HandleTitleScreen() {
  int on_title = 1;
  int ticker_tick = 0;
  int tick = 10000000;
  int precalc_sine[400];
  int can_continue;
  int pulse[SCREEN_W * SCREEN_H];
  int i, x, y;
  int option = 0;
  int maxoptions = 2;
  int last_key = 0;
  Uint8* src_p, * col_p;
  SDL_Surface *title, *title_pr;

  char url[32] = { '\0' };
  char port[8] = { '\0' };
  char slotname[32] = { '\0' };
  char password[32] = { '\0' };
  int archipelago_menu = 0;
  int connection_arrow_offset = 0;

  strcpy(url, "archipelago.gg");
  strcpy(port, "38281");

  title = IMG_Load("dat/i/title.png");
  title_pr = IMG_Load("dat/i/title.png");
  for (i = 0; i < 400; i++) {
    precalc_sine[i] = sin((float)i / 400 * M_PI * 2) * 24 + 24;
  }

  if (IsSaveFile()) {
    can_continue = 1;
  } else {
    can_continue = 0;
  }

  for (i = 0; i < screen->w * screen->h; i++) {
    x = i % SCREEN_W;
    y = i / SCREEN_W;

    pulse[i] = dist(x, y, SCREEN_W / 2, SCREEN_H / 2);
  }

  while (on_title == 1) {
    SetTitlePalette2(ticker_tick);
    col_p = (Uint8*)title_pr->pixels;
    src_p = (Uint8*)title->pixels;
    if ((tick % 10) == 0) {
      for (i = 0; i < 640 * 480; i++) {
        *(col_p++) = Uint8_Bound(*(src_p++) + precalc_sine[(pulse[i] + tick) % 400]);
      }
    }
    SDL_BlitSurface(title_pr, NULL, screen, NULL);
    draw_text(17, 156, MERITOUS_VERSION, 225 + sin((float)ticker_tick / 15) * 30);
    if (isArchipelago()) draw_text(0, 450, "AP Enabled", 255);

    // new start menu
    if (menu_screen == MENU_APENABLED) {
      maxoptions = 2;

      draw_text((SCREEN_W - 14 * 8) / 2, 310, "Archipelago", 255);
      draw_text((SCREEN_W - 14 * 8) / 2, 320, "Local randomiser", 255);

    }

    // main title screen
    else if (menu_screen == MENU_GAMEMODE) {

      if (can_continue) {
        draw_text((SCREEN_W - 14 * 8) / 2, 310, "Continue", 255);
        maxoptions = 3;
      }
      else {
        maxoptions = 2;
      }

      draw_text((SCREEN_W - 14 * 8) / 2, 310 + can_continue * 10, "New Game", 255);
      draw_text((SCREEN_W - 14 * 8) / 2, 320 + can_continue * 10, "New Game (Training mode)", 255);

    }
    // archipelago
    else if (menu_screen == MENU_CONNECTION) {
      maxoptions = 7;

      draw_text((SCREEN_W - 24 * 8) / 2, 310, "URL", 255);
      draw_text((SCREEN_W - 24 * 8) / 2, 320, "Port", 255);
      draw_text((SCREEN_W - 24 * 8) / 2, 330, "Slotname", 255);
      draw_text((SCREEN_W - 24 * 8) / 2, 340, "Password", 255);

      draw_text((SCREEN_W - 24 * 8) / 2, 360, "Connect", 255);
      draw_text((SCREEN_W - 24 * 8) / 2, 370, "Cancel", 255);

      draw_text((SCREEN_W + 2 * 8) / 2, 310, url, 255);
      draw_text((SCREEN_W + 2 * 8) / 2, 320, port, 255);
      draw_text((SCREEN_W + 2 * 8) / 2, 330, slotname, 255);

      int pwd_len = strlen(password);
      char* asterisks = malloc(pwd_len + 1);
      memset(asterisks, '*', pwd_len);
      asterisks[pwd_len] = '\0';
      draw_text((SCREEN_W + 2 * 8) / 2, 340, asterisks, 255);
    }


    if (ticker_tick >= 30) {
      connection_arrow_offset = menu_screen == MENU_CONNECTION ? 1 : 0;
      draw_text((SCREEN_W - (14 + connection_arrow_offset * 10) * 8) / 2 - 17, 310 + option * 10, "-", 205 + sin((float)ticker_tick / 5.0) * 24);
      draw_text((SCREEN_W - (14 + connection_arrow_offset * 10) * 8) / 2 - 20, 310 + option * 10, " >", 205 + sin((float)ticker_tick / 5.0) * 24);
      draw_text((SCREEN_W - (14 + connection_arrow_offset * 10) * 8) / 2 - 19, 310 + option * 10, " >", 190 + sin((float)ticker_tick / 5.0) * 24);
      draw_text((SCREEN_W - (14 + connection_arrow_offset * 10) * 8) / 2 - 21, 310 + option * 10, " >", 190 + sin((float)ticker_tick / 5.0) * 24);
      draw_text((SCREEN_W - (14 + connection_arrow_offset * 10) * 8) / 2 - 18, 310 + option * 10, " >", 165 + sin((float)ticker_tick / 5.0) * 24);
      draw_text((SCREEN_W - (14 + connection_arrow_offset * 10) * 8) / 2 - 22, 310 + option * 10, " >", 165 + sin((float)ticker_tick / 5.0) * 24);
    }

    VideoUpdate();

    if (ticker_tick++ > 30) {
      HandleEvents();

      // archipelago vs local randomiser
      if (menu_screen == MENU_APENABLED) {
        if (key_held[K_UP]) {
          if (last_key != 1)
            if (option > 0) option--;
          last_key = 1;
        }
        else {
          if (key_held[K_DN]) {
            if (last_key != 2)
              if (option < maxoptions - 1) option++;
            last_key = 2;
          }
          else {
            last_key = 0;
            if (key_held[K_SP] || enter_pressed) {
              if (option == 0) {
                menu_screen = MENU_CONNECTION;
                archipelago_menu = 1;
              }
              else if (option == 1) {
                menu_screen = MENU_GAMEMODE;
                archipelago_menu = 0;
              }
            }
          }
        }

        if (voluntary_exit) {
          //executable_running = 0;
          on_title = 0;
          SDL_Quit();
          exit(0);
        }
      }

      // main title screen
      else if (menu_screen == MENU_GAMEMODE) {
        if (key_held[K_UP]) {
          if (last_key != 1)
            if (option > 0) option--;
          last_key = 1;
        }
        else {
          if (key_held[K_DN]) {
            if (last_key != 2)
              if (option < maxoptions - 1) option++;
            last_key = 2;
          }
          else {
            last_key = 0;
            if (key_held[K_SP] || enter_pressed) {
              on_title = 0;
            }
          }
        }

        if (voluntary_exit) {
          //executable_running = 0;
          voluntary_exit = 0;
          if (archipelago_menu) menu_screen = MENU_CONNECTION;
          else menu_screen = MENU_APENABLED;
          option = 0;
        }
      }

      // archipelago
      else if (menu_screen == MENU_CONNECTION) {
        if (menu_key_held[K_UP]) {
          if (last_key != 1)
            if (option > 0) {
              option--;
              if (option == 4) option--;
            }
          last_key = 1;
        }
        else if (menu_key_held[K_DN]) {
          if (last_key != 2)
            if (option < maxoptions - 1) {
              option++;
              if (option == 4) option++;
            }
          last_key = 2;
        }
        else if (key_held[K_BS]) {
          if (last_key != K_BS) {
            if (option == 0) url[strlen(url) - 1] = '\0';
            else if (option == 1) port[strlen(port) - 1] = '\0';
            else if (option == 2) slotname[strlen(slotname) - 1] = '\0';
            else if (option == 3) password[strlen(password) - 1] = '\0';
            last_key = K_BS;
          }
        }
        else if (typed_key != ' ') {
          if (option == 0 && strlen(url) < 31) url[strlen(url)] = typed_key;
          else if (option == 1 && strlen(port) < 7) port[strlen(port)] = typed_key;
          else if (option == 2 && strlen(slotname) < 31) slotname[strlen(slotname)] = typed_key;
          else if (option == 3 && strlen(password) < 31) password[strlen(password)] = typed_key;
          typed_key = ' ';
        }
        else {
          last_key = 0;
          if (key_held[K_SP] || enter_pressed) {
            if (option == 5) {
              menu_screen = MENU_GAMEMODE;
              option = 0;
            }
            else if (option == 6) {
              menu_screen = MENU_APENABLED;
              option = 0;
            }
          }
        }

        if (voluntary_exit) {
          voluntary_exit = 0;
          menu_screen = MENU_APENABLED;
          option = 0;
        }
      }
    }

    EndCycle(10);

    //light = 0;
    tick -= 2;
  }
  SDL_FreeSurface(title);
  SDL_FreeSurface(title_pr);
}

void DrawMeter(int x, int y, int n)
{
  static SDL_Surface *meter = NULL;
  SDL_Rect drawfrom, drawto;
  if (meter == NULL) {
    meter = IMG_Load("dat/i/meter.png");
    SDL_SetColorKey(meter, SDL_TRUE | SDL_RLEACCEL, 0);
  }

  drawfrom.x = 0;
  drawfrom.y = 6;
  drawfrom.w = 150;
  drawfrom.h = 6;

  drawto.x = x;
  drawto.y = y;

  SDL_BlitSurface(meter, &drawfrom, screen, &drawto);

  drawfrom.w = n*6;
  drawfrom.y = 0;

  SDL_BlitSurface(meter, &drawfrom, screen, &drawto);
}

void ProgressBarScreen(int part, float progress, const char *message, float t_parts)
{
  memset(screen->pixels, 0, 640*480*4);

  DrawRect(200, 217, 240, 50, 80);
  DrawRect(202, 219, 236, 46, 20);
  draw_text(232, 228, message, 255);
  DrawRect(232, 244, 176, 12, 128);
  DrawRect(234, 246, 172, 8, 0);

  if ((int)(172.0 * progress / t_parts + (172.0 / t_parts * part)) > 0) {
    DrawRect(234, 246, (int)(172.0 * progress / t_parts + (172.0 / t_parts * part)), 8, 200);
  }
  VideoUpdate();
  DummyEventPoll();
}

void LoadingScreen(int part, float progress)
{
  float t_parts;

  if (game_load) t_parts = 6.0;
  else t_parts = 3.0;

  ProgressBarScreen(part, progress, "Loading... please wait", t_parts);
  ClearInput();
}

void SavingScreen(int part, float progress)
{
  ProgressBarScreen(part, progress, "Saving... please wait", 5.0);
  ClearInput();
}

void Arc(SDL_Surface *s, int x, int y, int r, float dir)
{
  int bright;
  int i, c;
  float pdir, cdir, ndir;

  int l_x = x, l_y = y;
  int cx, cy, c1x, c1y, c2x, c2y;

  bright = rand()%128+63;
  i = 0;
  while (i < r) {
    i += rand()%5+25;
    pdir = dir + (float)(rand()%16)/16.0*2.0*(M_PI / 15.0);
    ndir = dir - (float)(rand()%16)/16.0*2.0*(M_PI / 15.0);
    cdir = dir + (float)(rand()%16)/16.0*2.0*(M_PI / 20.0) - (float)(rand()%16)/16.0*2.0*(M_PI / 20.0);

    bright += rand()%30;
    bright -= rand()%30;

    if (bright < 0) bright = 0;
    if (bright > 255) bright = 255;

    c1x = x + cos(pdir) * i;
    c1y = y + sin(pdir) * i;
    ThinLine(s, l_x, l_y, c1x, c1y, bright);
    c2x = x + cos(ndir) * i;
    c2y = y + sin(ndir) * i;
    ThinLine(s, l_x, l_y, c2x, c2y, bright);

    for (c = 0; c < 5; c++) {
      DrawRect(x + cos(dir - (M_PI / 10.0) + (float)(rand()%16)/16.0*2.0*(M_PI / 10.0)) * i, y + sin(dir - (M_PI / 10.0) +
            (float)(rand()%16)/16.0*2.0*(M_PI / 10.0)) * i, 1, 1, rand()%128+63);
    }

    i += rand()%5+25;
    cx = x + cos(cdir) * i;
    cy = y + sin(cdir) * i;
    ThinLine(s, c1x, c1y, cx, cy, bright);
    ThinLine(s, c2x, c2y, cx, cy, bright);
    l_x = cx;
    l_y = cy;
  }

}

void ComposeTime(char *ptr, int msec, char displayMsec) {
  int t_days = msec / (1000*60*60*24);
  int t_hours = (msec / (1000*60*60)) % 24;
  int t_minutes = (msec / (1000*60)) % 60;
  int t_seconds = (msec / 1000) % 60;
  int t_msec = msec % 1000;

  char buf[16] = {0};

  if (displayMsec) sprintf(buf, "%dm %d.%03ds", t_minutes, t_seconds, t_msec);
  else sprintf(buf, "%dm %ds", t_minutes, t_seconds);

  if (t_days > 0) sprintf(ptr, "%dd %dh %s", t_days, t_hours, buf);
  else if (t_hours > 0) sprintf(ptr, "%dh %s", t_hours, buf);
  else sprintf(ptr, buf);
}

int DungeonPlay(const int continueGame)
{
  int ix,  iy;
  int off_x, off_y;
  int t = 0;
  int i, j;
  int lost_gems;
  int rg_x, rg_y, rg_v;
  int max_dist;
  int last_killed = 0;
  int n_arcs = 0;
  int can_move;

  float arcdir;

  char buf[50];

  expired_ms = 0;
  LoadingScreen(0, 0.0);
  if (continueGame) {
    LoadGame();
  }

  RandomGenerateMap();
  InitEnemies();
  InitBossVars();

  PlayerDefaultStats();
  init_stats();
  while (msgqueue != NULL) DisposeFrontMessage();

  if (game_load) {
    first_game = 0;
    ReadPlayerData();
    ReadStoreData();
    ReadStatsData();
    //Paint(rooms[0].x+1, rooms[0].y+1, rooms[0].w-2, rooms[0].h-2, "dat/d/fbossroom.loc");
  } else {
    player_x = map.w * 32 / 2 - PLAYERW/2;
    player_y = map.h * 32 / 2 - PLAYERH/2;
  }

  InitAutomap();

  if (game_load) CloseFile();

  max_dist = 0;
  for (i = 0; i < rooms_to_gen; i++) {
    if (rooms[i].s_dist > max_dist) {
      max_dist = rooms[i].s_dist;
    }
  }

  StartRando();

  game_running = 1;
  while (game_running) {
    //sprintf(buf, "X: %d  Y: %d", (player_x + PLAYERW/2)/32*32 + PLAYERW/2, (player_y + PLAYERH/2)/32*32 + PLAYERH/2);
    //SDL_WM_SetCaption(buf, "MT");

    PollAPClient();
    if (!game_paused) {

      if (player_dying > 30) {
        add_int_stat(STAT_DAMAGE_TAKEN, 1);
        player_hp--;

        if (player_hp <= 0) {
          add_int_stat(STAT_LIVES_LOST, 1);
          AnnounceDeath();
          if (!training) player_lives--;
          lost_gems = player_gems / 3;
          add_int_stat(STAT_GEMS_LOST_ON_DEATH, lost_gems);
          player_gems -= lost_gems;

          lost_gems = lost_gems * 95 / 100;
          while (lost_gems > 0) {
            rg_x = rooms[player_room].x * 32 + 32 + rand()%(rooms[player_room].w*32-64);
            rg_y = rooms[player_room].y * 32 + 32 + rand()%(rooms[player_room].h*32-64);
            rg_v = rand() % (lost_gems / 4 + 2);
            CreateGem(rg_x, rg_y, player_room, rg_v);
            lost_gems -= rg_v;
          }

          player_dying = 0;
          shield_hp = 0;

          if ( (current_boss == 3) && (boss_fight_mode != 0) ) {
            player_x = enter_room_x;
            player_y = enter_room_y;
            prv_player_room = 1;
          } else {
            player_x = checkpoint_x;
            player_y = checkpoint_y;
          }
          scroll_home = 1;
          CircuitBullets(player_x, player_y, 100);
          player_hp = 3 + (player_shield == 30)*3;
        } else {
          player_dying = 0;
        }
      }
    }

    circuit_size = 250 + 50*(circuit_fillrate + circuit_recoverrate);

    if (magic_circuit > 0) {
      circuit_range = (sqrt(magic_circuit + 1) * 6 + MIN(magic_circuit / 2, 50))*1.66;
      if (artifacts[AF_CIRCUIT_BOOSTER]) circuit_range += circuit_range / 2.4;
    } else circuit_range = -1;
    player_room = GetRoom(player_x/32, player_y/32);

    if (player_room != prv_player_room) {
      add_int_stat(STAT_ROOM_TRANSITIONS, 1);
      SetTonedPalette((float)rooms[player_room].s_dist / (float)max_dist);
      prv_player_room = player_room;
      RecordRoom(player_room);

      enter_room_x = player_x;
      enter_room_y = player_y;

      if (rooms[player_room].room_type == 2) {
        // lock the doors
        LockDoors(player_room);
        // it's a boss room
        BossRoom(player_room);
      }
      if (((rooms[player_room].checkpoint)||(player_room==0))&&(!artifacts[AF_CURSED_SEAL])) {
        checkpoint_x = rooms[player_room].x * 32 + (rooms[player_room].w / 2 * 32) + 8;
        checkpoint_y = rooms[player_room].y * 32 + (rooms[player_room].h / 2 * 32) + 4;
      }
      if (rooms[player_room].visited == 0) {
        rooms[player_room].visited = 1;
        explored++;

        if (explored == rooms_to_knife) {
          agate_knife_loc = player_room;
        }

        ActivateRoom(player_room);
      }
    }

    if (last_killed != killed_enemies) {
      SetTonedPalette((float)rooms[player_room].s_dist / (float)max_dist);
      last_killed = killed_enemies;
    } else {
      if ((player_room == 0)&&(artifacts[AF_CURSED_SEAL] == 1)) {
        SetTonedPalette(0);
      }
    }

    if (!map_enabled) {
      ScrollTo(player_x + PLAYERW/2 - 320, player_y + PLAYERH/2 - 240);
      DrawLevel(scroll_x, scroll_y, 1, 1);
      //DrawLevel(player_x + 8 - 320, player_y + 12 - 240);

      if (player_dying == 0) {
        DrawShield();

        if (magic_circuit > 0) {
          if (player_dying == 0) {
            if (circuit_release == 0) {
              arcdir = RandomDir();
              n_arcs = 1 + (circuit_size / 200 + 2) * magic_circuit / circuit_size;
              for (i = 0; i < n_arcs; i++) {
                Arc(screen, player_x - scroll_x + PLAYERW/2, player_y - scroll_y + PLAYERH/2, circuit_range, arcdir);
                arcdir += (float)(rand()%16) / 16.0 * (M_PI*2/(float)n_arcs);
              }
            }
          }
        }

        DrawPlayer(312, 228, player_dir, player_wlk / wlk_wait);
      } else {
        if (t % 2 == 0) DrawPlayer(312, 228, player_dir, player_wlk / wlk_wait);

        if (!game_paused)
          player_dying++;
      }
      t++;
      if ((boss_fight_mode != 0)&&(boss_fight_mode < 23)&&(!game_paused)) {
        BossControl();
      }
      DrawEntities();
      if (!game_paused) MoveEntities();

      if (boss_fight_mode == 2) {
        DrawBossHP(100);
      }

      if (rooms[player_room].room_type == 5) {
        DrawPowerObject();
      }
      if ( (rooms[player_room].room_type == 6) && (current_boss == 3) ) {
        DrawPowerObject();
      }
      if ((rooms[player_room].room_type == 4) && ((player_room % 1000) == 999)) {
        DrawPowerObject();
      }
      if (player_room == agate_knife_loc) {
        {
          static float agate_t = 0.0;
          static SDL_Surface *agate_knife = NULL;
          int xpos, ypos;
          int room_w, room_h;
          int room_x, room_y;

          room_x = rooms[player_room].x * 32 + 32;
          room_y = rooms[player_room].y * 32 + 32;
          room_w = rooms[player_room].w * 32 - 64;
          room_h = rooms[player_room].h * 32 - 64;

          SDL_Rect draw_to;
          if (agate_knife == NULL) {
            agate_knife = IMG_Load("dat/i/agate.png");
            SDL_SetColorKey(agate_knife, SDL_TRUE | SDL_RLEACCEL, 0);
          }
          xpos = (int)((sin(agate_t * 1.33)*0.5+0.5) * (float)room_w) + room_x;
          ypos = (int)((cos(agate_t * 0.7)*0.5+0.5) * (float)room_h) + room_y;

          if (dist(player_x, player_y, xpos, ypos) < 20) {
            // NOTE: Agate Knife
            agate_knife_loc = -1;
            CollectSpecialItem(SS_AGATE_KNIFE);
          }
          draw_to.x = xpos - 16 - scroll_x;
          draw_to.y = ypos - 16 - scroll_y;

          SDL_BlitSurface(agate_knife, NULL, screen, &draw_to);

          agate_t += 0.05;
        }
      }

      if (opening_door_i > 0) {
        DrawArtifactOverhead(opening_door_n);
        for (i = 0; i < 5; i++) {
          j = i * 50 - 250 + (opening_door_i * 5);
          if (j > 0) {
            DrawCircle(player_x - scroll_x, player_y - scroll_y, j, 255);
          }
        }

        if (!game_paused) {
          opening_door_i++;
          if (opening_door_i >= 100) {
            opening_door_i = 0;
            Put(opening_door_x, opening_door_y, Get(opening_door_x, opening_door_y) - 38 + 13, GetRoom(opening_door_x, opening_door_y));
          }
        }
      }

      if (circuit_release > 0) {
        DrawCircle(release_x - player_x + 320, release_y - player_y + 240, circuit_release * release_range / 20, sin((float)circuit_release / 20.0)*127+127);
        if (!game_paused) {
          CircuitBullets(release_x, release_y, circuit_release * release_range / 20);
          //HurtEnemies(release_x, release_y, circuit_release * release_range / 20, release_str);
          circuit_release+=2;

          if (circuit_release > 24) {
            circuit_release = 0;
            HurtEnemies(release_x, release_y, release_range, release_str);
            if (boss_fight_mode == 2) TryHurtBoss(release_x, release_y, release_range, release_str);
            if (!release_hit_something) add_int_stat(STAT_WHIFFS, 1);
          }
        }
      }

      if (!game_paused) {
        if (shield_hp < player_shield) {
          shield_recover += player_shield * 3 / (3 - training - (player_shield == 30));
          if (artifacts[AF_SHIELD_BOOST]) shield_recover += player_shield * 3 / (3 - training - (player_shield == 30));
          if (shield_recover >= 50) {
            shield_hp++;
            shield_recover -= 50 - (player_shield == 30)*25;
          }
        }
      }
    }

    DrawRect(0, 0, 640, 29, 0);
    DrawRect(1, 1, 638, 27, 32);
    DrawRect(2, 2, 636, 25, 64);

    if (!tele_select) {
      sprintf(buf, "Psi Crystals: %d", player_gems);
      draw_text(3, 3, buf, 200);
      sprintf(buf, "Explored: %.1f%% (%d/%d rooms)", (float)explored/30.0, explored, rooms_to_gen);
      draw_text(3, 11, buf, 200);
      sprintf(buf, "Cleared: %.1f%% (%d/%d monsters)", (float)killed_enemies/(float)total_enemies*100.0, killed_enemies, total_enemies);
      draw_text(3, 19, buf, 200);

      draw_text(316, 3, "Reflect shield", (player_gems >= UpgradePrice(0))&&(player_shield!=30) ? (231 + (t%13)*2) : 200);
      DrawMeter(434, 3, player_shield);

      draw_text(316, 11, "Circuit charge", (player_gems >= UpgradePrice(1))&&(circuit_fillrate!=30) ? (231 + (t%13)*2) : 200);
      DrawMeter(434, 11, circuit_fillrate);

      draw_text(316, 19, "Circuit refill", (player_gems >= UpgradePrice(2))&&(circuit_recoverrate!=30) ? (231 + (t%13)*2) : 200);
      DrawMeter(434, 19, circuit_recoverrate);

    } else {
      draw_text(80, 11-6, "Use the movement keys to locate a checkpoint. Press ENTER to", 240);
      draw_text(52, 11+6, "teleport to this checkpoint. Press ESCAPE or TAB once you are done.", 240);
    }

    if (!training) {
      buf[0] = 30;

      if (player_lives <= 99) {
        if (player_lives < 10) {
          sprintf(buf+1, " %d", player_lives);
        } else {
          sprintf(buf+1, "%d", player_lives);
        }
      } else {
        sprintf(buf+1, "**");
      }

      draw_text(615, 4, buf, 200);

      DrawRect(615, 13, 24, 4, 240);
      DrawRect(616, 14, 22, 2, 0);
      i = (player_lives_part * 22 / 88);
      if (i > 0) {
        DrawRect(616, 14, i, 2, 160 + (t % 40));
      }
    }

    if (player_shield != 30) {
      for (i = 0; i < player_hp; i++) {
        buf[i] = 3;
      }
      buf[player_hp]=0;
    } else {
      for (i = 0; i < (player_hp / 2); i++) {
        buf[i] = 3;
      }
      if ((player_hp % 2) == 1) {
        buf[(player_hp + 1) / 2 - 1] = 2;
      }
      buf[(player_hp+1)/2]=0;
    }

    draw_text(615, 18 - (5*training), buf, 200);

    DrawRect(0, 466, 640, 14, 0);
    DrawRect(1, 467, 638, 12, 32);
    DrawRect(2, 468, 636, 10, 64);

    DrawCircuit();
    DrawArtifacts();

    SpecialTile((player_x+PLAYERW/2)/32, (player_y+PLAYERH/2)/32);

    if (map_enabled) DisplayAutomap();

    if ((boss_fight_mode != 0)&&(boss_fight_mode == 23)&&(!game_paused)) {
      BossControl();
    }
    if ( (boss_dlg != 0) && (!game_paused)) {
      BossDialog();
    }

    if (game_paused && (!map_enabled) && (!voluntary_exit)) {
      for (i = 0; i < 10; i++) {
        DrawRect((640 - 6 * 8) / 2 - i, (480 - 8) / 2 - i, 6*8 + 2*i, 8 + 2*i, 64 - i*5);
      }
      draw_text((640 - 6 * 8) / 2, (480 - 8) / 2, "Paused", 255);

      // TODO: remove test code
      DrawRect(10, 10, 620, 460, 0);
      DrawStats();

      ComposeTime(buf, expired_ms, 0);
      draw_text(636 - strlen(buf)*8, 470, buf, 255);
    }

    if (voluntary_exit) {
      DrawRect(152, 200, 336, 80, 128);
      DrawRect(160, 208, 320, 64, 64);
      draw_text((640 - 30 * 8) / 2, (480 - 8) / 2 - 4, "Are you sure you want to quit?", 255);
      draw_text((640 - 23 * 8) / 2, (480 - 8) / 2 + 4, "Press enter to confirm.", 255);
    }

    if (isArchipelago() && HasAPStatus()) {
      size_t len = strlen(GetAPStatus());
      if (len > 0) {
        DrawRect(10, 450, len * 8 + 2, 10, 64);
        draw_text(11, 451, GetAPStatus(), 192);
      }
    }

    VideoUpdate();

    MusicUpdate();

    EndCycle(0);

    can_move = 1;

    if ((player_dying != 0) && (player_hp <= 1)) can_move = 0;
    if (rooms[player_room].room_type == 5)
      if (CanGetArtifact())
        if (Get((player_x+PLAYERW/2)/32, (player_y+PLAYERH/2)/32)==42)
          if (rooms[player_room].enemies == 0)
            can_move = 0;

    if (rooms[player_room].room_type == 6)
      if (CanGetArtifact())
        if (PlayerDist(rooms[player_room].w * 16 + rooms[player_room].x * 32,
              rooms[player_room].h * 16 + rooms[player_room].y * 32) < 32)
          if (rooms[player_room].enemies == 0)
            if (current_boss == 3)
              can_move = 0;

    if (scroll_home != 0) can_move = 0;
    if (boss_fight_mode == 1) can_move = 0;
    if (boss_fight_mode >= 3) can_move = 0;
    if (opening_door_i != 0) can_move = 0;
    if (game_paused) can_move = 0;

    HandleEvents();
    if (map_enabled) {
      game_paused = 1;
    }

    if (can_move) {

      ix = player_x;
      iy = player_y;
      off_x = 0;
      off_y = 0;
      if (key_held[K_UP] && !key_held[K_DN]) {
        iy -= player_walk_speed * (artifacts[AF_METABOLISM]?1.4:1);
        player_dir = 0;
      }
      if (key_held[K_DN] && !key_held[K_UP]) {
        iy += player_walk_speed * (artifacts[AF_METABOLISM]?1.4:1);;
        player_dir = 1;
        off_y = 24;
      }
      if (key_held[K_LT] && !key_held[K_RT]) {
        ix -= player_walk_speed * (artifacts[AF_METABOLISM]?1.4:1);;
        if (!(key_held[K_UP] || key_held[K_DN])) {
          player_dir = 3;
        }
      }
      if (key_held[K_RT] && !key_held[K_LT]) {
        off_x = 16;
        ix += player_walk_speed * (artifacts[AF_METABOLISM]?1.4:1);;
        if (!(key_held[K_UP] || key_held[K_DN])) {
          player_dir = 2;

        }
      }
      if ((key_held[K_SP])&&(magic_circuit >= 0)) {
        magic_circuit += (circuit_fillrate * (3+training+(circuit_fillrate==30))/3);
      } else {
        if (magic_circuit < 0) {
          magic_circuit += (circuit_recoverrate * (3+training+(circuit_recoverrate==30))/3);
          if (magic_circuit > 0) magic_circuit = 0;
        } else {
          if (magic_circuit > 0) {
            ReleaseCircuit();
          }
        }
      }

      if (magic_circuit > circuit_size) magic_circuit = circuit_size;

      if ((ix!=player_x)||(iy!=player_y)) {
        // Are we changing to a new square?
        if (((player_x / 32)!=((ix+off_x) / 32)) || ((player_y / 32)!=((iy+off_y) / 32))) {
          //printf("%d\n", tile);
          if (TouchTile(ix, iy)) {
            player_wlk = (player_wlk + 1 + artifacts[AF_METABOLISM]*3) % (4*wlk_wait);
          } else {
            if (TouchTile(player_x, iy)) {
              player_wlk = (player_wlk + 1 + artifacts[AF_METABOLISM]*3) % (4*wlk_wait);
            } else {
              if (TouchTile(ix, player_y)) {
                player_wlk = (player_wlk + 1 + artifacts[AF_METABOLISM]*3) % (4*wlk_wait);
                if (off_x > 0) player_dir = 2;
                else player_dir = 3;
              }
            }

          }
        } else {
          player_x = ix;
          player_y = iy;

          player_wlk = (player_wlk + 1 + artifacts[AF_METABOLISM]*3) % (4*wlk_wait);
        }
      }
    }

    if ((t % (33 * 10))==(33 * 10 - 1)) {
      ActivateRand();
    }

    if (voluntary_exit && enter_pressed) {
      voluntary_exit = 0;
      game_running = 0;
      game_paused = 0;
    }

    if ((player_lives == 0) && (!training)) {
      break;
    }
    if (show_ending) {
      break;
    }
  }

  if (show_ending) {
    show_ending = 0;
    ShowEnding();
  }

  if ((player_lives == 0) && (!training)) {
    SDL_FillRect(screen, NULL, 0);
    draw_text(252, 236, "G A M E   O V E R", 255);
    VideoUpdate();
    SDL_Delay(2000);
  }

  if (current_boss == 3 && artifacts[AF_CURSED_SEAL] && player_room == 0) {
    char showStats = 0;
    if (player_lives == 0) showStats = AnnounceVictory(0);
    if (showStats) ShowBadEndingStats();
  }

  EndRando();
  DestroyStores();

  return 0;
}

void UpRoom()
{
  int i, nd;

  nd = rooms[player_room].s_dist + 1;

  for (i = 0; i < rooms_to_gen; i++) {
    if (rooms[i].s_dist == nd) {
      player_x = rooms[i].x * 32 + 64;
      player_y = rooms[i].y * 32 + 64;
    }
  }
}

void CancelVoluntaryExit()
{
  if (voluntary_exit) {
    voluntary_exit = 0;
    game_paused = 0;
  }
}

void HandleEvents()
{
  unsigned short db;
  static SDL_Event event;
  int pressed_tab = 0;

  if (PLAYBACK) {
    db = fgetc(record_file);
    db |= fgetc(record_file) << 8;

    key_held[K_UP] = (db & 0x0001)>0;
    key_held[K_DN] = (db & 0x0002)>0;
    key_held[K_LT] = (db & 0x0004)>0;
    key_held[K_RT] = (db & 0x0008)>0;
    key_held[K_SP] = (db & 0x0010)>0;
    enter_pressed  = (db & 0x0020)>0;
    map_enabled    = (db & 0x0040)>0;
    game_running   = (db & 0x0080)>0;
    game_paused    = (db & 0x0100)>0;
    voluntary_exit = (db & 0x0200)>0;
    pressed_tab    = (db & 0x0400)>0;
    tele_select    = (db & 0x0800)>0;

    return;
  }

  if (pressed_tab) {
    c_scroll_x = player_x;
    c_scroll_y = player_y;
  }

  enter_pressed = 0;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
          case SDLK_UP:
            menu_key_held[K_UP] = 1;
            // fallthrough
          case SDLK_w:
            key_held[K_UP] = 1;
            CancelVoluntaryExit();
            break;
          case SDLK_DOWN:
            menu_key_held[K_DN] = 1;
            // fallthrough
          case SDLK_s:
            key_held[K_DN] = 1;
            CancelVoluntaryExit();
            break;
          case SDLK_a:
          case SDLK_LEFT:
            key_held[K_LT] = 1;
            CancelVoluntaryExit();
            break;
          case SDLK_d:
          case SDLK_RIGHT:
            key_held[K_RT] = 1;
            CancelVoluntaryExit();
            break;
          case SDLK_SPACE:
            key_held[K_SP] = 1;
            CancelVoluntaryExit();
            break;
          case SDLK_c:
            key_held[K_c] = 1;
            break;
          case SDLK_RETURN:
            enter_pressed = 1;
            break;
          case SDLK_ESCAPE:
            if (map_enabled) {
              map_enabled = 0;
              game_paused = 0;
              tele_select = 0;
            } else {
              voluntary_exit ^= 1;
              game_paused = voluntary_exit;
            }
            break;
          case SDLK_TAB:
            if (tele_select) {
              map_enabled = 0;
              game_paused = 0;
              tele_select = 0;
            } else {
              pressed_tab = 1;
              map_enabled ^= 1;
              game_paused = map_enabled;
              c_scroll_x = player_x;
              c_scroll_y = player_y;
            }
            CancelVoluntaryExit();
            break;
          case SDLK_h:
            CancelVoluntaryExit();
            if (menu_screen != MENU_CONNECTION) {
              ShowHelp();
            }
            break;
          case SDLK_p:
            game_paused ^= 1;
            CancelVoluntaryExit();
            break;
          case SDLK_BACKSPACE:
            key_held[K_BS] = 1;
            break;

#ifdef DEBUG_KEYS
          case SDLK_j:
            {
              player_shield = 20;
              circuit_recoverrate = 20;
              circuit_fillrate = 20;
            }
            break;
          case SDLK_k:
            {
              int i, n, j;
              for (j = 0; j < 1; j++) {
                for (i = 0; i < 50000; i++) {
                  n = rand()%rooms_to_gen;
                  if (rooms[n].visited == 0) {
                    player_x = rooms[n].x * 32 + rooms[n].w * 16;
                    player_y = rooms[n].y * 32 + rooms[n].h * 16;
                    rooms[n].visited = 1;
                    explored++;
                    break;
                  }
                }
              }
            }
            break;

          case SDLK_m:
            {
              int i;
              for (i = 0; i < AF_MAXX_NOKEYS; i++) {
                artifacts[i] = 1;
              }
              for (i = AF_MAXX_NOKEYS; i < AF_MAXX; i++) {
                artifacts[i] = 0;
              }
              //artifacts[AF_CURSED_SEAL] = 0;
            }
            break;

          case SDLK_n:
            {
              current_boss = 3;
              expired_ms = 1000000;
            }
            break;
#endif

          default:
            break;
        }
      }
      if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
          case SDLK_UP:
            menu_key_held[K_UP] = 0;
            // fallthrough
          case SDLK_w:
            key_held[K_UP] = 0;
            break;
          case SDLK_DOWN:
            menu_key_held[K_DN] = 0;
            // fallthrough
          case SDLK_s:
            key_held[K_DN] = 0;
            break;
          case SDLK_a:
          case SDLK_LEFT:
            key_held[K_LT] = 0;
            break;
          case SDLK_d:
          case SDLK_RIGHT:
            key_held[K_RT] = 0;
            break;
          case SDLK_SPACE:
            key_held[K_SP] = 0;
            break;
          case SDLK_c:
            key_held[K_c] = 0;
            break;
          case SDLK_BACKSPACE:
            key_held[K_BS] = 0;
            break;
          default:
            break;
        }
      }
      if (event.type == SDL_TEXTINPUT) {
        typed_key = event.text.text[0];
      }
      if (event.type == SDL_QUIT) {
        voluntary_exit = 1;
      }
    }

  //HandleGamepad (key_held);

  if (RECORDING) {
    db = 0;

    db |= 0x0001 * key_held[K_UP];
    db |= 0x0002 * key_held[K_DN];
    db |= 0x0004 * key_held[K_LT];
    db |= 0x0008 * key_held[K_RT];
    db |= 0x0010 * key_held[K_SP];
    db |= 0x0020 * enter_pressed;
    db |= 0x0040 * map_enabled;
    db |= 0x0080 * game_running;
    db |= 0x0100 * game_paused;
    db |= 0x0200 * voluntary_exit;
    db |= 0x0400 * pressed_tab;
    db |= 0x0800 * tele_select;

    fputc(db & 0x00FF, record_file);
    fputc((db & 0xFF00)>>8, record_file);
    return;
  }

}

void DrawLevel(int off_x, int off_y, int hide_not_visited, int fog_of_war)
{
  static SDL_Surface *tiles = NULL;
  static SDL_Surface *fog = NULL;
  Uint8 *pp;
  SDL_Rect tilerec, screenrec;
  int x, y, i;
  int resolve_x, resolve_y;

  DrawRect(0, 0, 640, 480, 255);

  if (tiles == NULL) {
    tiles = IMG_Load("dat/i/tileset.png");
    fog = IMG_Load("dat/i/tileset.png");

    pp = fog->pixels;

    for (i = 0; i < fog->w*fog->h; i++) {
      *pp = *pp / 2 + 128;
      pp++;
    }
  }
  for (y = 0; y < 16; y++) {
    for (x = 0; x < 21; x++) {
      resolve_x = x + (off_x/32);
      resolve_y = y + (off_y/32);

      if ((GetVisited(resolve_x, resolve_y) == 0)&&(player_room != GetRoom(resolve_x, resolve_y))&&(hide_not_visited)) {
        tilerec.x = 17 * 32;
      } else {
        tilerec.x = Get(resolve_x, resolve_y) * 32;
      }
      tilerec.y = 0;
      tilerec.w = 32;
      tilerec.h = 32;

      screenrec.x = x*32 - ( (off_x) %32);
      screenrec.y = y*32 - ( (off_y) %32);

      if ((player_room != GetRoom(resolve_x, resolve_y))&&(fog_of_war)) {
        SDL_BlitSurface(fog, &tilerec, screen, &screenrec);
      } else {
        SDL_BlitSurface(tiles, &tilerec, screen, &screenrec);
      }
    }
  }
}

void DrawPlayer(int x, int y, int pl_dir, int pl_frm)
{
  static SDL_Surface *playersprite = NULL;
  SDL_Rect playerrec, screenrec;

  if (playersprite == NULL) {
    playersprite = IMG_Load("dat/i/player.png");
    SDL_SetColorKey(playersprite, SDL_TRUE | SDL_RLEACCEL, 0);
  }

  playerrec.x = pl_frm * 16;
  playerrec.y = pl_dir * 24;
  playerrec.w = 16;
  playerrec.h = 24;

  screenrec.x = x;
  screenrec.y = y;

  SDL_BlitSurface(playersprite, &playerrec, screen, &screenrec);
}

void SetGreyscalePalette()
{
  SDL_Color grey[256];
  //SDL_Color pal[256];
  int i;

  float ip;

  for (i = 0; i < 256; i++) {
    grey[i].r = grey[i].g = grey[i].b = i;
  }

  for (i = 0; i < 256; i++) {
    ip = (float)i / 255.0;
    pal[i].r = (cos(ip * M_PI / 2.0 + M_PI) + 1.0) * 255;
    pal[i].g = (sin(ip * M_PI / 2.0) * 255 + i) / 2;
    pal[i].b = sin(ip * M_PI / 2.0) * 255;
  }

  //SDL_SetPaletteColors(screen->format->palette, pal, 0, 256);
  //SDL_SetPalette(screen, SDL_PHYSPAL, pal, 0, 256);
}

void SetTonedPalette(float dct)
{
  //SDL_Color pal[256];
  float pct = 1.0 - dct;
  float rp_dct, rp_pct;
  float ip;
  int ec;
  int i;
  static int tk = 0;

  ec = rooms[player_room].enemies;

  if (ec < 50) {
    rp_dct = (float)ec / 50.0;
  } else {
    rp_dct = 1.0;
  }
  rp_pct = 1.0 - rp_dct;

  if ( (player_room == 0) && (current_boss == 3) && (boss_fight_mode >= 3) ) {
    if (boss_fight_mode == 23) {
      for (i = 0; i < 256; i++) {
        pal[i].r = i;
        pal[i].g = i;
        pal[i].b = i;
      }
    } else {
      tk++;
      pct = sin((float)tk / 20.0 * M_PI) * (0.5 - (float)(boss_fight_mode-3)*0.025) + (0.5 - (float)(boss_fight_mode-3)*0.025);

      if (magic_circuit < 0.1) pct = 1.0;

      for (i = 0; i < 256; i++) {
        ip = (float)i / 255.0;
        pal[i].r = 255 - (255 - (cos(ip * M_PI / 2.0 + M_PI) + 1.0) * 255)*pct;
        pal[i].g = 255 - (255 - i)*pct;
        pal[i].b = 255 - (255 - sin(ip * M_PI / 2.0) * 255) * pct;
      }


      pal[1].r = 0;
      pal[1].g = 0;
      pal[1].b = 0;
    }
  } else {
    if (artifacts[AF_CURSED_SEAL]) {
      if (player_room == 0) {
        tk++;
        pct = sin((float)tk / 33.0 * M_PI) * 0.5 + 0.5;
        for (i = 0; i < 256; i++) {
          pal[i].r = i;
          pal[i].g = (i / 3)*pct;
          pal[i].b = (i * 2 / 3)*pct;
        }
      } else {
        for (i = 0; i < 256; i++) {
          ip = (float)i / 255.0;
          pal[i].r = i;
          pal[i].g = i * dct;
          pal[i].b = (cos(ip * M_PI / 2.0 + M_PI) + 1.0) * 255 * dct;
        }
      }

      if ( (current_boss == 3) && (player_shield == 30) && (player_room == 0)) {
        if (boss_lives <= 1) {
          tk++;
          for (i = 0; i < 256; i++) {
            pct = sin((float) (tk + i) / 24.0 * M_PI) * 0.5 + 0.5;

            pal[i].r = (i * 0.5 + 128)*pct;
            pal[i].g = i * 0.5 + 128;
            pal[i].b = (i * 0.5 + 128)*pct;
          }
        }
      }
    } else {
      for (i = 0; i < 256; i++) {
        ip = (float)i / 255.0;
        pal[i].r = (((cos(ip * M_PI / 2.0 + M_PI) + 1.0) * 255)*pct + i*dct)*rp_pct + (sin(ip * M_PI / 2.0) * 207 + 48)*rp_dct;
        pal[i].g = (i)*rp_pct + ((cos(ip * M_PI / 2.0 + M_PI) + 1.0) * 255)*rp_dct;
        pal[i].b = ((sin(ip * M_PI / 2.0) * 255 * pct)+((cos(ip * M_PI / 2.0 + M_PI) + 1.0) * 255 * dct))*rp_pct + ((cos(ip * M_PI / 2.0 + M_PI) + 1.0) * 255)*rp_dct;
      }
    }
  }

  //SDL_SetPaletteColors(screen->format->palette, pal, 0, 256);
}

void SetTitlePalette(int curve_start, int curve_end)
{
  //SDL_Color pal[256];
  int ec;
  int i;

  for (i = 0; i < 256; i++) {
    ec = (i - curve_start) * 255 / (curve_end-curve_start);
    if (ec < 0) ec = 0;
    if (ec > 255) ec = 255;

    pal[i].r = ec;
    pal[i].g = ec;
    pal[i].b = ec;
  }

  //SDL_SetPaletteColors(screen->format->palette, pal, 0, 256);
}

void SetTitlePalette2(int t)
{
  //SDL_Color pal[256];
  int i;

  float ip;
  float bright;
  float b_coeff;

  bright = 1 - ((float)t / 30.0);
  if (bright < 0.0) bright = 0.0;
  b_coeff = 1 - bright;

  for (i = 0; i < 256; i++) {
    ip = (float)i / 255.0;
    pal[i].r = (cos(ip * M_PI / 2.0 + M_PI) + 1.0) * 255 * b_coeff + 255*bright;
    pal[i].g = (sin(ip * M_PI / 2.0) * 255 + i) / 2 * b_coeff + 255*bright;
    pal[i].b = sin(ip * M_PI / 2.0) * 255 * b_coeff + 255*bright;
  }

  //SDL_SetPaletteColors(screen->format->palette, pal, 0, 256);
}

int IsSolid(unsigned char tile)
{
  return TileData[tile].Is_Solid;
}

void ActivateBossDoor(int x, int y)
{
  static int bd_timer = 0;
  int bx = x, by = y;

  // find boss room
  if (rooms[GetRoom(x+1, y)].room_type == 2) {
    bx += 1;
  } else
    if (rooms[GetRoom(x-1, y)].room_type == 2) {
    bx -= 1;
  } else
    if (rooms[GetRoom(x, y+1)].room_type == 2) {
    by += 1;
  } else
    if (rooms[GetRoom(x, y-1)].room_type == 2) {
    by -= 1;
  } else
    return;

  if (artifacts[AF_MAXX_NOKEYS + rooms[GetRoom(bx, by)].room_param]) {
    opening_door_x = x;
    opening_door_y = y;
    opening_door_i = 1;
    opening_door_n = rooms[GetRoom(bx, by)].room_param;
    if ((SDL_GetTicks() - bd_timer) > 100) {
      SND_Pos("dat/a/crystal2.wav", 100, 0);
      bd_timer = SDL_GetTicks();
    }
  }
}

int TouchTile(int ix, int iy)
{
    int i;
    int off_x, off_y;
    int ret = 1;
    unsigned char tile;

    for (i = 0; i < 4; i++) {
      off_x = 15*(i%2);
      off_y = 23*(i/2);

      tile = Get((ix+off_x)/32, (iy+off_y)/32);
      switch (tile) {
        case 38:
        case 39:
        case 40:
        case 41:
          ActivateBossDoor((ix+off_x)/32, (iy+off_y)/32);
          ret = 0;
          break;
        case 13:
          player_x = (ix + off_x) / 32 * 32 + 8;
          player_y = (iy/32 + 2)*32 + 32;
          return 1;
          break;
        case 14:
          player_x = (ix + off_x) / 32 * 32 + 8;
          player_y = (iy/32 - 2)*32 + 8;
          return 1;
          break;
        case 15:
          player_x = (ix/32 + 2)*32 + 32;
          player_y = (iy + off_y) / 32 * 32 + 4;
          return 1;
          break;
        case 16:
          player_x = (ix/32 - 2)*32 + 16;
          player_y = (iy + off_y) / 32 * 32 + 4;
          return 1;
          break;
        default:
          if (TileData[tile].Is_Solid) ret = 0;
          //ret = 0;
          break;
      }
    }
    if (ret == 1) {
      player_x = ix;
      player_y = iy;
    }
    return ret;
}

void text_init()
{
  FILE *font_data_file;
  int chr, x, y;
  const char* font_data_filename = "dat/d/font.dat";
  font_data_file = fopen(font_data_filename, "rb");
  if (font_data_file == NULL) {
    perror(font_data_filename);
    exit(1);
  }

  for (chr = 0; chr < 128; chr++) {
    for (y = 0; y < 8; y++) {
      for (x = 0; x < 8; x++) {
        font_data[chr][x][y] = fgetc(font_data_file);
      }
    }
  }

  fclose(font_data_file);
}

void draw_char(int cur_x, int cur_y, int c, Uint8 tcol)
{
  int px, py;
  Uint32 *pix;

  for (py = 0; py < 8; py++) {
    pix = (Uint32 *)screen->pixels;
    pix += ((py+cur_y)*screen->w);
    pix += cur_x;

    if ((cur_x >= 0)&&(py+cur_y >= 0)&&(cur_x < screen->w-8)&&(py+cur_y < screen->h)) {
      for (px = 0; px < 8; px++) {
        if (font_data[c][px][py] == 255) {
          *pix = SDL_MapRGBA(screen->format, tcol, tcol, tcol, 255);
        }
        if ((font_data[c][px][py] < 255)&&(font_data[c][px][py] > 0)) {
          Uint8 col2 = ((int)tcol * font_data[c][px][py] / 256) + ((((int)*pix) >> 24) * (256-font_data[c][px][py]) / 256);
          *pix = SDL_MapRGBA(screen->format, col2, col2, col2, 255);
        }
        pix++;
      }
    }
  }
}

void draw_text(int x, int y, const char *str, Uint8 tcol)
{
  int c, cur_x, cur_y;

  cur_x = x;
  cur_y = y;

  while (*str != 0) {
    c = *(str++);
    if (c == '\n') {
      cur_x = x;
      cur_y+=10;
    } else {
      draw_char(cur_x, cur_y, c, tcol);
      cur_x+=8;
    }
  }
}

void draw_text_f(int x, int y, const char *str, Uint8 tcol, ...)
{
  char buf[255] = {0};
  va_list args;
  va_start(args, tcol);
  vsprintf(buf, str, args);
  draw_text(x, y, buf, tcol);
}

void draw_text_ex(int x, int y, const char *str, Uint8 tcol, SDL_Surface *srf)
{
  Uint8 *pix;
  int c, cur_x, cur_y, px, py;

  cur_x = x;
  cur_y = y;

  while (*str != 0) {
    c = *(str++);
    if (c == '\n') {
      cur_x = x;
      cur_y+=8;
    } else {
      for (py = 0; py < 8; py++) {
        pix = (Uint8 *)srf->pixels;
        pix += (py+cur_y)*srf->w;
        pix += cur_x;
        for (px = 0; px < 8; px++) {
          if (font_data[c][px][py]) {
            *pix = tcol;
          }
          pix++;
        }
      }
      cur_x+=8;
    }
  }
}

void LockDoors(int r)
{
  //printf("Locking room %d...", r);
  int x, y;
  int rx, ry;
  int rt;
  int rcount = 0;

  for (y = 0; y < rooms[r].h; y++) {
    for (x = 0; x < rooms[r].w; x++) {
      rx = x + rooms[r].x;
      ry = y + rooms[r].y;
      rt = Get(rx, ry);

      if ((rt >= 13) && (rt <= 16)) {
        rcount++;
        Put(rx, ry, rt - 13 + 21, r);
      }
    }
  }
  //printf("locked %d doors\n", rcount);
}

void ActivateRoom(int room)
{
  //printf("Activating room %d (type %d)\n", room, rooms[room].room_type);
  if (rooms[room].checkpoint) {
    checkpoints_found++;
  }
  if (rooms[room].room_type == 3) {
    // lock the doors!
    LockDoors(room);
  }
  ActivateEnemies(room);
}

void DrawRect(int x, int y, int w, int h, unsigned char c)
{
  SDL_Rect r;

  r.x = x;
  r.y = y;
  r.w = w;
  r.h = h;

  SDL_FillRect(screen, &r, SDL_MapRGBA(screen->format, c, c, c, 255));
}

void DrawCircuit()
{
  int vd = 520;
  char buf[20];

  if (magic_circuit != 0) {
    DrawRect(110, 469, 8+abs(magic_circuit) * vd / circuit_size, 9, (magic_circuit > 0) ? 159 : 72);
    DrawRect(111, 470, 6+abs(magic_circuit) * vd / circuit_size, 7, (magic_circuit > 0) ? 183 : 80);
    DrawRect(112, 471, 4+abs(magic_circuit) * vd / circuit_size, 5, (magic_circuit > 0) ? 207 : 96);
    DrawRect(113, 472, 2+abs(magic_circuit) * vd / circuit_size, 3, (magic_circuit > 0) ? 231 : 112);
    DrawRect(114, 473, abs(magic_circuit) * vd / circuit_size, 1, (magic_circuit > 0) ? 255 : 128);
  }
  sprintf(buf, "%.1f", fabs((float)magic_circuit / 100.0));
  draw_text(115, 470, buf, 0);
  draw_text(3, 469, "Psi Circuit", 200);
}

void ReleaseCircuit()
{
  add_int_stat(STAT_BURSTS, 1);
  add_float_stat(STAT_CIRCUIT_VALUE, (float)magic_circuit / 100.0);

  circuit_release = 1;
  release_range = circuit_range;
  release_x = player_x;
  release_y = player_y;
  release_str = magic_circuit;
  release_hit_something = 0;
  if (circuit_fillrate==30) {
    release_str *= 1.25;
  }

  SND_CircuitRelease(release_str);
  magic_circuit *= -1;
}

void DrawCircle(int x, int y, int r, unsigned char c)
{
  int circ_y;

  int len_x, outer_len_x, inner_len_x;

  int inner_r = r - 10;
  if (inner_r < 1) inner_r = 1;

  if (r < 1) return;
  // a^2 + b^2 = c^2
  for (circ_y = 0; circ_y < r; circ_y++) {
    if (circ_y < (r-10)) {
      outer_len_x = sqrt(r*r - circ_y*circ_y);
      inner_len_x = sqrt((r-10)*(r-10) - circ_y*circ_y);
      DrawRect(x - outer_len_x, y - circ_y, (outer_len_x - inner_len_x), 1, c);
      DrawRect(x + inner_len_x, y - circ_y, (outer_len_x - inner_len_x), 1, c);
      DrawRect(x - outer_len_x, y + circ_y, (outer_len_x - inner_len_x), 1, c);
      DrawRect(x + inner_len_x, y + circ_y, (outer_len_x - inner_len_x), 1, c);
    } else {
      len_x = sqrt(r*r - circ_y*circ_y);

      DrawRect(x - len_x, y - circ_y, len_x*2, 1, c);
      DrawRect(x - len_x, y + circ_y, len_x*2, 1, c);
    }
  }
}

void DrawCircleEx(int x, int y, int r, int r2, unsigned char c)
{
  int circ_y;

  int len_x, outer_len_x, inner_len_x;

  int inner_r = r2;
  int diffi = r-r2;
  if (inner_r < 1) inner_r = 1;



  if (r < 1) return;
  // a^2 + b^2 = c^2
  for (circ_y = 0; circ_y < r; circ_y++) {
    if (circ_y < (r-diffi)) {
      outer_len_x = sqrt(r*r - circ_y*circ_y);
      inner_len_x = sqrt((r-diffi)*(r-diffi) - circ_y*circ_y);
      DrawRect(x - outer_len_x, y - circ_y, (outer_len_x - inner_len_x), 1, c);
      DrawRect(x + inner_len_x, y - circ_y, (outer_len_x - inner_len_x), 1, c);
      DrawRect(x - outer_len_x, y + circ_y, (outer_len_x - inner_len_x), 1, c);
      DrawRect(x + inner_len_x, y + circ_y, (outer_len_x - inner_len_x), 1, c);
    } else {
      len_x = sqrt(r*r - circ_y*circ_y);

      DrawRect(x - len_x, y - circ_y, len_x*2, 1, c);
      DrawRect(x - len_x, y + circ_y, len_x*2, 1, c);
    }
  }
}

void DrawShield()
{
  static int t=0;
  int s_size;
  int belts = 0;
  int i, bpos;
  t++;

  if (player_shield == 0) return;
  if (shield_hp == 0) return;

  s_size = shield_hp;
  if (s_size > 15) {
    belts = s_size - 15;
    s_size = 15;
  }
  DrawCircleEx(320, 240, 28+s_size, 28-s_size, 128 + (shield_hp*127/player_shield) - (50*(shield_hp<player_shield) + shield_recover) - 45 + ((t%4)*15));

  for (i = 0; i < belts; i++) {
    bpos = 13 + (30 * (i+1) / (belts+1));
    DrawCircleEx(320, 240, bpos + 1, bpos - 1, ((i+t)%6*12));
  }
}

int GetNearestCheckpoint(int nx, int ny)
{
  int i;
  int nearest_checkpoint = -1;
  int nearest_dist = 10000000;
  int cp_x, cp_y, cp_dist;
  int room_chk[3000] = {0};
  int x, y, rx, ry;

  i = GetRoom(nx/32, ny/32);
  if (i != -1) {
    room_chk[i] = 1;
    if ((rooms[i].checkpoint != 0)&&(rooms[i].visited!=0)) {
      nearest_checkpoint = i;
    }
  }
  if (nearest_checkpoint == -1) {

    for (y = 0; y < 54;) {
      for (x = 0; x < 54;) {
        rx = nx/32 - 27 + x;
        ry = ny/32 - 27 + y;

        i = GetRoom(rx, ry);
        if (i != -1) {
          if (room_chk[i] == 0) {
            room_chk[i] = 1;
            if ((rooms[i].checkpoint != 0)&&(rooms[i].visited!=0)) {
              cp_x = rooms[i].x * 32 + rooms[i].w * 16;
              cp_y = rooms[i].y * 32 + rooms[i].h * 16;
              cp_dist = dist(cp_x, cp_y, nx, ny);
              if (cp_dist < nearest_dist) {
                nearest_dist = cp_dist;
                nearest_checkpoint = i;
              }
            }
          }
        }
        x += 2;
      }
      y += 2;
    }

  }

  return nearest_checkpoint;
}

void TeleportPlayerToRoom(int c_room)
{
  if (c_room == 0) {
    player_x = 8232;
    player_y = 8108;
  } else {
    player_x = rooms[c_room].x * 32 + (rooms[c_room].w / 2 * 32) + 8;
    player_y = rooms[c_room].y * 32 + (rooms[c_room].h / 2 * 32) + 4;
  }
  c_scroll_x = player_x;
  c_scroll_y = player_y;
  scroll_home = 1;
}


void TeleportPlayerToNextRoom()
{
  int c_room;
  c_room = (player_room + 1) % rooms_to_gen;
  while (! ((rooms[c_room].checkpoint!=0)&&(rooms[c_room].visited!=0))) {
    c_room = (c_room + 1) % rooms_to_gen;
  }

  if (c_room == 0) {
    player_x = 8232;
    player_y = 8108;
  } else {
    player_x = rooms[c_room].x * 32 + (rooms[c_room].w / 2 * 32) + 8;
    player_y = rooms[c_room].y * 32 + (rooms[c_room].h / 2 * 32) + 4;
  }
  c_scroll_x = player_x;
  c_scroll_y = player_y;
  scroll_home = 1;
}

void ActivateTile(unsigned char tile, int x, int y)
{
  int c_room;

  enter_pressed = 0;
  switch (tile) {
    case 25:
      if (artifacts[AF_CURSED_SEAL]) break;

      c_room = GetNearestCheckpoint(c_scroll_x, c_scroll_y);
      if (tele_select) {
        if (c_room != -1) {
          add_int_stat(STAT_CHECKPOINT_WARPS, 1);
          if (c_room == player_room) {
            TeleportPlayerToNextRoom();
          } else {
            TeleportPlayerToRoom(c_room);
          }
        }
      } else {
        map_enabled = 1;
        game_paused = 1;
        tele_select = 1;

        c_scroll_x = player_x;
        c_scroll_y = player_y;
      }

      break;
    case 26:
      // NOTE: Treasure boxes
      add_int_stat(STAT_CHESTS, 1);
      CollectItem(IS_CHESTS);
      Put(x, y, 27, GetRoom(x, y));
      break;
    case 28:
    case 29:
    case 30:
      int storeoffset = tile - 28;
      if (CostFactor(storeoffset) < 0) return;
      if (player_gems >= UpgradePrice(storeoffset)) {
        add_int_stat(STAT_PURCHASES, 1);
        add_int_stat(STAT_GEMS_SPENT, UpgradePrice(storeoffset));
        player_gems -= UpgradePrice(storeoffset);
        // NOTE: All three stores
        CollectItem(storeoffset);
      }
      break;
    case 31:
      DoSaveGame();
      break;
    case 32:
      CrystalSummon();
      SND_Pos("dat/a/crystal.wav", 80, 0);
      break;
    default:
      break;
  }
}

void CompassPoint()
{
  int nearest = 1000000;
  int n_room = -1;
  int i;
  int loc_x, loc_y;
  int cdist;
  int rplx, rply;
  int bosses_defeated = current_boss;
  float pdir_1 = 0;
  float pdir_2 = 0;
  int pdir_1t = 0, pdir_2t = 0;

  rplx = player_x + PLAYERW/2;
  rply = player_y + PLAYERH/2;
  // Find the nearest SIGNIFICANT LOCATION for the player

  // Look at the three artifacts
  // Unless the player is going for the place of power

  // TODO: make this code not assume that x499 and x999 are special rooms
  if (current_boss < 3) {
    for (i = 0; i < 3; i++) {
      // Has the player got this artifact already?
      if (!HasItemByIndex(IS_SPECIAL, SS_PSI_KEY_1 + i)) { // no
        // Can the player get the artifact?
        if (CanGetArtifact()) {
          // Point player to this artifact room, if it is the nearest
          loc_x = rooms[i * 1000 + 499].x * 32 + rooms[i * 1000 + 499].w * 16;
          loc_y = rooms[i * 1000 + 499].y * 32 + rooms[i * 1000 + 499].h * 16;
          cdist = dist(rplx, rply, loc_x, loc_y);
          if (cdist < nearest) {
            nearest = cdist;
            n_room = i * 1000 + 499;
          }
        }
      } 

      // Has the player already destroyed the boss?
      if (rooms[i * 1000 + 999].room_type == 2) { // no
        // Has the player got the artifact for this boss room?
        if (artifacts[AF_MAXX_NOKEYS + i]) { // yes
          // Point player to the boss room, if it is the nearest
          loc_x = rooms[i * 1000 + 999].x * 32 + rooms[i * 1000 + 999].w * 16;
          loc_y = rooms[i * 1000 + 999].y * 32 + rooms[i * 1000 + 999].h * 16;
          cdist = dist(rplx, rply, loc_x, loc_y);
          if (cdist < nearest) {
            nearest = cdist;
            n_room = i * 1000 + 999;
          }
        }
      } else { // yes
        bosses_defeated++;
      }
    }
  }
  // If, on the other hand, the player has destroyed all three bosses, point them towards the
  // PLACE OF POWER
  if (bosses_defeated == 3) {
    // If the player already has the seal, point them to home
    if (artifacts[AF_CURSED_SEAL] == 1) {
      loc_x = rooms[0].x * 32 + rooms[0].w * 16;
      loc_y = rooms[0].y * 32 + rooms[0].h * 16;
      cdist = dist(rplx, rply, loc_x, loc_y);
      if (cdist < nearest) {
        nearest = cdist;
        n_room = 0;
      }
    } else {
      // Can the player touch the seal?
      if (CanGetArtifact()) {
        loc_x = rooms[place_of_power].x * 32 + rooms[place_of_power].w * 16;
        loc_y = rooms[place_of_power].y * 32 + rooms[place_of_power].h * 16;
        cdist = dist(rplx, rply, loc_x, loc_y);
        if (cdist < nearest) {
          nearest = cdist;
          n_room = place_of_power;
        }
      }
    }
  }

  // Did we find a room? If so, point to it

  if (n_room != -1) {
    loc_x = rooms[n_room].x * 32 + rooms[n_room].w * 16;
    loc_y = rooms[n_room].y * 32 + rooms[n_room].h * 16;

    pdir_1 = PlayerDir(loc_x, loc_y) + M_PI;
    pdir_1t = 1;

    n_room = -1;
  }

  nearest = 1000000;
  // Find the nearest uncleared artifact room
  for (i = 0; i < rooms_to_gen; i++) {
    if (rooms[i].room_type == 3) {
      loc_x = rooms[i].x * 32 + rooms[i].w * 16;
      loc_y = rooms[i].y * 32 + rooms[i].h * 16;
      cdist = dist(rplx, rply, loc_x, loc_y);
      if (cdist < nearest) {
        nearest = cdist;
        n_room = i;
      }
    }
  }

  if (n_room != -1) {
    loc_x = rooms[n_room].x * 32 + rooms[n_room].w * 16;
    loc_y = rooms[n_room].y * 32 + rooms[n_room].h * 16;

    pdir_2 = PlayerDir(loc_x, loc_y) + M_PI;
    pdir_2t = 1;

    n_room = -1;
  }

  // Did we find at least one thing to point to? If not, abort
  if (!(pdir_1t || pdir_2t))
    return;

  DrawCircleEx(rplx - scroll_x, rply - scroll_y, 200, 190, 255);
  if (pdir_1t)
    DrawCircleEx(rplx - scroll_x + cos(pdir_1) * 170, rply - scroll_y + sin(pdir_1) * 170, 30, 20, 255);
  if (pdir_2t)
    DrawCircleEx(rplx - scroll_x + cos(pdir_2) * 170, rply - scroll_y + sin(pdir_2) * 170, 30, 20, 195);

  for (i = 0; i < 50; i++) {
    if (pdir_1t)
      DrawCircle(rplx - scroll_x + cos(pdir_1) * (25 + i * 4), rply - scroll_y + sin(pdir_1) * (25 + i * 4), 5, 255);
    if (pdir_2t)
      DrawCircle(rplx - scroll_x + cos(pdir_2) * (25 + i * 4), rply - scroll_y + sin(pdir_2) * (25 + i * 4), 5, 195);
  }
  DrawCircleEx(rplx - scroll_x, rply - scroll_y, 30, 20, 255);

  DrawCircleEx(rplx - scroll_x, rply - scroll_y, 197, 193, 128);
  if (pdir_1t)
    DrawCircleEx(rplx - scroll_x + cos(pdir_1) * 170, rply - scroll_y + sin(pdir_1) * 170, 27, 23, 128);
  if (pdir_2t)
    DrawCircleEx(rplx - scroll_x + cos(pdir_2) * 170, rply - scroll_y + sin(pdir_2) * 170, 27, 23, 78);

  for (i = 0; i < 50; i++) {
    if (pdir_1t)
      DrawCircle(rplx - scroll_x + cos(pdir_1) * (25 + i * 4), rply - scroll_y + sin(pdir_1) * (25 + i * 4), 3, 128);
    if (pdir_2t)
      DrawCircle(rplx - scroll_x + cos(pdir_2) * (25 + i * 4), rply - scroll_y + sin(pdir_2) * (25 + i * 4), 3, 78);
  }
  DrawCircleEx(rplx - scroll_x, rply - scroll_y, 27, 23, 128);
}

void PostMessage(int msgid, int duration, int paramcount, ...)
{
  t_queued_message *newmsg = malloc(sizeof(t_queued_message));
  memset(newmsg, 0, sizeof(t_queued_message));
  newmsg->msgid = msgid;
  newmsg->duration = duration;
  newmsg->paramcount = paramcount;
  newmsg->params = paramcount > 0 ? malloc(sizeof(char*) * paramcount) : NULL;

  if (paramcount > 0) {
    va_list ptr;

    va_start(ptr, paramcount);
    for (int x = 0; x < paramcount; x++) {
      const char *str = va_arg(ptr, const char*);
      size_t makelen = MIN(sizeof(char) * (strlen(str) + 1), 100);
      newmsg->params[x] = malloc(makelen);
      memset(newmsg->params[x], 0, makelen);
      snprintf(newmsg->params[x], makelen, str);
    }
    va_end(ptr);
  }

  if (msgqueue == NULL) msgqueue = newmsg;
  else {
    t_queued_message *findend = msgqueue;
    while (findend->next != NULL) findend = findend->next;
    findend->next = newmsg;
  }
}

void DisposeFrontMessage() {
  t_queued_message *delmsg = msgqueue;
  msgqueue = msgqueue->next;
  if (delmsg->params != NULL) {
    for (int x = 0; x < delmsg->paramcount; x++) free(delmsg->params[x]);
    free(delmsg->params);
  }
  free(delmsg);
}

void SpecialTile(int x, int y)
{
  static int otext = 0;
  static int t = 0;
  unsigned char tile;
  char message[100] = "";
  char specialmessage1[100] = "";
  char specialmessage2[100] = "";

  tile = Get(x, y);
  switch (tile) {
    case 25:
      if (artifacts[AF_CURSED_SEAL]) {
        sprintf(message, "This is a checkpoint, but it doesn't seem to be working");
        break;
      }
      if (checkpoints_found <= 1) {
        sprintf(message, "This is a checkpoint. You will return here when you die.");
      } else {
        sprintf(message, "Press ENTER to teleport between checkpoints.");
      }
      break;
    case 26:
      int chestnum = GetNextItemIndex(IS_CHESTS) + 1;
      if (chestnum > 0) sprintf(message, "Press ENTER to open the storage chest [#%d]", chestnum);
      else sprintf(message, "Press ENTER to open the storage chest [extra]");
      break;
    case 28:
    case 29:
    case 30:
      int offset = tile - 28;
      if (CostFactor(IS_ALPHA + offset) < 0) {
        sprintf(message, "%s cache is empty", cache_names[offset]);
      } else {
        sprintf(message, "Press ENTER to loot %s cache [#%d] (%d crystals)",
                cache_names[offset], GetNextItemIndex(IS_ALPHA + offset) + 1, UpgradePrice(offset));
      }
      break;
    case 31:
      sprintf(message, "Press ENTER to record your progress");
      break;
    case 32:
      if (total_gems == 0) {
        sprintf(message, "This is a crystal device. It isn't working at the moment.");
      } else {
        sprintf(message, "Press ENTER to activate the crystal device");
      }
      break;
    case 42:
      if (rooms[player_room].room_type == 5) {
        if (!CanGetArtifact(rooms[player_room].room_param)) {
          sprintf(message, "The artifact is tainted with shadow. You must slay more of the shadow first.");
        }
      }
      break;
    case 53:
      CompassPoint();
      break;
    default:
      if (first_game && otext < 60) {
        sprintf(message, "Press H to read the help file");
        otext++;
      }
      break;
  }

  if (tile != 53 && key_held[K_c] && artifacts[AF_PORTABLE_COMPASS]) CompassPoint();

  if (msgqueue != NULL) {
    switch (msgqueue->msgid) {
      case 1: // Map
        sprintf(specialmessage1, "A ripple from %s", msgqueue->params[0]);
        sprintf(specialmessage2, "reveals the labyrinth within the Atlas Dome [TAB]");
        break;
      case 2: // Shield Boost
        sprintf(specialmessage1, "A ripple from %s", msgqueue->params[0]);
        sprintf(specialmessage2, "rejuvenates Virtue's shield");
        break;
      case 3: // Crystal Efficiency
        sprintf(specialmessage1, "A ripple from %s", msgqueue->params[0]);
        sprintf(specialmessage2, "enriches all crystals within the Dome");
        break;
      case 4: // Circuit Booster
        sprintf(specialmessage1, "A ripple from %s", msgqueue->params[0]);
        sprintf(specialmessage2, "extends Virtue's PSI Circuit range");
        break;
      case 5: // Metabolism
        sprintf(specialmessage1, "A ripple from %s", msgqueue->params[0]);
        sprintf(specialmessage2, "hastens Virtue's pace");
        break;
      case 6: // Dodge Enhancer
        sprintf(specialmessage1, "A ripple from %s", msgqueue->params[0]);
        sprintf(specialmessage2, "sharpens Virtue's reflexes");
        break;
      case 7: // Ethereal Monocle
        sprintf(specialmessage1, "A ripple from %s", msgqueue->params[0]);
        sprintf(specialmessage2, "reveals nearby threats");
        break;
      case 8: // Crystal Gatherer
        sprintf(specialmessage1, "A ripple from %s", msgqueue->params[0]);
        sprintf(specialmessage2, "draws crystals toward Virtue");
        break;
      case 9: // Portable Compass
        sprintf(specialmessage1, "A ripple from %s", msgqueue->params[0]);
        sprintf(specialmessage2, "shows Virtue the way forward [C]");
        break;

      case 10: // Reflect Shield upgrade
        sprintf(specialmessage1, "A ripple from %s", msgqueue->params[0]);
        sprintf(specialmessage2, "strengthens Virtue's shield");
        break;
      case 11: // Circuit Charge upgrade
        sprintf(specialmessage1, "A ripple from %s", msgqueue->params[0]);
        sprintf(specialmessage2, "increases Virtue's PSI Circuit strength");
        break;
      case 12: // Circuit Refill upgrade
        sprintf(specialmessage1, "A ripple from %s", msgqueue->params[0]);
        sprintf(specialmessage2, "hastens Virtue's PSI Circuit recharge rate");
        break;

      case 20: // Crystals
        sprintf(specialmessage1, "A ripple from %s", msgqueue->params[0]);
        sprintf(specialmessage2, "showers Virtue with PSI crystals");
        break;
      case 21: // "Nothing" item
        sprintf(specialmessage1, "A ripple from %s", msgqueue->params[0]);
        sprintf(specialmessage2, "dissipates and does nothing");
        break;
      case 22: // Extra life
        sprintf(specialmessage1, "A ripple from %s", msgqueue->params[0]);
        sprintf(specialmessage2, "protects Virtue's life");
        break;

      case 25: // Evolution Trap
        sprintf(specialmessage1, "A disruption from %s", msgqueue->params[0]);
        sprintf(specialmessage2, "makes the Atlas Dome more dangerous");
        break;

      case 30: // deprecated
        sprintf(specialmessage1, "Holy Sword 'Balmung' answers your call");
        break;
      case 31: // deprecated
        sprintf(specialmessage1, "Mystic Halberd 'Amenonuhoko' answers your call");
        break;
      case 32: // deprecated
        sprintf(specialmessage1, "Divine Bow 'Gandiva' answers your call");
        break;
      case 33: // Cursed Seal
        sprintf(specialmessage1, "You capture the cursed seal");
        sprintf(specialmessage2, "Return to the entrance");
        break;
      case 34: // PSI Key
        sprintf(specialmessage1, "A surge from %s beckons", msgqueue->params[0]);
        sprintf(specialmessage2, "the %s to Virtue's aid", msgqueue->params[1]);
        break;

      case 40: // deprecated
        sprintf(specialmessage1, "Balmung will remain here,");
        sprintf(specialmessage2, "where the ley lines are strong");
        break;
      case 41: // deprecated
        sprintf(specialmessage1, "Amenonuhoko will remain here,");
        sprintf(specialmessage2, "where the ley lines are strong");
        break;
      case 42: // deprecated
        sprintf(specialmessage1, "Gandiva will remain here,");
        sprintf(specialmessage2, "where the ley lines are strong");
        break;
      case 43: // Boss defeated
        sprintf(specialmessage1, "%s will remain here,", msgqueue->params[0]);
        sprintf(specialmessage2, "where the ley lines are strong");
        break;

      case 50: // Agate Knife
        sprintf(specialmessage1, ". . . . . .");
        sprintf(specialmessage2, "retrieved 'Agate Knife'");
        break;

      case 70: // Check goes to other player
        sprintf(specialmessage1, "Looting %s restores %s's", msgqueue->params[0], msgqueue->params[1]);
        sprintf(specialmessage2, msgqueue->params[2]);
        break;
      case 71: // Release received
        sprintf(specialmessage1, "As %s's world stabilizes, Virtue receives", msgqueue->params[0]);
        sprintf(specialmessage2, msgqueue->params[1]);
        break;
      case 72: // Collect received
        sprintf(specialmessage1, msgqueue->params[0]);
        sprintf(specialmessage2, "%s restored to %s's world", msgqueue->params[1], msgqueue->params[2]);
        break;
      case 73: // DeathLink received
        sprintf(specialmessage1, "A disruption from %s's world", msgqueue->params[0]);
        sprintf(specialmessage2, "results in Virtue's demise");
        break;
      case 74: // DeathLink sent
        sprintf(specialmessage1, "Virtue's death disrupts reality");
        sprintf(specialmessage2, "and can be felt across dimensions");
        break;
      case 75: // DeathLink disabled
        sprintf(specialmessage1, "Death seems like less");
        sprintf(specialmessage2, "of a threat now");
        break;

      default: sprintf(specialmessage1, "ERROR: NO/INVALID MESSAGE VALUE GIVEN"); break;
    }
    msgqueue->duration--;
    if (msgqueue->duration > 20 && msgqueue->next != NULL) msgqueue->duration--;
    if (msgqueue->duration <= 0) DisposeFrontMessage();
  }

  if (message[0] != 0) {
    DrawRect(320 - strlen(message)*8 / 2 - 20, 100, strlen(message)*8+40, 48, 200);
    DrawRect(320 - strlen(message)*8 / 2 - 15, 105, strlen(message)*8+30, 38, 32);
    DrawRect(320 - strlen(message)*8 / 2 - 10, 110, strlen(message)*8+20, 28, 64);

    draw_text(320 - strlen(message)*8 / 2, 120, message, t%16<8 ? 255 : 192);
  }

  if (specialmessage1[0] != 0) {
    if (specialmessage2[0] == 0) sprintf(specialmessage2, "test");
    int longermsg = (int)MAX(strlen(specialmessage1), strlen(specialmessage2));
    DrawRect(320 - longermsg*8 / 2 - 20, 325, longermsg*8+40, 55, 200);
    DrawRect(320 - longermsg*8 / 2 - 15, 330, longermsg*8+30, 45, 32);
    DrawRect(320 - longermsg*8 / 2 - 10, 335, longermsg*8+20, 35, 64);

    draw_text(320 - strlen(specialmessage1)*8 / 2, 344, specialmessage1, t%16<8 ? 255 : 192);
    draw_text(320 - strlen(specialmessage2)*8 / 2, 354, specialmessage2, t%16<8 ? 255 : 192);
  }

  t++;
  if (message[0] != 0 && enter_pressed) {
    ActivateTile(tile, x, y);
  }
}

void ScrollTo(int x, int y)
{
  //static int scrollspeed_x = 1, scrollspeed_y = 1;
  if (scroll_home == 0) {
    scroll_x = x;
    scroll_y = y;
    return;
  }

  if (scroll_home == 1) {
    // scrollspeed_x = (x - scroll_x)/20;
    // scrollspeed_y = (y - scroll_y)/20;
    scroll_home = 2;
  }

  if (scroll_home == 2) {
    scroll_x += (x - scroll_x)/2;
    scroll_y += (y - scroll_y)/2;

    if ((abs(scroll_x-x)<2)&&(abs(scroll_y-y)<2)) {
      scroll_x = x;
      scroll_y = y;
      scroll_home = 0;
    }
  }
}

void DrawArtifacts()
{
  int i;
  SDL_Rect from, to;

  if (artifact_spr == NULL) {
    artifact_spr = IMG_Load("dat/i/artifacts.png");
    SDL_SetColorKey(artifact_spr, SDL_TRUE | SDL_RLEACCEL, 0);
  }

  for (i = 0; i < AF_MAXX; i++) {
    if (artifacts[i]) {
      from.x = i * 32;
      from.y = 0;
      from.w = 32;
      from.h = 32;

      to.x = 608 - (i / 12) * 35;
      to.y = 47 + (i % 12) * 35;
      SDL_BlitSurface(artifact_spr, &from, screen, &to);
    }
  }
}

void Swap(int *a, int *b)
{
  *a ^= *b ^= *a ^= *b;
}

void ThinLine(SDL_Surface *scr, int x1, int y1, int x2, int y2, Uint8 col)
{
  int dx, dy, dm;
  int i, j;

  dx = (x2 - x1);
  dy = (y2 - y1);

  dm = abs(dx) > abs(dy) ? dx : dy;

  if (dm == 0) return;

  if (dm < 0) {
    Swap(&x1, &x2);
    Swap(&y1, &y2);
    dx = (x2 - x1);
    dy = (y2 - y1);

    dm = dm * -1;
  }

  if (dm == dx) {
    if (dy == 0) {
      DrawRect(x1, y1, x2-x1+1, 1, col);
      return;
    }
    for (i = 0; i < dm; i++) {
      j = (dy * i / dm);
      DrawRect(i+x1, j+y1, 1, 1, col);
    }
  }
  if (dm == dy) {
    if (dx == 0) {
      DrawRect(x1, y1, 1, y2-y1+1, col);
      return;
    }
    for (i = 0; i < dm; i++) {
      j = (dx * i / dm);
      DrawRect(j+x1, i+y1, 1, 1, col);
    }
  }
}

