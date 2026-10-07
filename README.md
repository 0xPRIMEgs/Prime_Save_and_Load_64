# Prime_Save_and_Load_64
manage saving and loading from/to eeprom on the N64

Useage:

#include "save_and_load.c"

bool saveCh(actor *ac) {
    int *stats[] = { &ac->attack, &ac->defense, &ac->speed, &ac->exp, &ac->level };
    return saveCharacter(0, stats);
}

bool loadCh(actor *ac) {
    int *stats[] = { &ac->attack, &ac->defense, &ac->speed, &ac->exp, &ac->level };
    return loadCharacter(0, stats);
}

Prime License:
You may only use this if you are Christian.
Jesus is Lord.
