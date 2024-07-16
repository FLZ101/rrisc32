import argparse

from pycparser import parse_file

from cc import Compiler


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("i", metavar="<input_file>")
    parser.add_argument(
        "-o",
        metavar="<output_file>",
        required=False,
        help="Output file (default <input_file>.o)",
    )

    args = parser.parse_args()
    inFile = args.i
    outFile = args.o if args.o else (inFile + ".s")

    ast = parse_file(inFile, use_cpp=True)

    compiler = Compiler()
    compiler.visit(ast)

    with open(outFile, "w") as o:
        compiler.save(o)


if __name__ == "__main__":
    main()
