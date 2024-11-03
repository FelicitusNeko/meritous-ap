//
//   ending.c
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
#include <math.h>
#include <SDL.h>
#include <SDL_image.h>
#include <assert.h>

#include "levelblit.h"
#include "audio.h"
#include "boss.h"
#include "mapgen.h"
#include "itemhandler.h"
#include "stats.h"
#include "demon.h"

void DrawScrolly(int t);
void DrawPText(int t);
void DrawSText(int t);
void DrawSTextV(int t);
void DrawCircuitFlash(int t, int method);
void DrawStream(int t);
void DrawCredits();

void InitParticleStorm();
void RunParticleStorm(int offset);

SDL_Surface *streamspr = NULL, *glitter = NULL;
SDL_Color ending_pal[256];

char statsmode = 0;

int credits_scroll = 0;

char *PText[10] = {
  "Activating the seal quickly initiated the shutdown sequence",
  "for the Chaos Engine Archipelago. Chambers throughout the",
  "Atlas Dome started to shatter in flashes of light.",
  "",
  "In one final act, Berserker had focused a burst of PSI at the",
  "machine's control panel, rendering it inoperable, before",
  "he himself flashing away into nothingness.",
  "",
  "With no other option at his disposal, Virtue's only course of",
  "action was to attempt to escape. However . . ."
};
char *SText[15] = {
  "The only exit from the Atlas Dome broken off from its very",
  "existence, and too much damage had been inflicted upon",
  "reality.",
  "The Atlas Dome project had shattered reality into endless",
  "timelines, and sadly, many of those were doomed to their",
  "destruction from the very beginning.",
  "",
  "For what seemed like a moment, or could as well have been an",
  "eternity, nothing was. It felt as though Virtue floated in an",
  "empty void, uncertain of their fate.",
  "",
  "It would come to be that someone calling herself the Goddess",
  "of Time, living in one of the worlds affected by the project,",
  "would have the ability to stitch reality back together,",
  "restoring it to a semblance of normalcy."
};

char *PTextV[10] ={
  "Activating the seal quickly initiated the shutdown sequence",
  "for the Chaos Engine Archipelago. Chambers throughout the",
  "Atlas Dome started to shatter in flashes of light.",
  "",
  "Virtue acted quickly, focusing the Agate Knife upon the machine",
  "before them, as reality warped around them and threatened to",
  "collapse at a moment's notice.",
  "",
  "Then, a sense of serenity fell upon them. Was the Agate Knife",
  "the missing piece of this project the entire time?"
};
char *STextV[15] = {
  "Piece by piece, reality began to fall back into place. In a",
  "moment, everything purloined from other realities had been",
  "returned to their origins. Meanwhile, Berserker seemed to have",
  "vanished, as though he had never been there.",
  "",
  "Virtue realized that, with the stability offered by the Knife,",
  "the Archipelago project could carry on without risking the",
  "destruction of myriad universes.",
  "",
  "Thus, rather than terminating the project, it continued on,",
  "the world below still oblivious to the happenings within the",
  "Atlas Dome.",
  "",
  "                      [[ BEST ENDING ]]",
  ""
};

void UpdatePalette()
{
  for(int i = 0; i < 256; i++)
  {
    pal[i] = ending_pal[i];
  }
  //SDL_SetPaletteColors(screen->format->palette, ending_pal, 0, 256);
}

int EndingEvents()
{
  static SDL_Event event;
  
  player_room = 0;
  current_boss = 3;
  boss_fight_mode = 4;
  
  MusicUpdate();
  
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
          return 1;
          break;
        case SDLK_s:
          return 2;
          break;
        case SDLK_r:
          SendAPSignal(APSIG_RELEASE);
          break;
        case SDLK_c:
          SendAPSignal(APSIG_COLLECT);
          break;
        default:
          break;
      }
    }
    if (event.type == SDL_QUIT) {
      return 1;
    }
  }

  if (isArchipelago()) PollAPClient();
  
  return 0;
}

