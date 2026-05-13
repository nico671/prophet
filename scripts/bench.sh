#!/usr/bin/env bash

ENGINE="./outs/prophet/prophet"
DEPTH=12
TOTAL_NODES=0
TOTAL_TIME_MS=0

# A small suite of diverse positions
FENS=(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"               # Startpos
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"    # Kiwipete
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"                              # Endgame
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"        # Tactics
)

echo "Running benchmark to depth $DEPTH..."

for FEN in "${FENS[@]}"; do
    echo "Position: $FEN"
    # Feed commands via standard input and capture the output
    OUTPUT=$(echo -e "uci\nisready\nposition fen $FEN\ngo depth $DEPTH\nisready\nquit" | $ENGINE)
    
    # Because of Iterative Deepening, the engine prints 'info depth 1', 'info depth 2', etc.
    # We grep for the specific depth we want and use 'tail -n 1' to grab the final result line.
    LAST_INFO=$(echo "$OUTPUT" | grep "^info depth $DEPTH" | tail -n 1)
    echo "Raw output for depth $DEPTH: $LAST_INFO"
    # Use awk to find the word 'nodes' and print the number immediately following it
    NODES=$(echo "$LAST_INFO" | awk '{for(i=1;i<=NF;i++) if($i=="nodes") print $(i+1)}')
    TIME=$(echo "$LAST_INFO" | awk '{for(i=1;i<=NF;i++) if($i=="time") print $(i+1)}')

    echo "Depth: $DEPTH, Nodes: $NODES, Time: ${TIME}ms"

    # Default to 0 if parsing failed to prevent script crashes
    NODES=${NODES:-0}
    TIME=${TIME:-0}



    TOTAL_NODES=$((TOTAL_NODES + NODES))
    TOTAL_TIME_MS=$((TOTAL_TIME_MS + TIME))
done

# Calculate total time in seconds and NPS using awk (avoids needing 'bc' installed)
TOTAL_TIME_SEC=$(awk "BEGIN {printf \"%.3f\", $TOTAL_TIME_MS / 1000}")
NPS=$(awk "BEGIN {if ($TOTAL_TIME_MS > 0) printf \"%.0f\", $TOTAL_NODES / ($TOTAL_TIME_MS / 1000); else print 0}")

echo "==========================="
echo "Total Time  : ${TOTAL_TIME_SEC} s"
echo "Total Nodes : $TOTAL_NODES"
echo "Bench NPS   : $NPS"
echo "==========================="