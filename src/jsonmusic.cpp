// Copyright (C) 2025 FelicitusNeko
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
#include <filesystem>
#include <nlohmann/json.hpp>

extern "C" {
    #include "jsonmusic.h"
}

using nlohmann::json;

json music_manifest;

char LoadMusicManifest() {
    printf("loading manifest\n");
    std::ifstream i("dat/m/manifest.json");
    if (i.is_open()) {
        i >> music_manifest;
        i.close();
        return 1;
    } else {
        printf("Failed to load music manifest, check dat/m/manifest.json\n");
        return 0;
    }
}

const char *GetTrack(const char *name) {
    printf("%s %s\n", name, music_manifest[name].get<std::string>().c_str());
    return music_manifest[name].get<std::string>().c_str();
}