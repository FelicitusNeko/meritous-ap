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

#include <fstream>
#include <map>

#include <SDL.h>
#include <zlib.h>

#include "submodules/apclientpp/apclient.hpp"
#include "uuid.h"
#include "apitemstore.hpp"

extern "C" {
  #include "itemhandler.h"
  #include "apinterface.h"
  #include "save.h"
}

#define HIDWORD(n) (int32_t)((n >> 8) & 0xffffffff)
#define LODWORD(n) (int32_t)(n & 0xffffffff)

#define GAME_NAME "Meritous"
#define DATAPACKAGE_CACHE "datapackage.json"
#define AP_OFFSET 593000

using nlohmann::json;

APClient *ap = NULL;
bool ap_sync_queued = false;
bool ap_connect_sent = false;
double deathtime = -1;

std::vector<ItemStore*> apStores;
int nextCheckToGet = 0;

std::string server;
std::string slotname;
std::string password;
std::string seed;

int goal = 0;
bool deathlink = false;
std::vector<int> apCostScale({80, 5, 4});

const char *storeNames[] = {
  "Alpha cache",
  "Beta cache",
  "Gamma cache",
  "the chest",
  "somewhere special"
};

const char *specialStoreNames[] = {
  "the pedestal",
  "the pedestal",
  "the pedestal",
  "Meridian's soul",
  "Ataraxia's soul",
  "Merodach's soul",
  "this remote location",
  "this reclusive location"
};

void DestroyAPStores();

int64_t FRInt64() {
  return (int64_t)FRInt() << 8 | (int64_t)FRInt();
}
void FWInt64(int64_t val) {
  FWInt(HIDWORD(val));
  FWInt(LODWORD(val));
}

const char *IDToLocation(int locId) {
  if (locId >= AP_OFFSET) locId -= AP_OFFSET;
  if (locId < 96) return storeNames[locId / 24];
  else return specialStoreNames[locId - 96];
}

bool isEqual(double a, double b)
{
  return fabs(a - b) < std::numeric_limits<double>::epsilon() * fmax(fabs(a), fabs(b));
}

char ReadAPSettings() {
  std::ifstream i("meritous-ap.json");
  if (i.is_open()) {
    std::ostringstream fullserver;
    json settings;
    i >> settings;
    i.close();

    bool apEnabled = settings["ap-enable"].get<bool>();
    if (!apEnabled) return 0;

    fullserver
      << (settings["server"].is_string() ? settings["server"].get<std::string>() : (std::string)"localhost")
      << ":"
      << (settings["port"].is_number() ? settings["port"].get<int>() : 38281);
    server = fullserver.str();

    password = settings["password"].is_string() ? settings["password"].get<std::string>() : (std::string)"";
    slotname = settings["slotname"].get<std::string>();

    return 1;
  } else return 0;
}

const char* GetSlotName() {
  return slotname.c_str();
}

const char* GetRoomSeed() {
  return seed.c_str();
}

void CreateAPStores()
{
  if (apStores.size() > 0) DestroyAPStores();
  apStores.push_back(new ItemStore(24, false));
  apStores.push_back(new ItemStore(24, false));
  apStores.push_back(new ItemStore(24, false));
  apStores.push_back(new ItemStore(24, true));
  apStores.push_back(new ItemStore(8, false));
  nextCheckToGet = 0;
}

void DestroyAPStores()
{
  while (apStores.size()) {
    ItemStore *delStore = apStores.back();
    apStores.pop_back();
    delete delStore;
  }
  nextCheckToGet = 0;
}

int GetAPCostFactor(t_itemStores store)
{
  if (!apStores.size() || store < IS_ALPHA || store > IS_GAMMA) return -1;
  return apStores[store]->GetCostFactor();
}

int GetAPUpgradeCost(t_itemStores store, char training)
{
  int costFactor = GetAPCostFactor(store);
  if (costFactor < 0) return 9999999;
  else return (apCostScale[0] - training * (int)((float)apCostScale[0] / 2.f))
               * costFactor + (apCostScale[1] << costFactor)
               * (apCostScale[2] - training * (int)((float)apCostScale[2] / 2.f));
}

void InternalCollectAPItem(uint64_t location)
{
  if (apStores[location / 24]->HasItemStored(location % 24))
    ReceiveItem((t_itemTypes)(apStores[location / 24]->GetStoredItem(location % 24) % AP_OFFSET),
                storeNames[location / 24]);
  else {
    std::list<int64_t> check;
    check.push_back(location + AP_OFFSET);
    ap->LocationScouts(check);
    ap->LocationChecks(check);
  }
}

void CollectAPItem(t_itemStores store)
{
  if (!ap || !apStores.size()) return;
  int next = apStores[store]->BuyNextItem();
  if (next >= 0) {
    InternalCollectAPItem((store * 24) + next);
  } else if (apStores[store]->HasCrystalFallback()) {
    ReceiveItem(MakeCrystals(), storeNames[store]);
  }
}

void CollectAPSpecialItem(t_specialStore index)
{
  if (!ap || !apStores.size()) return;
  if ((*apStores[IS_SPECIAL])[index]) return;
  apStores[IS_SPECIAL]->MarkCollected(index);
  InternalCollectAPItem(index + 96);
}

