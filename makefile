# Professional Voice-to-Text Makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -std=c99 -D_POSIX_C_SOURCE=200809L \
         -D_GNU_SOURCE -D_XOPEN_SOURCE=700 -DV2T_VERSION=\"3.0.0\" \
         -Iinclude -Isrc

# Source files
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET = $(BIN_DIR)/voice2text

# Library dependencies
LIBS = -lm -lpthread -lportaudio -lsndfile -lfftw3f -lcurl

# Platform-specific settings
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LIBS += -framework CoreAudio -framework CoreFoundation -framework CoreServices
    CFLAGS += -DMACOS
    # Homebrew paths
    CFLAGS += -I/usr/local/include
    LIBS += -L/usr/local/lib
endif
ifeq ($(UNAME_S),Linux)
    LIBS += -lrt -lpulse-simple -lpulse
    CFLAGS += -DLINUX
endif

# Build flags
DEBUG_CFLAGS = -g -DDEBUG -O0 -fsanitize=address -fsanitize=undefined
RELEASE_CFLAGS = -O3 -DNDEBUG -flto

# Installation paths
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man/man1
SYSCONFDIR = /etc/voice2text

.PHONY: all debug release clean install uninstall test package

all: release

debug: CFLAGS += $(DEBUG_CFLAGS)
debug: $(TARGET)

release: CFLAGS += $(RELEASE_CFLAGS)
release: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	@echo "Build complete: $(TARGET) version $(V2T_VERSION)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) *.wav *.log
	find . -name "*.o" -delete
	find . -name "*.d" -delete

distclean: clean
	rm -rf *.tar.gz *.dSYM

install: release
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/
	install -d $(DESTDIR)$(MANDIR)
	install -m 644 docs/voice2text.1 $(DESTDIR)$(MANDIR)/
	install -d $(DESTDIR)$(SYSCONFDIR)
	install -m 644 config/voice2text.conf $(DESTDIR)$(SYSCONFDIR)/
	@echo "Installed voice2text version $(V2T_VERSION)"

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/voice2text
	rm -f $(DESTDIR)$(MANDIR)/voice2text.1
	rm -rf $(DESTDIR)$(SYSCONFDIR)
	@echo "Uninstalled voice2text"

test: debug
	$(TARGET) --test

package: release
	tar -czf voice2text-$(V2T_VERSION).tar.gz $(TARGET) README.md LICENSE docs/ config/

# Include dependency files
-include $(OBJS:.o=.d)

%.d: %.c
	@$(CC) $(CFLAGS) -MM -MT $(@:.d=.o) -o $@ $<

# Development targets
format:
	find . -name "*.c" -o -name "*.h" | xargs clang-format -i

analyze:
	scan-build make debug

valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)
