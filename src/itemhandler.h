// Copyright (C) 2022 FelicitusNeko
// 
// This file is part of Meritous-AP.
// 
// Meritous-AP is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Meritous-AP is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Meritous-AP.  If not, see <http://www.gnu.org/licenses/>.

#ifndef ITEMHANDLER_H
#define ITEMHANDLER_H

#include "itemdefs.h"

extern const char* enemyNames[];
extern int rooms_to_gen;
extern int rooms_to_knife;
extern float crystal_scaling;

void InitRando();

void InitStores();
void DestroyStores();

uint32_t RetrieveSeed();

void StartRando();
void EndRando();

t_itemTypes MakeCrystals();

int UpgradePrice(int t);
int CostFactor(t_itemStores store);
void CollectItem(t_itemStores store);
void CollectSpecialItem(t_specialStore itemIndex);
size_t GetNextItemIndex(t_itemStores store);
char HasItemByIndex(t_itemStores store, size_t index);

char isArchipelago();
char HasAPStatus();
const char *GetAPStatus();
void SetAPStatus(const char *status, char important);
void PostCollectNotice(const char *player, const char *itemName, const char *waswere);

void PollAPClient();
void ReceiveItem(t_itemTypes item, const char *source);
void ReportSentItem(const char *source, const char *player, const char *item);
void SendAPSignal(t_apSignal signal);
void KillPlayer(const char *from);
void AnnounceDeath();
char AnnounceVictory(char isFullVictory);

void WriteStoreData();
void ReadStoreData();

#endif