size_t GetAPNextItemIndex(t_itemStores store)
{
  if (!ap || !apStores.size()) return -1;
  return apStores[store]->GetNextItemIndex();
}

char APHasItemByIndex(t_itemStores store, size_t index)
{
  if (!ap || !apStores.size()) return 0;
  return (char)(*apStores[store])[index];
}

void ConnectAP()
{
  std::string uri = server;
  // read or generate uuid, required by AP
  std::string uuid = get_uuid();

  deathlink = false;

  if (ap) delete ap;
  ap = nullptr;
  if (!uri.empty() && uri.find("ws://") != 0 && uri.find("wss://") != 0) uri = "ws://"+uri;

  printf("Connecting to AP, server %s\n", uri.c_str());
  if (uri.empty()) ap = new APClient(uuid, GAME_NAME);
  else ap = new APClient(uuid, GAME_NAME, uri);
  SetAPStatus("Connecting", 1);

  // load DataPackage cache
  FILE* f = fopen(DATAPACKAGE_CACHE, "rb");
  if (f) {
    char* buf = nullptr;
    size_t len = (size_t)0;
    if ((0 == fseek(f, 0, SEEK_END)) &&
        ((len = ftell(f)) > 0) &&
        ((buf = (char*)malloc(len+1))) &&
        (0 == fseek(f, 0, SEEK_SET)) &&
        (len == fread(buf, 1, len, f)))
    {
      buf[len] = 0;
      /// NOTE: deprecated
      // try {
      //   ap->set_data_package(json::parse(buf));
      // } catch (std::exception&) { /* ignore */ }
    }
    free(buf);
    fclose(f);
  }

  // set state and callbacks
  ap_sync_queued = false;
  ap->set_socket_connected_handler([](){
    // TODO: what to do when we've connected to the server
    printf("Connected, authenticating\n");
    SetAPStatus("Authenticating", 1);
  });
  ap->set_socket_disconnected_handler([](){
    // TODO: what to do when we've disconnected from the server
    printf("Disconnected\n");
    SetAPStatus("Disconnected", 1);
  });
  ap->set_room_info_handler([](){
    printf("Room info received\n");
    seed = ap->get_seed();
    std::list<std::string> tags;
    ap->ConnectSlot(slotname, password, 0b111, {"AP"}, {0,4,6});
    ap_connect_sent = true; // TODO: move to APClient::State ?
  });
  ap->set_slot_connected_handler([](const json& data){
    if (data.find("death_link") != data.end() && data["death_link"].is_boolean())
      deathlink = data["death_link"].get<bool>();
    else deathlink = false;

    if (data.find("goal") != data.end() && data["goal"].is_number_integer())
      goal = data["goal"].get<int>();
    else goal = 0;

    if (data.find("cost_scale") != data.end() && data["cost_scale"].is_array())
      apCostScale = data["cost_scale"].get<std::vector<int>>();
    else apCostScale = std::vector<int>({80,5,4}); 

    if (deathlink) ap->ConnectUpdate(false, 0b111, true, {"AP", "DeathLink"});
    ap->StatusUpdate(APClient::ClientStatus::PLAYING);
    printf("Connected and ready to go as %s\n", ap->get_player_alias(ap->get_player_number()).c_str());
    SetAPStatus("Connected", 0);
  });
  ap->set_slot_disconnected_handler([](){
    printf("Disconnected\n");
    SetAPStatus("Disconnected", 1);
    ap_connect_sent = false;
  });
  ap->set_slot_refused_handler([](const std::list<std::string>& errors){
    ap_connect_sent = false;
    if (std::find(errors.begin(), errors.end(), "InvalidSlot") != errors.end()) {
      //bad_slot(game?game->get_slot():"");
    } else {
      printf("AP: Connection refused:");
      for (const auto& error: errors) printf(" %s", error.c_str());
      printf("\n");
      SetAPStatus(errors.front().c_str(), 1);
    }
  });
  ap->set_items_received_handler([](const std::list<APClient::NetworkItem>& items) {
    auto me = ap->get_player_number();
    if (!ap->is_data_package_valid()) {
      // NOTE: this should not happen since we ask for data package before connecting
      if (!ap_sync_queued) ap->Sync();
      ap_sync_queued = true;
      return;
    }
    for (const auto& item: items) {
      auto itemname = ap->get_item_name(item.item, ap->get_player_game(item.player));
      auto sender = item.player ? (ap->get_player_alias(item.player) + "'s world") : "out of nowhere";
      auto location = ap->get_location_name(item.location);

      printf("  #%d: %s (%" PRId64 ") from %s - %s\n",
              item.index, itemname.c_str(), item.item,
              sender.c_str(), location.c_str());

      if (item.index < nextCheckToGet) {
        printf("Ignoring\n");
        continue;
      }
      nextCheckToGet = item.index + 1;

      // Note: !GetItem has item.player == me but has location == -1 which crashes due to negative array indexing
      if (item.player == me && item.location != -1) {
        auto localLocation = item.location - AP_OFFSET;
        if (!(*apStores[localLocation / 24])[localLocation % 24]) {
          printf("Not yet purchased on this save; storing\n");
          apStores[localLocation / 24]->StoreItem(localLocation % 24, item.item);
          continue;
        }
        location = IDToLocation(item.location);
      }
      
      ReceiveItem((t_itemTypes)(item.item % AP_OFFSET),
        (item.player == 0 && item.location == -2)
        ? NULL
        : (item.player == ap->get_player_number()
          ? location.c_str()
          : sender.c_str()));
    }
    fflush(stdout);
  });
  ap->set_location_info_handler([](const std::list<APClient::NetworkItem> &items) {
    auto me = ap->get_player_number();
    for (auto item: items) {
      if (item.player != me) {
        auto itemname = ap->get_item_name(item.item, ap->get_player_game(item.player));
        auto recipient = ap->get_player_alias(item.player);

        ReportSentItem(IDToLocation(item.location), recipient.c_str(), itemname.c_str());
      }
    }
  });
  ap->set_data_package_changed_handler([](const json& data) {
    // NOTE: what to do when the data package changes (probably doesn't need to change this code)
    FILE* f = fopen(DATAPACKAGE_CACHE, "wb");
    if (f) {
      std::string s = data.dump();
      fwrite(s.c_str(), 1, s.length(), f);
      fclose(f);
    }
  });
  ap->set_print_handler([](const std::string& msg) {
    printf("%s\n", msg.c_str());
  });
  ap->set_print_json_handler([](const std::list<APClient::TextNode>& msg) {
    printf("%s\n", ap->render_json(msg, APClient::RenderFormat::ANSI).c_str());
  });
  ap->set_bounced_handler([](const json& cmd) {
    if (deathlink) {
      auto tagsIt = cmd.find("tags");
      auto dataIt = cmd.find("data");
      if (tagsIt != cmd.end() && tagsIt->is_array()
          && std::find(tagsIt->begin(), tagsIt->end(), "DeathLink") != tagsIt->end())
      {
        if (dataIt != cmd.end() && dataIt->is_object()) {
          json data = *dataIt;
          if (data["time"].is_number() && isEqual(data["time"].get<double>(), deathtime)) {
            deathtime = -1;
          } else {
            KillPlayer(data["source"].is_string() ? data["source"].get<std::string>().c_str() : "???");
          }
        } else {
          printf("Bad deathlink packet!\n");
        }
      }
    }
  });
}

