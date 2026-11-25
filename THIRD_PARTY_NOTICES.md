# Third-party notices

## UHO_4060_v4 opening set

- Upstream catalog: <https://github.com/official-stockfish/books>
- Local file: `testdata/openings/UHO_4060_v4.epd`
- Local SHA-256: `3f499996ff0b674a04f85f2634811d102dd53b5115841e8f11d18e1f550ba2ca`
- License published by the upstream repository: CC0 1.0 Universal
- Use: Prophet SPRT openings and the Prophet NNUE production corpus pilot.
- The local file has not been compared byte for byte with an upstream archive.

## nnue-pytorch binpack writer

- Upstream project: <https://github.com/official-stockfish/nnue-pytorch>
- Pinned commit: `86adb4b075456144d58a69398303ab9ce9ec6590`
- License: GNU General Public License version 3
- Adapted file: `src/engine/datagen/binpack_writer.c`
- Local modification: a small C17 writer for the documented compressed record and chunk format;
  it does not include the trainer or loader.
