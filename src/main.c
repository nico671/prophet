
#include "chess/board/cboard.h"
#include "engine/engine.h"
#include "engine/datagen/datagen.h"
#include "engine/uci/uci.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
    if (argc > 1 && !strcmp(argv[1], "datagen")) {
        return datagen_main(argc - 1, argv + 1);
    }
    engine_init();
    uci_loop();
    return 0;
}
