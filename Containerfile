FROM fedora:39

RUN dnf install -y \
    git gcc-c++ cmake ninja-build make python-devel \
    bash-completion && dnf clean all