void ShowEnding()
{
  int i;

  statsmode = 0;

  if (streamspr == NULL) {
    streamspr = IMG_Load("dat/i/stream.png");
    SDL_SetColorKey(streamspr, SDL_TRUE | SDL_RLEACCEL, 0);
    glitter = IMG_Load("dat/i/glitter.png");
    SDL_SetColorKey(glitter, SDL_TRUE | SDL_RLEACCEL, 0);
  }
  
  for (i = 0; i < 500; i += 1) {
    if (((i % 60) >= 24)&&((i % 60) < 34)) {
      DrawCircuitFlash((i % 60) - 24, 0);
    } else {
      DrawScrolly(i);
    }
    EndCycle(0);
    if (EndingEvents()) return;
  }
  for (i = 0; i < 30; i++) {
    DrawCircuitFlash(i, 1);
    
    EndCycle(0);
    if (EndingEvents()) return;
  }
  SDL_FillRect(screen, NULL, 255);
  for (i = 0; i < 350; i++) {
    DrawPText(i);
    EndCycle(0);
    if (EndingEvents()) return;
  }
  
  Paint(0, 0, 22, 27, "dat/d/cstream.loc");
  
  if (player_shield < 30) {
    for (i = 0; i < 400; i++) {
      DrawStream(i);
      EndCycle(0);
      if (EndingEvents()) return;
    }
    InitParticleStorm();
    for (i = 0; i < 240; i++) {
      RunParticleStorm(240-i);
      EndCycle(0);
      if (EndingEvents()) return;
    }
    for (i = 0; i < 60; i++) {
      RunParticleStorm(0);
      EndCycle(0);
      if (EndingEvents()) return;
    }
    for (i = 0; i < 180; i++) {
      RunParticleStorm(i*3);
      EndCycle(0);
      if (EndingEvents()) return;
    }
    for (i = 0; i < 500; i++) {
      DrawSText(i);
      EndCycle(0);
      if (EndingEvents()) return;
    }
  } else {
    for (i = 0; i < 250; i++) {
      DrawStream(i);
      EndCycle(0);
      if (EndingEvents()) return;
    }
    for (i = 0; i < 500; i++) {
      DrawSTextV(i);
      EndCycle(0);
      if (EndingEvents()) return;
    }
  }
  
  credits_scroll = 0;
  for (;;) {
    DrawCredits();
    EndCycle(0);
    switch (EndingEvents()) {
      case 1: return;
      case 2: statsmode = !statsmode; break;
      default: break;
    }
  }
}

// TODO: consolidate these two functions
void DrawSText(int t)
{
  int offset = 540 + (t / 2);
  int i, c;
  t = t * 350 / 500;
  int cl = 350 - t;
  
  for (i = 0; i < 64; i++) {
    DrawRect(0, i * 15 - offset, 640, 15, (64 - i) * cl / 350);
  }
  for (i = 64; i < 128; i++) {
    DrawRect(0, i * 15 - offset, 640, 15, (i - 64) * cl / 350);
  }
  
  if (t < 300) {
    for (i = 0; i < 15; i++) {
      c = (255 + (i * 100) - t*10);
      if (c < 0) c = 0;
      if (c > 255) c = 255;
      
      draw_text(68, 150+i*12, SText[i], 255-c);
    }
  } else {
    for (i = 0; i < 15; i++) {
      c = 5 + (t-300) * 5;
      
      draw_text(68, 150+i*12, SText[i], 255-c);
    }
  }
  
  UpdatePalette();
  VideoUpdate();
}
void DrawSTextV(int t)
{
  int offset = 540 + (t / 2);
  int i, c;
  t = t * 350 / 500;
  int cl = 350 - t;
  
  for (i = 0; i < 64; i++) {
    DrawRect(0, i * 15 - offset, 640, 15, (64 - i) * cl / 350);
  }
  for (i = 64; i < 128; i++) {
    DrawRect(0, i * 15 - offset, 640, 15, (i - 64) * cl / 350);
  }
  
  if (t < 300) {
    for (i = 0; i < 15; i++) {
      c = (255 + (i * 100) - t*10);
      if (c < 0) c = 0;
      if (c > 255) c = 255;
      
      draw_text(68, 150+i*12, STextV[i], 255-c);
    }
  } else {
    for (i = 0; i < 15; i++) {
      c = 5 + (t-300) * 5;
      
      draw_text(68, 150+i*12, STextV[i], 255-c);
    }
  }
  
  UpdatePalette();
  VideoUpdate();
}


float pt_x[500];
float pt_y[500];
float pt_vx[500];
float pt_vy[500];
int pt_t[500];

void InitParticleStorm()
{
  int i;
  
  for (i = 0; i < 500; i++) {
    pt_x[i] = 320;
    pt_y[i] = 960;
    pt_vx[i] = (float)(rand()%101) / 33.333 - 1.5;
    pt_vy[i] = (float)(rand()%101) / 10.0 - 16.1;
    pt_t[i] = rand()%100;
  }
}

