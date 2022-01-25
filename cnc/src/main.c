#include <stdio.h>

#include "cnc.h"

int main(int argv, char **argc) {
    struct sLaika_cnc *cnc = laikaC_newCNC(13337);

    while (true) {
        if (!laikaC_pollPeers(cnc, 1000)) {
            LAIKA_DEBUG("no events!\n");
        }
    }

    laikaC_freeCNC(cnc);
    LAIKA_DEBUG("cnc killed\n");
    return 0;
}