# CSVLens

CSVLens is a small, dependency-free C command-line tool for inspecting, previewing, filtering, selecting, and profiling CSV data with streaming memory usage.

Created by arjunrenvon — GitHub: https://github.com/Arjunren

## Features

- RFC-style quoted fields, escaped quotes, CRLF, and multiline field support
- Inspect headers, row counts, and inconsistent row widths
- Preview data, select named columns, and filter rows by a text match
- Calculate numeric count, missing/invalid values, minimum, maximum, and mean
- Stream records instead of loading complete files into memory
- Custom single-character delimiters and machine-friendly CSV output

## Screenshots

CSVLens is a terminal tool. The commands below demonstrate its output-oriented interface.

## Installation

With CMake and a C11 compiler:

```bash
cmake -S . -B build
cmake --build build --config Release
```

On Unix-like systems, `make` is also supported.

## Usage

```bash
csvlens info examples/people.csv
csvlens head examples/people.csv -n 2
csvlens select examples/people.csv --columns name,score
csvlens find examples/people.csv --column city --contains York
csvlens stats examples/people.csv --column score
```

Use `--delimiter ';'` with semicolon-delimited files. Commands return `0` on success and `2` for input, parse, or usage errors.

## Development

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

CI compiles with strict warnings on GCC and Clang.

## Project Structure

- `include/csvlens.h` — public streaming parser interface
- `src/csvlens.c` — safe dynamic CSV reader and writer
- `src/main.c` — commands and output handling
- `tests/test_csv.c` — quoted-field, multiline, and lookup tests
- `examples/people.csv` — sample data

## Security / Privacy

CSVLens processes local files, does not use a network, does not execute input, and never modifies the source CSV. Output is written only to standard output and errors to standard error.

## Contributing

Issues and pull requests are welcome. Add tests for parsing edge cases and compile with warnings treated as errors.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

## Author

**arjunrenvon**

GitHub: [@Arjunren](https://github.com/Arjunren)
