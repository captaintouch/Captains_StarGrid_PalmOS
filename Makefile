# Makefile
FILENAME = StarGrid
APPDEFINITION = $(FILENAME).def
SRCFILES = $(wildcard sauce/*.c) $(wildcard sauce/game/*.c)
OBJS = $(SRCFILES:.c=.o)
HIRES = false

# Palm SDK config
SDK_VERSION = 5
PALMCC = m68k-palmos-gcc
PALMINC = /opt/palmdev/sdk-5r3/include
PILRC = /usr/bin/pilrc
MULTIGEN = /usr/local/bin/m68k-palmos-multigen
BUILDPRC = /usr/local/bin/build-prc
PALMCFLAGS = -O2 -mshort -DPALMOS -DSDK_$(SDK_VERSION) \
	-I$(PALMINC) \
	-I$(PALMINC)/Dynamic \
	-I$(PALMINC)/Core \
	-I$(PALMINC)/Core/UI \
	-I$(PALMINC)/Core/Hardware \
	-I$(PALMINC)/Core/System \
	-I$(PALMINC)/Core/System/Unix \
	-I$(PALMINC)/Libraries \
	-I$(PALMINC)/Libraries/PalmOSGlue \
	-I$(PALMINC)/Libraries/Lz77 \
	-I$(PALMINC)/Libraries/ExgLocal \
	-I$(PALMINC)/Libraries/Sms \
	-I$(PALMINC)/Libraries/Pdi \
	-I$(PALMINC)/Libraries/Telephony \
	-I$(PALMINC)/Libraries/Telephony/UI \
	-I$(PALMINC)/Libraries/INet \
	-I$(PALMINC)/Extensions \
	-I$(PALMINC)/Extensions/ExpansionMgr
WARNINGFLAGS = -Wswitch -Wunused

all:
	$(MAKE) EXT="_lowres" HIRES=false build
	$(MAKE) EXT="_hires" HIRES=true PILRCFLAGS="-D PALMHIRES" GCCFLAGS="-DHIRESBUILD" build
	$(MAKE) debug

debug: 
	$(MAKE) EXT="_debug" HIRES=false GCCFLAGS="-DDEBUG" build

build: prebin bin combine cleanup

ifeq ($(HIRES), true)
prebin:
	./generateBitmaps.sh
	./generateResourceFile.sh --hires
else
prebin:
	./generateBitmaps.sh
	./generateResourceFile.sh
endif

sections.o:
	$(MULTIGEN) $(APPDEFINITION)
	$(PALMCC) -c -o sections.o $(FILENAME)-sections.s

.c.o:
	$(PALMCC) -c $(GCCFLAGS) $(PALMCFLAGS) ${WARNINGFLAGS} $<

app.out: sections.o $(OBJS)
	$(PALMCC) $(notdir $(OBJS)) sections.o $(FILENAME)-sections.ld -o app.out

bin:
	$(PILRC) $(PILRCFLAGS) resources/ui.rcp 
	$(PILRC) $(PILRCFLAGS) resources/graphicResources.rcp

combine: app.out
	$(BUILDPRC) $(APPDEFINITION) -o artifacts/$(FILENAME)$(EXT).prc app.out *.bin

cleanup:
	rm -f *.out *.bin *.o *.ld *.s || true
	rm -Rf resources/assets || true
	rm -Rf resources/hiresTMP || true
	rm -f resources/graphicResources.rcp || true
