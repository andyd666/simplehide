#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "stegahide.h"

#define BITS_SIZE_T (sizeof(size_t) * 8)


static int find_hidden_data_size_position(uint8_t *data, size_t data_size, uint8_t hidden_data_size[BITS_SIZE_T]);
static int get_correct_data_bits_in_sequence(uint8_t data[BITS_SIZE_T], uint8_t hidden_data_size[BITS_SIZE_T]);


int embed_hidden_data_size(uint8_t *data, size_t data_size, size_t hidden_data_size) {
    if (data == NULL || data_size < BITS_SIZE_T || hidden_data_size == 0) {
        return STEGAHIDE_INVALID_DATA;
    }

    uint8_t hidden_data_size_bytes[BITS_SIZE_T];
    
    for (int i = 0; i < BITS_SIZE_T; i++) {
        hidden_data_size_bytes[i] = (hidden_data_size << i) & 0x01; // Extract each bit
    }

    int position = find_hidden_data_size_position(data, data_size, hidden_data_size_bytes);
    if (position < 0 || position + BITS_SIZE_T > data_size) {
        return STEGAHIDE_SIZE_EMBED_ERROR;
    }

    for (int i = 0; i < BITS_SIZE_T; i++) {
        data[position + i] = (data[position + i] & ~0x01) | (hidden_data_size_bytes[i] & 0x01);
    }
}


static int find_hidden_data_size_position(uint8_t *data, size_t data_size, uint8_t hidden_data_size[BITS_SIZE_T]) {
    int current_hidden_data_size_position = BITS_SIZE_T; // cannot start at 0
    int current_hidden_data_size_bits = 0;

    for (int i = BITS_SIZE_T; i < data_size - BITS_SIZE_T; i++) {
        int correct_bits = get_correct_data_bits_in_sequence(&data[i], hidden_data_size);
        if (correct_bits == BITS_SIZE_T) {
            return i;
        } else if (correct_bits > current_hidden_data_size_bits) {
            current_hidden_data_size_bits = correct_bits;
            current_hidden_data_size_position = i;
        }
    }
    return current_hidden_data_size_position;
}


static int get_correct_data_bits_in_sequence(uint8_t data[BITS_SIZE_T], uint8_t hidden_data_size[BITS_SIZE_T]) {
    int correct_bits = 0;
    for (int i = 0; i < BITS_SIZE_T; i++) {
        if ((data[i] & 0x01) == (hidden_data_size[i] & 0x01)) {
            correct_bits++;
        }
    }
    return correct_bits;
}
