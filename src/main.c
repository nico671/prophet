
#include "chess/board/cboard.h"
#include "engine/engine.h"
#include "engine/uci/uci.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    engine_init();
    uci_loop();
    return 0;
}