char *credits[] = {
  "Concept, design, graphics, programming:",
  "                            Lancer-X/Asceai",
  "Sound Effects:              Various (public domain) sources",
  "Original beta testing:      Quasar, Terryn, Wervyn",
  "Additional patches:         bart9h",
  "Item randomizer:            KewlioMZX",
  "Archipelago client:         Black Sliver",
  "Additional gfx:             Riafeir",
  "AP beta testing:            MazukiTskiven, alwaysontreble,",
  "                            Archipelago community",
  "Music:                      ",
  "\"Ambient Light\"             Vogue of Triton",
  "\"Battle of Ragnarok\"        Frostbite",
  "\"Dragon Cave\"               TICAZ",
  " cavern.xm                  Unknown",
  "\"Caverns Boss\"              Alexis Janson",
  "\"Forest Boss\"               Alexis Janson",
  "\"Catacombs Boss\"            Alexis Janson",
  "\"Fear 2\"                    Mick Rippon",
  "\"The Final Battle\"          Goose/C�DA & iNVASiON",
  "\"Ice Frontier\"              Skaven/FC",
  "\"KnarkLoader 1.0\"           Rapacious",
  "\"RPG-Battle\"                Cyn",
  "\"Metallic Forest\"           Joseph Fox"
};

void DrawStats()
{
  int room_scaling_debug_stats = 0; // manually set this to 1 to output room scaling related logs

  char buf[30] = {0};

  char kills[11] = { '\0' };
  sprintf(kills, "%d/%d", get_int_stat(STAT_KILLS), total_enemies);

  draw_text_f(66, 30, "Circuit bursts     %9d", 192, get_int_stat(STAT_BURSTS));
  draw_text_f(66, 40, "Successful hits    %9d", 168, get_int_stat(STAT_HITS));
  draw_text_f(66, 50, "Kills            %11s", 192, kills);
  draw_text_f(66, 60, "Resisted hits      %9d", 168, get_int_stat(STAT_RESISTS));
  draw_text_f(66, 70, "Misses             %9d", 192, get_int_stat(STAT_WHIFFS));
  draw_text_f(66, 80, "Total discharged     %7.1f", 168, get_float_stat(STAT_CIRCUIT_VALUE));
  draw_text_f(66, 90, "Bullets cancelled  %9d", 192, get_int_stat(STAT_BULLETS_CANCELLED));

  draw_text_f(66, 110, "Hearts gathered    %9d", 192, get_int_stat(STAT_HEARTS_GATHERED));
  draw_text_f(66, 120, "Shielded hits      %9d", 168, get_int_stat(STAT_SHIELD_HITS));
  draw_text_f(66, 130, "Damage taken       %9d", 192, get_int_stat(STAT_DAMAGE_TAKEN));
  draw_text_f(66, 140, "Lives gained       %9d", 168, get_int_stat(STAT_LIVES_GAINED));
  draw_text_f(66, 150, "Lives lost         %9d", 192, get_int_stat(STAT_LIVES_LOST));

  draw_text_f(350, 30, "Crystals collected %9d", 168, get_int_stat(STAT_GEMS_COLLECTED));
  draw_text_f(350, 40, "Crystals spent     %9d", 192, get_int_stat(STAT_GEMS_SPENT));
  draw_text_f(350, 50, "Crys lost on death %9d", 168, get_int_stat(STAT_GEMS_LOST_ON_DEATH));
  draw_text_f(350, 60, "Store purchases    %9d", 192, get_int_stat(STAT_PURCHASES));
  draw_text_f(350, 70, "Chests opened      %9d", 168, get_int_stat(STAT_CHESTS));
  draw_text_f(350, 80, "Room transitions   %9d", 192, get_int_stat(STAT_ROOM_TRANSITIONS));
  draw_text_f(350, 90, "Checkpoint warps   %9d", 168, get_int_stat(STAT_CHECKPOINT_WARPS));
  draw_text_f(350, 100, "Saves              %9d", 192, get_int_stat(STAT_SAVES));

  if (player_shield == 30) {
    draw_text(350, 130, "Time to Agate Knife", 168);
    ComposeTime(buf, get_int_stat(STAT_TIME_KNIFE), 1);
    draw_text_f(350, 140, "%28s", 192, buf);
  }
  else {
    char explored_fraction[10] = { '\0' };
    sprintf(explored_fraction, "%d/%d", explored, rooms_to_gen);
    draw_text_f(350, 130, "Rooms explored     %9s", 168, explored_fraction);
  }

  draw_text(176, 200, "Tries      Time spent fighting              Time beaten", 192);
  for (int x = 0; x < 4; x++) {
    int bosstries = get_int_stat(STAT_TRIES_BOSS1 + x);
    if (bosstries) {
      char sent[25] = {0}, beat[25] = {0};
      ComposeTime(sent, get_int_stat(STAT_TIMESPENT_BOSS1 + x), 1);
      ComposeTime(beat, get_int_stat(STAT_TIME_BOSS1 + x), 1);
      draw_text(20, 210 + (x * 10), boss_names[x], x % 2 ? 192 : 168);
      draw_text_f(176, 210 + (x * 10), "%5d %24s %24s", x % 2 ? 192 : 168, bosstries, sent, beat);
    } else {
      draw_text(20, 210 + (x * 10), "????????", x % 2 ? 192 : 168);
    }
  }

  if (room_scaling_debug_stats) {
    draw_text_f(20, 280, "Debug stats:", 192);
    draw_text_f(20, 290, "Average Gems Left  %9.1f", 168, average_gems);
    draw_text_f(20, 300, "^ After Multiplier %9.1f", 192, expected_total_gems);
    draw_text_f(350, 290, "Max room distance  %9d", 168, max_dist);
  }
}

