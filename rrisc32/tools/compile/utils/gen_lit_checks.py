import argparse


if __name__ == "__main__":
    argparser = argparse.ArgumentParser()
    argparser.add_argument("filename")
    args = argparser.parse_args()

    with open(args.filename) as ifs:
        for line in ifs:
            line = line.rstrip()
            print(f"// CC-NEXT: {line}")
