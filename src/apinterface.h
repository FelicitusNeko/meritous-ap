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

#ifndef APINTERFACE_H
#define APINTERFACE_H

char ReadAPSettings();

void CreateAPStores();
void DestroyAPStores();

int GetAPCostFactor(t_itemStores store);
int GetAPUpgradeCost(t_itemStores store, char training);
void CollectAPItem(t_itemStores store);
void CollectAPSpecialItem(t_specialStore index);
size_t GetAPNextItemIndex(t_itemStores store);
char APHasItemByIndex(t_itemStores store, size_t index);

void ConnectAP();
void DisconnectAP();

void WriteAPState();
void ReadAPState();

void UpdateSavingScreen(float value);
void UpdateLoadingScreen(float value);

uint32_t RetrieveAPSeed();
void SendAPSignalMsg(t_apSignal signal);
void PollServer();

char isDeathLink();
char SendDeathLink();
char AnnounceAPVictory(char isFullVictory);

const char* GetSlotName();
const char* GetRoomSeed();

#endif