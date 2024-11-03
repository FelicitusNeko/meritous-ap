/**
 * Copyright (C) 2022 FelicitusNeko
 * 
 * This file is part of Meritous-AP.
 * 
 * Meritous-AP is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Meritous-AP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Meritous-AP.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <SDL.h>
#include <time.h>

#include "audio.h"
#include "boss.h"
#include "demon.h"
#include "itemstore.h"
#include "levelblit.h"
#include "stats.h"
#include "apinterface.h"

const char *enemyNames[] = {
  "Residue",
  "Warped Ant",
  "Focus",
  "Biped",
  "Lazet",
  "Amoeba",
  "Hyperslug",
  "Diskord",
  "Chaos",
  "Unseen",
  "Hunter"
};

const char *itemNames[] = {
  "Nothing",
  "Reflect Shield upgrade",
  "Circuit Charge upgrade",
  "Circuit Refill upgrade",
  "Map",
  "Shield Boost",
  "Crystal Efficiency",
  "Circuit Booster",
  "Metabolism",
  "Dodge Enhancer",
  "Ethereal Monocle",
  "Crystal Gatherer",
  "Portable Compass",
  "", // get these from artifact_names
  "",
  "",
  "",
  "Agate Knife",
  "Evolution Trap",
  "Some crystals",
  "More crystals",
  "Many crystals",
  "Extra Life"
};

int rooms_to_gen = 3000;
int rooms_to_knife = 3000;
float crystal_scaling = 1.0f; // adds multiplier to crystal gain to make up for fewer enemies when using smaller room counts

char apEnabled = 0;
char apStatus[24] = {0};
time_t apLastStatusUpdate = 0;
char deathlink_announced = 0;

char isArchipelago();

void InitRando() {
  apEnabled = ReadAPSettings();
  if (apEnabled) sprintf(apStatus, "Not connected");
}

void InitStores() {
  if (isArchipelago()) CreateAPStores();
  else LocalGenerateItemStores(0);
}

void DestroyStores() {
  if (isArchipelago()) DestroyAPStores();
  else LocalDestroyItemStores();
}

uint32_t RetrieveSeed() {
  if (isArchipelago()) return RetrieveAPSeed();
  else return time(NULL);
}

void StartRando() {
  if (isArchipelago()) ConnectAP();
}

void EndRando() {
  if (isArchipelago()) DisconnectAP();
}

int UpgradePrice(int t)
{
  if (isArchipelago()) return GetAPUpgradeCost((t_itemStores)t, training);
  else return GetLocalUpgradeCost((t_itemStores)t, training);
}

int CostFactor(t_itemStores store) {
  if (store < IS_ALPHA || store > IS_GAMMA) return -1;
  else if (isArchipelago()) return GetAPCostFactor(store);
  else return LocalGetNextIndexInStore(store);
}

t_itemTypes MakeCrystals() {
  // TODO: Make these weights configurable
  int randCrystals = rand() % 32;
  if (randCrystals < 24) return T_CRYSTALS_500;
  if (randCrystals < 31) return T_CRYSTALS_1000;
  return T_CRYSTALS_2000;
}

char HasItem(t_itemTypes item) {
  switch (item) {
    case T_NOTHING:
    case T_CRYSTALS_500:
    case T_CRYSTALS_1000:
    case T_CRYSTALS_2000:
    case T_1UP:
      return 0;
    case T_REFLECT_SHIELD:
      return player_shield >= 25;
    case T_CIRCUIT_CHARGE:
      return circuit_fillrate >= 25;
    case T_CIRCUIT_REFILL:
      return circuit_recoverrate >= 25;
    case T_AGATE_KNIFE:
      return player_shield == 30;
    case T_EVOLUTION_TRAP:
      return enemy_evolutions >= 3;
    default:
      return artifacts[item - T_MAP];
  }
}

void ProcessItem(t_itemTypes item, const char *source, char isRelease) {
  char noise = 0;
  int basemod = 500;

  if (HasItem(item)) item = MakeCrystals();
  //printf("Processing %d: ", item);

  // Process the item effect
  switch (item) {
    case T_NOTHING:
      //printf("Nothing\n");
      break;
    case T_REFLECT_SHIELD:
      //printf("Shield\n");
      player_shield++;
      break;
    case T_CIRCUIT_CHARGE:
      //printf("Charge\n");
      circuit_fillrate++;
      break;
    case T_CIRCUIT_REFILL:
      //printf("Refill\n");
      circuit_recoverrate++;
      break;
    case T_AGATE_KNIFE:
      //printf("Knife\n");
      add_int_stat(STAT_TIME_KNIFE, expired_ms);
      player_shield = 30;
      circuit_fillrate = 30;
      circuit_recoverrate = 30;
      player_hp = 6;
      break;
    case T_EVOLUTION_TRAP:
      //printf("EvoTrap\n");
    	enemy_evolutions++;
      SoupUpEnemies();
      break;
    case T_CRYSTALS_500:
    case T_CRYSTALS_1000:
    case T_CRYSTALS_2000:
      if (item >= T_CRYSTALS_1000) basemod *= 2;
      if (item == T_CRYSTALS_2000) basemod *= 2;
      //printf("Crystals x%d\n", basemod);
      int collect = rand()%((1 << (explored / (rooms_to_gen / 15))) * basemod);
      add_int_stat(STAT_GEMS_COLLECTED, collect);
      player_gems += collect;
      break;
    case T_1UP:
      //printf("1up\n");
      if (!training) {
        add_int_stat(STAT_LIVES_GAINED, 1);
        player_lives++;
      }
      player_hp = 3 + (player_shield == 30)*3;
      break;
    default:
      //printf("Artifact %d\n", item - T_MAP);
      artifacts[item - T_MAP] = 1;
      if (item == T_CURSED_SEAL) {
        current_boss = 3;
        Curse();
      }
      break;
  }

  // Show the appropriate message
  if (source) {
    if (isRelease) {
      char kelka = item >= T_PSI_KEY_1 && item <= T_CURSED_SEAL;
      PostMessage(71, 50, 2, source, kelka ? artifact_names[item - T_PSI_KEY_1] : itemNames[item]);
    } else switch (item) {
      case T_NOTHING:
        PostMessage(21, 120, 1, source);
        break;
      case T_REFLECT_SHIELD:
        PostMessage(10, 120, 1, source);
        noise = 1;
        break;
      case T_CIRCUIT_CHARGE:
        PostMessage(11, 120, 1, source);
        noise = 1;
        break;
      case T_CIRCUIT_REFILL:
        PostMessage(12, 120, 1, source);
        noise = 1;
        break;
      case T_AGATE_KNIFE:
        PostMessage(50, 180, 0);
        noise = 2;
        break;
      case T_EVOLUTION_TRAP:
        PostMessage(25, 180, 1, source);
        break;
      case T_CRYSTALS_500:
      case T_CRYSTALS_1000:
      case T_CRYSTALS_2000:
        PostMessage(20, 120, 1, source);
        noise = 1;
        break;
      case T_1UP:
        PostMessage(22, 120, 1, source);
        noise = training ? 2 : 3;
        break;
      case T_CURSED_SEAL:
        PostMessage(33, 180, 0);
        break;
      default:
        if (item >= T_PSI_KEY_1) PostMessage(34, 180, 2, source, artifact_names[item - T_PSI_KEY_1]);
        else PostMessage(item - T_MAP + 1, 180, 1, source);
        noise = 2;
        break;
    }
  }

  if (noise == 1) SND_Pos("dat/a/tone.wav", 128, 0);
  else if (noise == 2) SND_Pos("dat/a/crystal.wav", 128, 0);
  else if (noise == 3) SND_Pos("dat/a/crystal2.wav", 100, 0);
}

void CollectItem(t_itemStores store) {
  if (isArchipelago()) {
    CollectAPItem(store);
  } else {
    t_itemTypes item = LocalGetNextItem(store, 1);
    char source[12] = {0};

    switch (store) {
      case IS_ALPHA: sprintf(source, "Alpha cache"); break;
      case IS_BETA: sprintf(source, "Beta cache"); break;
      case IS_GAMMA: sprintf(source, "Gamma cache"); break;
      case IS_CHESTS: sprintf(source, "the chest"); break;
      default: sprintf(source, "nowhere"); break;
    }
    ProcessItem(item, source, 0);
  }
}

void CollectSpecialItem(t_specialStore itemIndex) {
  if (isArchipelago()) {
    CollectAPSpecialItem(itemIndex);
  } else {
    t_itemTypes item = LocalGetItemByIndex(IS_SPECIAL, itemIndex, 1);
    char source[16] = {0};

    switch (itemIndex) {
      case SS_PSI_KEY_1: case SS_PSI_KEY_2: case SS_PSI_KEY_3:
        sprintf(source, "the pedestal"); break;

      case SS_BOSS_PRIZE_1: sprintf(source, "Meridian's soul"); break;
      case SS_BOSS_PRIZE_2: sprintf(source, "Ataraxia's soul"); break;
      case SS_BOSS_PRIZE_3: sprintf(source, "Merodach's soul"); break;

      default: sprintf(source, "somewhere");
    }

    ProcessItem(item, source, 0);
  }
}

size_t GetNextItemIndex(t_itemStores store) {
  return isArchipelago() ? GetAPNextItemIndex(store) : LocalGetNextIndexInStore(store);
}

char HasItemByIndex(t_itemStores store, size_t index) {
  return isArchipelago() ? APHasItemByIndex(store, index) : LocalHasItemByIndex(store, index);
}

char isArchipelago() {
  return apEnabled;
}

char HasAPStatus() {
  return apStatus[0] != 0;
}

const char *GetAPStatus() {
  if (apLastStatusUpdate != 0 && time(NULL) > apLastStatusUpdate + 10) {
    apLastStatusUpdate = 0;
    apStatus[0] = 0;
  }
  return apStatus;
}

void SetAPStatus(const char *status, char important) {
  printf("AP status: %s\n", status);
  apLastStatusUpdate = important ? 0 : time(NULL);
  sprintf(apStatus, status);
}

void PostCollectNotice(const char *player, const char *itemName, const char *waswere) {
  PostMessage(72, 60, 3, itemName, waswere, player);
}

void PollAPClient() {
  if (isArchipelago()) PollServer();
}

void ReceiveItem(t_itemTypes item, const char *source) {
  ProcessItem(item, source, 0);
}

void ReportSentItem(const char *source, const char *player, const char *item) {
  PostMessage(70, 60, 3, source, player, item);
}

void SendAPSignal(t_apSignal signal) {
  if (isArchipelago()) SendAPSignalMsg(signal);
}

void KillPlayer(const char *from) {
  if (player_dying <= 0 && player_hp > 0) {
    PostMessage(73, 120, 1, from);
    player_hp = 0;
    player_dying = 1;
    SND_Pos("dat/a/playerhurt.wav", 128, 0);
    deathlink_announced = 1;
  }
}

void AnnounceDeath() {
  if (!isArchipelago() || !isDeathLink()) return;
  if (SendDeathLink() && !deathlink_announced) PostMessage(74, 120, 0);
  deathlink_announced = 0;
}

char AnnounceVictory(char winState) {
  if (isArchipelago()) return AnnounceAPVictory(winState);
  else return winState > 0;
}

void WriteStoreData() {
  if (isArchipelago()) WriteAPState();
  else LocalSaveStores();
}

void ReadStoreData() {
  if (isArchipelago()) ReadAPState();
  else LocalLoadStores();
}

void UpdateLoadingScreen(float value) {
  LoadingScreen(5, value);
}

void UpdateSavingScreen(float value) {
  SavingScreen(4, value);
}