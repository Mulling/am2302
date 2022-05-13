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

static const char *TAG = "am2302";

#include "am2302.h"

#include <driver/gpio.h>
#include <driver/rmt.h>
#include <esp_err.h>
#include <esp_log.h>
#include <stdint.h>
#include <stdlib.h>

static RingbufHandle_t ringbuf_handle = NULL;

inline __attribute__((always_inline))
void dht_init(void){
    rmt_config_t rmt_rx_config = {
        .rmt_mode      = RMT_MODE_RX,
        .channel       = CONFIG_AM2302_RMT_CHANNEL,
        .gpio_num      = CONFIG_AM2302_GPIO_PIN,
        .clk_div       = 80,
        .mem_block_num = 1,  // mem_block is 64 * uint32_t
        .rx_config = {
            .idle_threshold      = 500,
            .filter_ticks_thresh = 2, // clock source (80MHz) * 2
            .filter_en           = 1
        }
    };

    ESP_ERROR_CHECK(rmt_config(&rmt_rx_config));
    ESP_ERROR_CHECK(rmt_driver_install(CONFIG_AM2302_RMT_CHANNEL, 1024, 0));

    gpio_config_t gpio = {
        .intr_type    = GPIO_INTR_DISABLE,
        .pin_bit_mask = AM2302_GPIO_PIN_SEL,
        .mode         = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en   = true
    };

    ESP_ERROR_CHECK(gpio_config(&gpio));

    ESP_ERROR_CHECK(rmt_get_ringbuf_handle(CONFIG_AM2302_RMT_CHANNEL, &ringbuf_handle));
}

static inline __attribute__((always_inline))
esp_err_t dht_check_checksum(const uint64_t bits){
    uint8_t sum = 0x00;

    for (uint8_t i = 1; i < 5; i++)
        sum += 0xFF & (bits >> (i << 3));

    return sum == (uint8_t)(bits & 0xFF) ? ESP_OK : ESP_ERR_INVALID_CRC;
}

static inline __attribute__((always_inline))
esp_err_t dht_parse(
        const rmt_item32_t *restrict items,
        int16_t *t,
        int16_t *h){

    uint64_t bits = 0x0;

    // NOTE: not checking for durations higher than the ones in the spec ¯\_(ツ)_/¯
    // also not checking for events where two signals are high/low in sequence, i.e:
    //     __ __    __
    // ___|     |__|
    //       ^
    //       |
    //       the RMT driver should take care of these cases, resulting in a checksum fail

    for (ssize_t i = 39; i >= 0; i--){
        bits |= ((abs((uint16_t)items->duration1 - 70) <= 5) ? 1ULL : 0ULL) << i;
        items++;
    }

    esp_err_t checksum_result = dht_check_checksum(bits);

    if (checksum_result != ESP_OK){
        ESP_LOGE(TAG, "checksum fail");
        return checksum_result;
    }

    *h = (0xFFFF) & bits >> 24;
    *t = (0xFFFF) & bits >> 8;

    return checksum_result;
}

esp_err_t dht_read(int16_t *t, int16_t *h){
    esp_err_t err;

    size_t len_items = 0;

    gpio_set_level(CONFIG_AM2302_GPIO_PIN, 0);
    rmt_rx_start(CONFIG_AM2302_RMT_CHANNEL, true);
    ets_delay_us(800);
    gpio_set_level(CONFIG_AM2302_GPIO_PIN, 1);

    // NOTE: wait for 4 ticks (4ms)
    rmt_item32_t *items = (rmt_item32_t *)xRingbufferReceive(ringbuf_handle, &len_items, 4);

    rmt_rx_stop(CONFIG_AM2302_RMT_CHANNEL);

    if (len_items < 42){
        err = ESP_ERR_INVALID_SIZE;
        ESP_LOGE(TAG, "could not read sensor data");
        goto end;
    }

    if (items == NULL){
        err = ESP_ERR_INVALID_RESPONSE;
        ESP_LOGE(TAG, "could not read sensor");
        goto end;
    }

    // the items passed to the parser are aligned so values in duration1
    // are always high
    //
    //    ~80us, sensor pulls-up
    //    |
    //    |
    //    |     ~25us or ~70us, data bits aligned so that duration1 always
    //    |     points to high signal
    //    |     |
    //    v     v
    //    __    __
    // __|  |__|
    //
    // ^     ^
    // |     |
    // |     ~50us, sensor pulls-down, begin data transmission
    // |
    // ~25us, sensor pulls-down

    err = dht_parse((void *)(items + 1) + sizeof(uint16_t), t, h);

end:
    vRingbufferReturnItem(ringbuf_handle, (void *)items);

    return err;
}
