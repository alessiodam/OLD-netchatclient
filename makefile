# ----------------------------
# Makefile Options
# ----------------------------

NAME = TINET
ICON = icon.png
DESCRIPTION = "TINET Client"
COMPRESSED = YES
ARCHIVED = YES

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
