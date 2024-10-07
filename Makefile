#
#   Makefile
#
#   Copyright 2007, 2008 Lancer-X/ASCEAI
#
#   This file is part of Meritous.
#
#   Meritous is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Meritous is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Meritous.  If not, see <http://www.gnu.org/licenses/>.
#
LDFLAGS = `sdl2-config --libs` -lSDL2_image -lSDL2_mixer -lz -lpthread
CCFLAGS = -Os -Wall `sdl2-config --cflags` -ggdb
#CCFLAGS = -Wall `sdl2-config --cflags`
#DEFINES = -DDEBUG_KEYS

AP_INCLUDES = -Isrc/submodules/wswrap/include\
              -Isrc/submodules/json/include\
              -Isrc/submodules/websocketpp\
              -Isrc/submodules/asio/include\
              -Isrc/submodules/valijson/include\
              -Isrc/submodules
AP_LIBS     = -lssl -lcrypto 
AP_WIN_LIBS = -lwsock32 -lws2_32 -lcrypt32
AP_DEFINES2  = -DASIO_STANDALONE
AP_DEFINES = -DASIO_STANDALONE -D_WEBSOCKETPP_CPP11_THREAD_

#
OBJS = 	src/levelblit.o \
		src/mapgen.o \
		src/demon.o \
		src/gamemap.o \
		src/tiles.o \
		src/save.o \
		src/help.o \
		src/audio.o \
		src/boss.o \
		src/ending.o \
		src/gamepad.o \
		src/itemstore.o \
		src/itemhandler.o \
		src/stats.o
#
#default:	meritous
default:        debug

debug: CCFLAGS += -g -D DEBUG
debug: meritous

debug_linux: CCFLAGS += -g -D DEBUG
debug_linux: meritous_linux

# this is your cpp code that bridges between apclientpp and the game
apinterface.o: src/apinterface.cpp
		g++ -c $? -o $@ ${AP_INCLUDES} ${AP_DEFINES} ${CCFLAGS}

wswrap.o: src/submodules/wswrap/src/wswrap.cpp
		g++ -c $? -o $@ ${AP_INCLUDES} ${AP_DEFINES} ${CCFLAGS}

%.o:		%.c
		gcc -c -o $@ $? ${CCFLAGS} ${DEFINES}

meritous.res: meritous.rc
		windres $? -O coff -o $@

meritous:	${OBJS} apinterface.o meritous.res
		g++ -o build/$@ ${OBJS} apinterface.o meritous.res ${AP_LIBS} ${AP_WIN_LIBS} ${LDFLAGS}

meritous_linux: ${OBJS} apinterface.o
		g++ -o meritous ${OBJS} apinterface.o ${AP_LIBS} ${LDFLAGS}

clean:		
		rm ${OBJS} apinterface.o build/meritous.exe
#		rm ${OBJS} wswrap.o apinterface.o

