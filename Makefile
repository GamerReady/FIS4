# FIS4 - Frame Interpolation System for PS4
# PS4 Homebrew Makefile (OpenOrbis PS4 Toolchain)
# =================================================
# Builds FIS4.pkg for PS4 (FW 9.00+ with GoldHEN)
# Requires: OO_PS4_TOOLCHAIN environment variable

TITLE       := FIS4 - Frame Interpolation Lab
VERSION     := 1.00
TITLE_ID    := FIS400001
CONTENT_ID  := IV0000-FIS400001_00-FIS4LAB000000000

# Libraries
LIBS        := -lc -lkernel -lc++ -lSceVideoOut -lScePad -lSceSystemService -lSceUserService

# Assets
ASSETS      := $(wildcard sce_sys/assets/*)
LIBMODULES  := $(wildcard sce_module/*)

# Toolchain check
TOOLCHAIN   := $(OO_PS4_TOOLCHAIN)
PROJDIR     := $(shell pwd)
INTDIR      := $(PROJDIR)/build/ps4/x64/Debug
SRCDIR      := $(PROJDIR)/source
COMMONDIR   := $(PROJDIR)/source/common

# Source files
CFILES      := $(wildcard $(SRCDIR)/*.c)
CPPFILES    := $(wildcard $(SRCDIR)/*.cpp)
COMMONCFILES:= $(wildcard $(COMMONDIR)/*.c)
COMMONCPP   := $(wildcard $(COMMONDIR)/*.cpp)

OBJS        := $(patsubst $(SRCDIR)/%.c, $(INTDIR)/%.o, $(CFILES)) \
               $(patsubst $(SRCDIR)/%.cpp, $(INTDIR)/%.o, $(CPPFILES)) \
               $(patsubst $(COMMONDIR)/%.c, $(INTDIR)/common_%.o, $(COMMONCFILES)) \
               $(patsubst $(COMMONDIR)/%.cpp, $(INTDIR)/common_%.o, $(COMMONCPP))

# Flags
CFLAGS      := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include -DGRAPHICS_USES_FONT -DFIS4_PS4=1 -O2 -std=c11
CXXFLAGS    := $(CFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1 -std=c++17 -DFIS4_PS4=1
LDFLAGS     := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

# Create intermediate dir
_unused     := $(shell mkdir -p $(INTDIR))

UNAME_S     := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CC      := clang
	CCX     := clang++
	LD      := ld.lld
	CDIR    := linux
endif
ifeq ($(UNAME_S),Darwin)
	CC      := /usr/local/opt/llvm/bin/clang
	CCX     := /usr/local/opt/llvm/bin/clang++
	LD      := /usr/local/opt/llvm/bin/ld.lld
	CDIR    := macos
endif

# Default target
all: check_toolchain $(CONTENT_ID).pkg

check_toolchain:
ifndef OO_PS4_TOOLCHAIN
	@echo "=================================================="
	@echo " OO_PS4_TOOLCHAIN not set - PS4 build requires"
	@echo " OpenOrbis PS4 Toolchain."
	@echo " See README.md for setup instructions."
	@echo " For CI: workflow installs it automatically."
	@echo "=================================================="
	@exit 1
endif
	@if [ ! -d "$(TOOLCHAIN)/bin" ]; then echo "Toolchain not found at $(TOOLCHAIN)"; exit 1; fi

$(CONTENT_ID).pkg: pkg.gp4
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core pkg_build $< .

pkg.gp4: eboot.bin sce_sys/about/right.sprx sce_sys/param.sfo sce_sys/icon0.png $(LIBMODULES) $(ASSETS)
	$(TOOLCHAIN)/bin/$(CDIR)/create-gp4 -out $@ --content-id=$(CONTENT_ID) --files "$^"

sce_sys/param.sfo: Makefile
	@mkdir -p sce_sys
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_new $@
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

eboot.bin: $(OBJS)
	$(LD) $(INTDIR)/*.o -o $(INTDIR)/FIS4.elf $(LDFLAGS)
	$(TOOLCHAIN)/bin/$(CDIR)/create-fself -in=$(INTDIR)/FIS4.elf -out=$(INTDIR)/FIS4.oelf --eboot "eboot.bin" --paid 0x3800000000000011

$(INTDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR)/%.o: $(SRCDIR)/%.cpp
	$(CCX) $(CXXFLAGS) -o $@ $<

$(INTDIR)/common_%.o: $(COMMONDIR)/%.c
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR)/common_%.o: $(COMMONDIR)/%.cpp
	$(CCX) $(CXXFLAGS) -o $@ $<

clean:
	rm -rf build pkg.gp4 eboot.bin sce_sys/param.sfo $(CONTENT_ID).pkg
	rm -rf pkg

pc:
	$(MAKE) -f Makefile.pc

.PHONY: all clean check_toolchain pc
