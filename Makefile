#
# Makefile for mali-memtester by Dimitar Kunchev
#
# Copyright (C) 2017 Dimitar Kunchev
# 
# Licensed under the GNU General Public License version 2.  See the file
# COPYING for details.
#

# Compiling is simple - just call make and you should get a mali-memtester executable

CC=gcc
MEMTESTER_FOLDER=memtester-4.3.0

LIBMALIPATH = -L/usr/lib/mali

LDFLAGS= -lm -lpthread -lMali -lGLESv2 -lEGL -lgbm -ldrm
CFLAGS=-c -Wall -I$(MEMTESTER_FOLDER) -I ./include/wayland -I/usr/include/drm
OBJDIR=obj

SOURCES_TC=textured-cube.c textured-cube-demo.c
OBJECTS_TC=$(addprefix $(OBJDIR)/, $(SOURCES_TC:.c=.o))

SOURCES_MT=textured-cube.c mali-memtester.c 
SOURCES_MT_ORIG=$(addprefix $(MEMTESTER_FOLDER)/, memtester.c tests.c arm-asm-helpers.S)
OBJECTS_MT=$(addprefix $(OBJDIR)/, $(SOURCES_MT:.c=.o)) $(SOURCES_MT_ORIG:.c=.o)

all: textured-cube-demo mali-memtester

mkobjdir:
	mkdir -p $(OBJDIR)

textured-cube-demo: $(OBJECTS_TC)
	$(CC) $(OBJECTS_TC) $(LDFLAGS) -o $@ $(LIBMALIPATH)

mali-memtester: $(OBJECTS_MT)
	$(CC) $(OBJECTS_MT) $(LDFLAGS) -o $@ $(LIBMALIPATH)	

$(OBJDIR)/%.o: %.c mkobjdir
	$(CC) $(CFLAGS) $< -o $@

$(MEMTESTER_FOLDER)/%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean: 
	-rm -rf $(OBJECTS_MT) $(OBJECTS_TC) textured-cube-demo
	-rmdir $(OBJDIR)

#memtester:
#	EXTRA_CC_FLAGS="-DMALI_MEMTESTER" make -C $(MEMTESTER_FOLDER) memtester.o tests.o

#memtester-clean:
#	make -C $(MEMTESTER_FOLDER)/ clean
