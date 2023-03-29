# ----------------------------
# Makefile Options
# ----------------------------

NAME = TINET
ICON = icon.png
DESCRIPTION = "TI-84 Plus CE Net Client"
COMPRESSED = YES
ARCHIVED = YES

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev --makefile)