void DisconnectAP()
{
  if (ap) {
    delete ap;
    ap = NULL;
  }
}

void WriteAPState()
{
  int progress = 0;
  FWInt(nextCheckToGet);
  for (const auto store: apStores) {
    FWInt(store->GetCostFactor());
    for (size_t x = 0; x < store->GetLength(); x++) {
      bool hasItem = (unsigned char)store->HasItemStored(x);
      FWChar((unsigned char)(*store)[x]);
      FWChar((unsigned char)hasItem);
      if (hasItem) FWInt64(store->GetStoredItem(x));
    }
    UpdateSavingScreen((float)(++progress + 1) / (IS_MAX + 1));
  }
}

void ReadAPState()
{
  CreateAPStores();

  int progress = 0;
  nextCheckToGet = FRInt();
  for (auto store: apStores) {
    store->SetCostFactor(FRInt());
    for (size_t x = 0; x < store->GetLength(); x++) {
      auto collected = FRChar();
      if (collected) store->MarkCollected(x);
      auto hasItem = (bool)FRChar();
      if (hasItem) store->StoreItem(x, FRInt64());
    }
    UpdateLoadingScreen((float)(++progress + 1) / (IS_MAX + 1));
  }
}

uint32_t RetrieveAPSeed()
{
  if (ap && ap->get_state() >= APClient::State::ROOM_INFO) {
    auto crc = crc32(0L, NULL, 0);
    crc = crc32(crc, (const Bytef*)ap->get_seed().c_str(), ap->get_seed().length());
    return HIDWORD(crc) ^ LODWORD(crc);
  }
  else return time(NULL);
}

void SendAPSignalMsg(t_apSignal signal)
{
  if (!ap) return;
  switch (signal) {
    case APSIG_COLLECT: ap->Say("!collect"); break;
    case APSIG_RELEASE: ap->Say("!release"); break;
    default: break;
  }
}

void PollServer()
{
  if (ap) ap->poll();
}

char isDeathLink()
{
  return (char)deathlink;
}

char SendDeathLink()
{
  if (!ap || !deathlink) return 0;
  deathtime = ap->get_server_time();
  json data{
    {"time", deathtime},
    {"cause", "PSI of defeat"}, // TODO: see about adding more granular reasons
    {"source", ap->get_slot()}
  };
  ap->Bounce(data, {}, {}, {"DeathLink"});
  return 1;
}

char AnnounceAPVictory(char winState)
{
  char retval = 0;

  if (!ap || ap->get_state() != APClient::State::SLOT_CONNECTED) return 0;
  if (winState >= goal) {
    retval = 1;
    ap->StatusUpdate(APClient::ClientStatus::GOAL);
  }

  return retval;
}
