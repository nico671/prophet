#!/usr/bin/env bash
set -euo pipefail

ENGINE_BIN=${1:-./build/prophet/prophet}
MAX_DEPTH=${2:-5}

if [[ ! -x "$ENGINE_BIN" ]]; then
  echo "error: engine binary not found or not executable: $ENGINE_BIN" >&2
  exit 1
fi

if ! [[ "$MAX_DEPTH" =~ ^[0-9]+$ ]] || [[ "$MAX_DEPTH" -lt 1 ]]; then
  echo "error: max_depth must be a positive integer" >&2
  exit 1
fi

TESTS=$(cat <<'EOF'
Initial Position|rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1|6|20,400,8902,197281,4865609,119060324
Kiwipete Position|r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -|5|48,2039,97862,4085603,193690690
Position 3|8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1|6|14,191,2812,43238,674624,11030083
Position 4|r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1|5|6,264,9467,422333,15833292
Position 5|rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8|5|44,1486,62379,2103487,89941194
Position 6|r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10|5|46,2079,89890,3894594,164075551
EOF
)

passed=0
failed=0

echo "Running perft via UCI (max depth: $MAX_DEPTH)"

while IFS='|' read -r name fen test_max expected_csv; do
  if [[ -z "$name" ]]; then
    continue
  fi

  run_depth=$MAX_DEPTH
  if [[ "$run_depth" -gt "$test_max" ]]; then
    run_depth=$test_max
  fi

  echo "=== $name ==="
  echo "FEN: $fen"

  output=$(printf "uci\nisready\nposition fen %s\nperft %d\nquit\n" "$fen" "$run_depth" | "$ENGINE_BIN")

  suite_passed=1
  for ((depth = 1; depth <= run_depth; depth++)); do
    expected=$(echo "$expected_csv" | cut -d',' -f"$depth")
    actual=$(echo "$output" | awk -v d="$depth" '$1=="perft" && $2=="depth" && $3==d {print $5; exit}')
    nps=$(echo "$output" | awk -v d="$depth" '$1=="perft" && $2=="depth" && $3==d {print $7; exit}')
    elapsed=$(echo "$output" | awk -v d="$depth" '$1=="perft" && $2=="depth" && $3==d {print $9; exit}')

    if [[ -z "$actual" ]]; then
      echo "Depth $depth: FAIL (missing output)"
      suite_passed=0
      break
    fi

    if [[ "$actual" == "$expected" ]]; then
      echo "Depth $depth: $actual (nps $nps, time $elapsed) PASS"
    else
      echo "Depth $depth: $actual (nps $nps, time $elapsed) FAIL (expected $expected)"
      suite_passed=0
      break
    fi
  done

  if [[ "$suite_passed" -eq 1 ]]; then
    echo "$name: ✓ PASSED"
    passed=$((passed + 1))
  else
    echo "$name: ✗ FAILED"
    failed=$((failed + 1))
  fi

  echo
done <<< "$TESTS"

echo "==========================================="
echo "Results: $passed passed, $failed failed"

if [[ "$failed" -gt 0 ]]; then
  exit 1
fi
