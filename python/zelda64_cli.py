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
    parser.add_argument(
        "--version",
        action="version",
        version=show_version(),
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    parser.parse_args(argv)
    return 0


if __name__ == "__main__":
    sys.exit(main())