void ShowBadEndingStats()
{
  for (;;) {
    draw_text(2, 2, "ESC to end, R to release, C to collect", 192);
    DrawStats();
    EndCycle(0);
    switch (EndingEvents()) {
      case 2: break;
      default: return;
    }
  }
}

void DrawCredits()
{
  static SDL_Surface *fin = NULL;
  static SDL_Surface *theend[2] = {NULL};
  SDL_Rect draw_to;
  int i;
  int ypos;
  int c;
  int n_credits = sizeof(credits)/sizeof(*credits);
  int finish_point;
  
  finish_point = 400 + (n_credits * 50);
  
  draw_to.x = 384;
  draw_to.y = 352;
  
  SDL_FillRect(screen, NULL, 0);
  
  if (fin == NULL) {
    fin = IMG_Load("dat/i/fin.png");
    
    theend[0] = IMG_Load("dat/i/theend.png");
    theend[1] = IMG_Load("dat/i/true_end.png");
  }
  
  if (credits_scroll >= (finish_point + 80)) {
    if (statsmode) DrawStats();
    else SDL_BlitSurface(theend[(player_shield == 30)], NULL, screen, NULL);

    draw_text(2, 2, "ESC to end, S for stats", 192);
    if (isArchipelago()) draw_text(2, 12, "R to release, C to collect", 192);
  } else {
    SDL_BlitSurface(fin, NULL, screen, &draw_to);
    
    // Show each line of credits
    
    for (i = 0; i < n_credits; i++) {
      ypos = 800 + (i * 100) - credits_scroll * 2;
      
      if ((ypos >= 0)&&(ypos < 480)) {
        c = 255 - abs(ypos - 240);
        draw_text(120, ypos, credits[i], c);
      }
    }
    
  }
  
  for (i = 0; i < 128; i++) {
    ending_pal[i].r = 0;
    ending_pal[i].g = i;
    ending_pal[i].b = i*2;
  }
  for (i = 128; i < 256; i++) {
    ending_pal[i].r = (i - 128)*2+1;
    ending_pal[i].g = i;
    ending_pal[i].b = 255;
  }
  
  // Dim palette if we're just starting
  
  if (credits_scroll < 80) {
    for (i = 0; i < 256; i++) {
      ending_pal[i].r = ending_pal[i].r * credits_scroll / 80;
      ending_pal[i].g = ending_pal[i].g * credits_scroll / 80;
      ending_pal[i].b = ending_pal[i].b * credits_scroll / 80;
    }
  }
  
  // Also palette if we're finishing
  
  if ((credits_scroll >= (finish_point))&&(credits_scroll < (finish_point + 80))) {
    for (i = 0; i < 256; i++) {
      ending_pal[i].r = ending_pal[i].r * (finish_point+80-credits_scroll) / 80;
      ending_pal[i].g = ending_pal[i].g * (finish_point+80-credits_scroll) / 80;
      ending_pal[i].b = ending_pal[i].b * (finish_point+80-credits_scroll) / 80;
    }
  }
  
  if ((credits_scroll >= (finish_point + 80))&&(credits_scroll < (finish_point + 160))) {
    for (i = 0; i < 256; i++) {
      ending_pal[i].r = ending_pal[i].r * (credits_scroll - (finish_point + 80)) / 80;
      ending_pal[i].g = ending_pal[i].g * (credits_scroll - (finish_point + 80)) / 80;
      ending_pal[i].b = ending_pal[i].b * (credits_scroll - (finish_point + 80)) / 80;
    }
  }
  
  credits_scroll++;
  
  UpdatePalette();
  VideoUpdate();
}

