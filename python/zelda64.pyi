from typing import BinaryIO, IO, final

__version__: str


@final
class Rom:
    def __init__(self, source: BinaryIO | IO[bytes]) -> None: ...
