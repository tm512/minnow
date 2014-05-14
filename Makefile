CC = gcc
CFLAGS =
LDFLAGS =
GIT_VERSION = $(shell ./gitver.sh)
CFLAGS_ = -std=c99 -pedantic -DGIT_VERSION="\"$(GIT_VERSION)\"" $(CFLAGS) 
LDFLAGS_ = $(LDFLAGS)
OBJDIR = obj
OUT = chess

.PHONY: clean
default: debug

release: CFLAGS_ += -O2 -g0
release: $(OUT)

debug: CFLAGS_ += -O0 -g3
debug: $(OUT)

profile: CFLAGS_ += -O0 -g3 -pg
profile: LDFLAGS_ += -pg
profile: $(OUT)

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
