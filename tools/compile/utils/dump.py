import argparse

from pycparser import parse_file


if __name__ == "__main__":
    argparser = argparse.ArgumentParser("Dump AST")
    argparser.add_argument("filename", help="name of file to parse")
    args = argparser.parse_args()

    ast = parse_file(args.filename, use_cpp=True)
    ast.show(showcoord=False)
