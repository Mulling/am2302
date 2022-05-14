// Copyright (C) 2022 Lucas Mulling

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef AM2302_H
#define AM2302_H

#include <stdint.h>
#include <esp_err.h>

#define AM2302_GPIO_PIN_SEL ((1ULL << CONFIG_AM2302_GPIO_PIN))

// NOTE: am2302_init needs to run on the same core as dht_read
void am2302_init();
esp_err_t am2302_read(int16_t *t, int16_t *v);

#endif
