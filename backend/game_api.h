#ifndef BINGOAT_GAME_API_H
#define BINGOAT_GAME_API_H

#include "civetweb.h"

int bingo_api_init(void);
void bingo_api_register(struct mg_context *server);

#endif
