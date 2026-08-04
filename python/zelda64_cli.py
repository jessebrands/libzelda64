import argparse
import sys
from argparse import RawTextHelpFormatter

import zelda64


def show_version() -> str:
    return (f"zelda64 {zelda64.__version__}\n\n"
            f"Copyright (C) 2026  Jesse Gerard Brands\n"
            f"This is free software; see the source for copying conditions. There is NO\n"
            f"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="zelda64",
        description="Nintendo 64 Zelda ROM utility",
        formatter_class=RawTextHelpFormatter,
    )
    parser.add_argument("--version",
                        action="version",
                        version=show_version())
    parser.add_argument("-i", "--info", action="store_true", help="show ROM information")
    parser.add_argument("rom", help="input ROM")
    return parser


def run_info(options: argparse.Namespace) -> int:
    with open(options.rom, "rb") as source:
        rom = zelda64.Rom(source)
        print(rom)
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    options = parser.parse_args(argv)

    if not options.info:
        parser.error("no command given")

    try:
        return run_info(options)
    except OSError as error:
        print(f"zelda64: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
