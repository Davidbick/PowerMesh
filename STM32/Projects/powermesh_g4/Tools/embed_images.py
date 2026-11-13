import sys
from pathlib import Path


def main():
    rte = Path(sys.argv[1]).resolve()
    pe = Path(sys.argv[2]).resolve()
    output = Path(sys.argv[3])

    lines = ['.section .rodata.st8500_images,"a",%progbits']

    for name, path in [("rte", rte), ("pe", pe)]:
        if not path.is_file():
            raise FileNotFoundError(path)

        label = f"st8500_{name}"
        lines.extend([
            ".balign 4",
            f".global {label}_start",
            f".global {label}_end",
            f"{label}_start:",
            f'    .incbin "{path.as_posix()}"',
            f"{label}_end:"
        ])

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
