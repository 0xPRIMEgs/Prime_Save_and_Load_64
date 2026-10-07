#ifndef PRIME_SAVE_AND_LOAD_C
#define PRIME_SAVE_AND_LOAD_C

#include <libdragon.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Four fixed slots in the first 512 bytes of EEPROM (4K or larger).
 * Each slot: 'X', 'T', version 2, stat count, then little-endian u16 stats.
 * This NEW layout is NOT compatible with the old named-stat saves.
 * Saving will overwrite old-layout data, including other old slots.
 * Keep array order identical between save and load. Changing the meaning/order
 * of stats requires a format-version change, even if the count stays the same.
 * Values are clamped to 0..65535; the caller's input array is unchanged.
 * Calls are synchronous. A failed/interrupted save can invalidate that slot;
 * this is not a transactional or checksummed format.
 */
#define CHARACTER_SAVE_SLOTS       4
#define CHARACTER_SAVE_BLOCKS      16
#define CHARACTER_SAVE_BYTES       128
#define CHARACTER_SAVE_MAX_STATS   62
#define CHARACTER_SAVE_VERSION     2

/* Pass an actual array of int pointers, not a pointer to an array's first
 * element. C cannot infer the length of a pointer or malloc allocation.
 * Example:
 * int *stats[] = { &maxHP, &maxMP, &atk, &def, &spd, &exp, &lv };
 * bool saved = saveCharacter(slot, stats);
 * bool loaded = loadCharacter(slot, stats); // updates original variables
 * All pointed-to variables must be int, with valid lifetimes.
 * Use the same order and count for save and load.
 * For a dynamically allocated pointer list, call the Counted functions
 * explicitly with the number of pointers.
 */
#define saveCharacter(slot, stats) \
    saveCharacterCounted((slot), (stats), sizeof(stats) / sizeof((stats)[0]))
#define loadCharacter(slot, stats) \
    loadCharacterCounted((slot), (stats), sizeof(stats) / sizeof((stats)[0]))

bool saveCharacterCounted(int slot, int *const stats[], size_t count)
{
    if (slot < 0 || slot >= CHARACTER_SAVE_SLOTS || !stats ||
        count == 0 || count > CHARACTER_SAVE_MAX_STATS)
        return false;
    for (size_t i = 0; i < count; ++i)
        if (!stats[i]) return false;
    if (eeprom_present() == EEPROM_NONE) return false;

    uint8_t data[CHARACTER_SAVE_BYTES] = {0};
    data[0] = 'X';
    data[1] = 'T';
    data[2] = CHARACTER_SAVE_VERSION;
    data[3] = (uint8_t)count;
    for (size_t i = 0; i < count; ++i) {
        int value = *stats[i];
        if (value < 0) value = 0;
        if (value > 65535) value = 65535;
        data[4 + i * 2] = (uint8_t)((unsigned)value & 0xFFu);
        data[5 + i * 2] = (uint8_t)((unsigned)value >> 8);
    }

    int base = slot * CHARACTER_SAVE_BLOCKS;
    /* Invalidate first; publish the valid header only after all data writes. */
    uint8_t invalid[EEPROM_BLOCK_SIZE] = {0};
    if (eeprom_write(base, invalid) != 0) return false;
    size_t blocks = (4 + count * 2 + EEPROM_BLOCK_SIZE - 1) / EEPROM_BLOCK_SIZE;
    for (size_t b = 1; b < blocks; ++b) {
        if (eeprom_write(base + (int)b, data + b * EEPROM_BLOCK_SIZE) != 0)
            return false;
    }
    return eeprom_write(base, data) == 0;
}

/* Count must match the saved count exactly; false leaves stats unchanged.
 * Each slot may have a different count (1..62), without moving other slots.
 */
bool loadCharacterCounted(int slot, int *const stats[], size_t count)
{
    if (slot < 0 || slot >= CHARACTER_SAVE_SLOTS || !stats ||
        count == 0 || count > CHARACTER_SAVE_MAX_STATS)
        return false;
    for (size_t i = 0; i < count; ++i)
        if (!stats[i]) return false;
    if (eeprom_present() == EEPROM_NONE) return false;

    uint8_t data[CHARACTER_SAVE_BYTES] = {0};
    int base = slot * CHARACTER_SAVE_BLOCKS;
    eeprom_read(base, data);
    if (data[0] != 'X' || data[1] != 'T' ||
        data[2] != CHARACTER_SAVE_VERSION || data[3] != count)
        return false;

    size_t blocks = (4 + count * 2 + EEPROM_BLOCK_SIZE - 1) / EEPROM_BLOCK_SIZE;
    for (size_t b = 1; b < blocks; ++b)
        eeprom_read(base + (int)b, data + b * EEPROM_BLOCK_SIZE);

    for (size_t i = 0; i < count; ++i)
        *stats[i] = (int)((unsigned)data[4 + i * 2] |
                       ((unsigned)data[5 + i * 2] << 8));
    return true;
}

#endif
