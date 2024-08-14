import os
import re
import argparse
import subprocess

from typing import Optional

comment_prefix = None

class Run:
    def __init__(self, cmd: str, check: str) -> None:
        self.cmd = cmd
        self.check = check

    def __repr__(self) -> str:
        return "%s --- %s" % (self.cmd, self.check)


def get_run(line: str) -> Optional[Run]:
    m = re.fullmatch(f" *{re.escape(comment_prefix)} +RUN:(.+)", line)
    if not m:
        return None
    cmd = m.group(1).strip()

    i = cmd.rfind("|")
    if i != -1:
        last = cmd[i + 1 :].strip()
        if last.startswith("filecheck "):
            cmd = cmd[:i].strip()
            check = "CHECK"
            m = re.search("--check-prefix=([^ ]+)", last)
            if m:
                check = m.group(1)
            return Run(cmd, check)
    return Run(cmd, None)


def is_check(line: str, check: str):
    m = re.fullmatch(f" *{re.escape(comment_prefix)} +%s(-[A-Z]+)?:(.+)" % check, line)
    return m is not None


def gen_checks(infile, run: Run) -> list[str]:
    tmpfile = infile + ".tmp"
    current_dir = os.path.dirname(infile)
    cmd = run.cmd
    cmd = cmd.replace(r"%s", infile)
    cmd = cmd.replace(r"%t", tmpfile)
    cmd = cmd.replace(r"%S", current_dir)

    checks = []
    if not run.check:
        subprocess.run(["/bin/bash", "-c", cmd])
    else:
        cp = subprocess.run(
            ["/bin/bash", "-c", cmd], stdout=subprocess.PIPE, encoding="utf-8", check=True
        )
        for line in cp.stdout.splitlines():
            line = line.expandtabs(8)
            checks.append(f"{comment_prefix} {run.check}-NEXT: {line}")
    return checks


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("infile")
    args = parser.parse_args()

    infile = args.infile
    lines:list[str] = []
    with open(infile) as ifs:
        for line in ifs:
            line = line.rstrip()
            lines.append(line)

    global comment_prefix

    for line in lines:
        if line == '':
            continue
        idx = line.find(' RUN:')
        if idx != -1:
            comment_prefix = line[:idx].strip()
        break
    assert comment_prefix

    runs: list[Run] = []
    for line in lines:
        if line == "":
            continue
        run = get_run(line)
        if run:
            runs.append(run)
        else:
            break

    def _filter(check: str):
        nonlocal lines
        lines = list(filter(lambda _: not is_check(_, check), lines))

    for run in runs:
        if run.check:
            _filter(run.check)

        checks = gen_checks(infile, run)
        if len(checks) > 0:
            lines.append("")
            lines.extend(checks)

    with open(infile, "w") as ofs:
        for line in lines:
            print(line, file=ofs)


if __name__ == "__main__":
    main()
