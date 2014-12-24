CROSS = 
CC = $(CROSS)gcc
STRIP = $(CROSS)strip
CFLAGS =
LDFLAGS =
GIT_VERSION = $(shell ./gitver.sh)
CFLAGS_ = -std=c99 -pedantic -DGIT_VERSION="\"$(GIT_VERSION)\"" $(CFLAGS) 
LDFLAGS_ = $(LDFLAGS)
OBJDIR = obj
OUT = minnow

.PHONY: default release debug profile strip clean
default: release

release: CFLAGS_ += -O2 -g3
release: $(OUT)

debug: CFLAGS_ += -O0 -g3
debug: $(OUT)

profile: CFLAGS_ += -O0 -g3 -pg
profile: LDFLAGS_ += -pg
profile: $(OUT)

strip: release
	$(STRIP) $(OUT)

SRC = $(wildcard src/*.c)
HDR = $(wildcard src/*.h)
OBJ = $(patsubst src/%.c,$(OBJDIR)/%.o,$(SRC))

$(OBJDIR)/%.o: src/%.c $(HDR)
	@mkdir -p $(OBJDIR)
	$(CC) -o $@ $(CFLAGS_) -c $<

$(OUT): $(OBJ)
	$(CC) -o $(OUT) $(OBJ) $(LDFLAGS_)

clean:
	@rm -rf $(OUT) $(OBJDIR)
