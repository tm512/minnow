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

release: OPT = 3
release: DBG = 0
release: $(OUT)

debug: OPT = 0
debug: DBG = 3
debug: $(OUT)

profile: OPT = 1
profile: DBG = 3
profile: CFLAGS_ += -pg
profile: LDFLAGS_ += -pg
profile: $(OUT)

strip: release
	$(STRIP) $(OUT)

SRC = $(wildcard src/*.c)
HDR = $(wildcard src/*.h)
OBJ = $(patsubst src/%.c,$(OBJDIR)/%.o,$(SRC))

$(OBJDIR)/%.o: src/%.c $(HDR)
	@mkdir -p $(OBJDIR)
	$(CC) -o $@ $(CFLAGS_) -O$(OPT) -g$(DBG) -c $<

$(OUT): $(OBJ)
	$(CC) -o $(OUT) $(OBJ) $(LDFLAGS_)

clean:
	@rm -rf $(OUT) $(OBJDIR)
