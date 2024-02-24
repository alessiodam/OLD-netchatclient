# ----------------------------
# Makefile Options
# ----------------------------

NAME = NETCHAT
ICON = icon.png
DESCRIPTION = "NETCHAT Client"
COMPRESSED = YES
ARCHIVED = YES
COMPRESSED_MODE = zx0
OUTPUT_MAP = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------
include $(shell cedev-config --makefile)
