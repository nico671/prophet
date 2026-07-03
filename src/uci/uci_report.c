#include "uci/uci_report.h"
#include "engine/engine.h"
#include "movegen/move.h"

#include <stdio.h>
#include <string.h>

static void move_to_uci_string(Move move, char* out)
{
    if (move_get_from_square(move) == NO_SQUARE
        || move_get_to_square(move) == NO_SQUARE) {
        strcpy(out, "0000");
        return;
    }

    out[0] = (char)('a' + (move_get_from_square(move) % 8));
    out[1] = (char)('1' + (move_get_from_square(move) / 8));
    out[2] = (char)('a' + (move_get_to_square(move) % 8));
    out[3] = (char)('1' + (move_get_to_square(move) / 8));

    if (move_is_promotion(move)) {
        PieceType promo = move_get_promotion_piecetype(move);
        char promo_char = 'q';
        if (promo == KNIGHT)
            promo_char = 'n';
        else if (promo == BISHOP)
            promo_char = 'b';
        else if (promo == ROOK)
            promo_char = 'r';
        else
            promo_char = 'q';

        out[4] = promo_char;
        out[5] = '\0';
        return;
    }

    out[4] = '\0';
}

void* uci_search_reporter_worker(void* arg)
{
    SearchReport* report = (SearchReport*)arg;

    for (;;) {
        pthread_mutex_lock(&report->mutex);
        while (!report->has_update && !report->abort) {
            pthread_cond_wait(&report->cond, &report->mutex);
        }

        if (report->abort) {
            pthread_mutex_unlock(&report->mutex);
            break;
        }

        SearchReportUpdate update = report->update;
        bool finished = report->is_finished;
        report->has_update = false;
        pthread_mutex_unlock(&report->mutex);

        if (update.depth > 0) {
            if (update.is_mate) {
                printf("info depth %d score mate %d nodes %lld time "
                       "%lld nps "
                       "%lld pv %s\n",
                    update.depth, update.mate_moves, update.nodes,
                    update.elapsed_ms, update.nps, update.pv);
            } else {
                printf("info depth %d score cp %d nodes %lld time "
                       "%lld nps "
                       "%lld pv %s\n",
                    update.depth, update.score_cp, update.nodes,
                    update.elapsed_ms, update.nps, update.pv);
            }
            fflush(stdout);
        }

        if (finished) {
            char best_move_uci_string[6];
            move_to_uci_string(update.best_move, best_move_uci_string);
            if (move_get_from_square(update.ponder_move) != NO_SQUARE
                && move_get_to_square(update.ponder_move) != NO_SQUARE) {
                char ponder_move_uci_string[6];
                move_to_uci_string(update.ponder_move, ponder_move_uci_string);
                printf("bestmove %s ponder %s\n", best_move_uci_string,
                    ponder_move_uci_string);
            } else {
                printf("bestmove %s\n", best_move_uci_string);
            }
            fflush(stdout);
            break;
        }
    }

    return NULL;
}