void RunParticleStorm(int offset)
{
  SDL_Rect draw_from, draw_to;
  int i;
  
  for (i = 0; i < 64; i++) {
    DrawRect(0, i * 15 - offset, 640, 15, 64 - i);
  }
  
  for (i = 0; i < 500; i++) {
    if (pt_t[i] > 0) {
      pt_t[i]--;
    } else {
      pt_vy[i] += 0.1;
      pt_x[i] += pt_vx[i];
      pt_y[i] += pt_vy[i];
    }
    
    draw_from.x = (rand()%3)*32;
    draw_from.y = 0;
    draw_from.w = 32;
    draw_from.h = 32;
  
    draw_to.x = (int)pt_x[i] - 16;
    draw_to.y = (int)pt_y[i] - 16 - offset;
    SDL_BlitSurface(glitter, &draw_from, screen, &draw_to);
  }

  for (i = 0; i < 128; i++) {
    ending_pal[i].r = i*2;
    ending_pal[i].g = i*2;
    ending_pal[i].b = 0;
  }
  for (i = 128; i < 256; i++) {
    ending_pal[i].r = 255;
    ending_pal[i].g = 255;
    ending_pal[i].b = (i - 128)*2+1;
  }
  
  UpdatePalette();
  VideoUpdate();
}

void DrawStream(int t)
{
  int i;
  int scr_x = 32;
  int scr_y = 0;
  int strm_scrl;
  SDL_Rect draw_from, draw_to;
  
  for (i = 0; i < 256; i++) {
    ending_pal[i].r = i;
    ending_pal[i].g = (i * 7 / 8) + 16 + sin( (float)t / 8 )*16;
    ending_pal[i].b = (i * 3 / 4) + 32 + sin( (float)t / 8 )*32;
  }
  

  if (t >= 300) {
    scr_x = 32 + rand()%32 - rand()%32;
    scr_y = rand()%8;
  }
  
  if (t < 10) {
    scr_y = (20 - t * 2);
  }
  
  DrawLevel(scr_x, scr_y, 0, 0);
  DrawPlayer(344 - scr_x, 228 - scr_y, 0, 0);
  
  for (i = 0; i < 7; i++) {
    strm_scrl = (t * 20) % 128;
    draw_to.x = 0 - strm_scrl - scr_x + (128*i);
    draw_to.y = 19 - scr_y;
    
    if (i >= 300) {
      draw_to.y += rand()%4;
      draw_to.y -= rand()%4;
    }
    SDL_BlitSurface(streamspr, NULL, screen, &draw_to);
  }
  
  // glitter
  for (i = 0; i < 20; i++) {
    draw_from.x = (rand()%3)*32;
    draw_from.y = 0;
    draw_from.w = 32;
    draw_from.h = 32;
  
    draw_to.x = rand()%(640+32)-32;
    draw_to.y = (rand()%(124)) + 3;
    
    SDL_BlitSurface(glitter, &draw_from, screen, &draw_to);
  }
  
  if (t > 250) {
    if (t < 300) {
      if (t == 251) {
        SND_CircuitRelease(1000);
      }
      DrawCircle(320+32 - scr_x, 240 - scr_y, (t - 254) * 10, 255);
      DrawCircle(320+32 - scr_x, 240 - scr_y, (t - 252) * 10, 225);
      DrawCircle(320+32 - scr_x, 240 - scr_y, (t - 250) * 10, 195);
    }
  }
  
  UpdatePalette();
  VideoUpdate();
}

