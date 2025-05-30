# some common useful utilities shared across scripts:

import os


def FileExistsWithSize(file_):
    return os.path.isfile(file_) and os.path.getsize(file_) > 0


def pad(x):
    if len(x) == 1:
        return '0' + x
    elif len(x) == 2:
        return x
