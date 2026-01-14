CC      := gcc
CFLAGS  := -Wall -Wextra -Werror $(shell pkgconf --cflags libmpdclient sqlite3)
LDFLAGS := $(shell pkgconf --libs libmpdclient sqlite3)
SRC_DIR := src
OBJ_DIR := obj
TARGET  := mpd_stats

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

install: $(TARGET)
	systemctl --user stop mpd_stats.service
	sudo cp $(TARGET) /usr/bin/$(TARGET)
	mkdir -p ~/.config/systemd/user
	cp mpd_stats.service ~/.config/systemd/user/mpd_stats.service
	sudo systemctl daemon-reload
	systemctl --user enable mpd_stats.service
	systemctl --user start mpd_stats.service

