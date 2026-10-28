# Host-side tests for the device logic. Runs natively; no ESP-IDF needed.
# The on-device build (ESP-IDF, Tab5 BSP) comes after the board arrives.

CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
CORE    := components/fleet_core

.PHONY: test clean

test: build/test_fleet_time
	./build/test_fleet_time

build/test_fleet_time: $(CORE)/fleet_time.c $(CORE)/test/test_fleet_time.c $(CORE)/include/fleet_time.h
	@mkdir -p build
	$(CC) $(CFLAGS) -I$(CORE)/include -o $@ $(CORE)/fleet_time.c $(CORE)/test/test_fleet_time.c

clean:
	rm -rf build