void DrawPText(int t)
{
  int i;
  int c;
  
  int x, y;
  
  for (i = 0; i < 256; i++) {
    ending_pal[i].r = i;
    ending_pal[i].g = i;
    ending_pal[i].b = (i * 3 / 4) + 64;
  }
  
  if (t < 300) {
  
    for (i = 0; i < 10; i++) {
      c = (255 + (i * 100) - t*10);
      if (c < 0) c = 0;
      if (c > 255) c = 255;
      
      if (player_shield != 30) {
        draw_text(68, 180+i*12, PText[i], c);
      } else {
        draw_text(68, 180+i*12, PTextV[i], c);
      }
    }
  } else {
    for (i = 0; i < 10; i++) {
      c = 5 + (t-300) * 5;
      
      if (player_shield != 30) {
        draw_text(68, 180+i*12, PText[i], c);
      } else {
        draw_text(68, 180+i*12, PTextV[i], c);
      }
    }
  }
  
  for (i = 0; i < (32 * 8); i++) {
    x = (i % 32)*20;
    y = (i / 32)*20;
    
    c = 237 + (i/32*2) + (rand()% (19 - (i/32) *2));
    DrawRect(x, y, 20, 20, c);
    c = 237 + (i/32*2) + (rand()% (19 - (i/32) *2));
    DrawRect(x, 460 - y, 20, 20, c);
  }
  
  
  UpdatePalette();
  VideoUpdate();
}

void DrawScrolly(int t)
{
  int xp;
  int yp;
  int i, j;
  float a_dir;
  float v_radius;
  int all_blue = 0;
  SDL_Rect draw_from, draw_to;
  
  int x, y, r;
  
  float bright;
  
  if (t < 795) {
    xp = 8192 - 320 - 3180 + (t * 4);
    yp = t * 20;
  } else {
    xp = 8192 - 320 + ( (t-795) * 10);
    yp = 795 * 20 - (t-795)*10;
  }
  
  // Palette
  
  
  if ((rand() % 10)==9) {
    all_blue = 1;
  }
  for (i = 0; i < 256; i++) {
    bright = sin((float)t / 10.0) * 0.2 + 0.4;
    ending_pal[i].r = (i * bright + (256*(1.0-bright))) * ((float)(all_blue == 0) * 0.5 + 0.5);
    ending_pal[i].g = (i * bright + (256*(1.0-bright))) * ((float)(all_blue == 0) * 0.5 + 0.5);
    ending_pal[i].b = i * bright + (256*(1.0-bright));
  }
  DrawLevel(xp, yp, 0, 0);
  
  v_radius = sin((float)t / 10.0)*20 + 100;
  
  for (i = 0; i < 5; i++) {
    x = rand()%640;
    y = rand()%480;
    r = rand()%500+100;
    
    DrawCircleEx(x, y, r+2, r-4, 128);
    DrawCircleEx(x, y, r, r-2, 255);
  }
  
  for (i = 0; i < 4; i++) {
    draw_from.x = (AF_PSI_KEY_1 + i) * 32;
    draw_from.y = 0;
    draw_from.w = 32;
    draw_from.h = 32;
    
    a_dir = ((float)t / 10.0) + (M_PI*(float)i/2);
    
    for (j = 10; j >= 0; j--) {
      DrawCircleEx(320+cos(a_dir)*v_radius, 240+sin(a_dir)*v_radius, 22 + j * 2, 0, abs(j-3) * 15);
    }
    DrawCircleEx(320+cos(a_dir)*v_radius, 240+sin(a_dir)*v_radius, 20, 0, 0);
    
    draw_to.x = 320 + cos(a_dir) * v_radius - 16;
    draw_to.y = 240 + sin(a_dir) * v_radius - 16;
    SDL_BlitSurface(artifact_spr, &draw_from, screen, &draw_to);
  }
  
  UpdatePalette();
  VideoUpdate();
}

void DrawCircuitFlash(int t, int method)
{
  static SDL_Surface *circ = NULL;
  static int xpos, ypos;
  int i, j;
  SDL_Rect from;
  
  if (circ == NULL) {
    circ = IMG_Load("dat/i/circuits_1.png");
  }
  
  if (t == 0) {
    if (method == 0) {
      xpos = rand()%641;
      ypos = rand()%481;
    } else {
      xpos = 320;
      ypos = 240;
    }
  }
  
  from.x = xpos;
  from.y = ypos;
  from.w = 640;
  from.h = 480;
  
  SDL_BlitSurface(circ, &from, screen, NULL);
  
  for (i = 0; i < 256; i++) {
    if (method == 0) {
      j = i * t / 4;
    } else {
      j = i * t / 8;
      if (t >= 20) {
        j += t * 25;
      }
    }
    
    if (j > 255) j = 255;
    ending_pal[i].r = j;
    ending_pal[i].g = j;
    ending_pal[i].b = j;
  }
  
  UpdatePalette();
  VideoUpdate();
}

