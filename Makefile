# Makefile

FILENAME = StarGrid
APPDEFINITION = $(FILENAME).def
SRCFILES = $(wildcard sauce/*.c) $(wildcard sauce/game/*.c)
OBJS = $(SRCFILES:.c=.o)
SECTIONNAME = $(FILENAME)-sections
HIRES = false

# Palm SDK config
SDK_VERSION = 5
PALMCC = m68k-palmos-gcc
PALMINC = /opt/palmdev/sdk-5r3/include
PILRC = pilrc
MULTIGEN = m68k-palmos-multigen
BUILDPRC = build-prc
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
	$(MAKE) lowres
	$(MAKE) hires
	$(MAKE) debug

lowres:
	$(MAKE) EXT="_lowres" HIRES=false build

hires:
	$(MAKE) EXT="_hires" HIRES=true PILRCFLAGS="-D PALMHIRES" GCCFLAGS="-DHIRESBUILD" build

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

$(SECTIONNAME).o:
	$(MULTIGEN) --base $(SECTIONNAME) $(APPDEFINITION)
	@$(PALMCC) -c $(SECTIONNAME).s

.c.o:
	@$(PALMCC) -c $(GCCFLAGS) $(PALMCFLAGS) ${WARNINGFLAGS} $<

$(FILENAME).out: $(SECTIONNAME).o $(OBJS)
	@$(PALMCC) $(notdir $(OBJS)) $(SECTIONNAME).o $(SECTIONNAME).ld -o $(FILENAME).out

bin:
	$(PILRC) $(PILRCFLAGS) resources/ui.rcp 
	$(PILRC) $(PILRCFLAGS) resources/graphicResources.rcp

combine: $(FILENAME).out
	mkdir -p artifacts
	$(BUILDPRC) $(APPDEFINITION) -o artifacts/$(FILENAME)$(EXT).prc $(FILENAME).out *.bin

cleanup:
	rm -f *.out *.bin *.o *.ld *.s || true
	rm -Rf resources/assets || true
	rm -Rf resources/hiresTMP || true
	rm -f resources/graphicResources.rcp || true
