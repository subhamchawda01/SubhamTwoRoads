#!/media/shared/ephemeral14/anaconda2/bin/python

"""
Usage: python remove_output.py notebook.ipynb [ > without_output.ipynb ]
Modified from remove_output by Minrk

"""
import sys
import io
import os
from nbformat.current import read, write


def remove_outputs(nb):
    """remove the outputs from a notebook"""
    for ws in nb.worksheets:
        for cell in ws.cells:
            if cell.cell_type == 'code':
                cell.outputs = []


if __name__ == '__main__':
    if len(sys.argv) < 1:
        print("USAGE: <notebook> [<inline(0/1)>]")
        exit(0)

    fname = sys.argv[1]
    inline = False
    if len(sys.argv) > 2:
        inline = int(sys.argv[2]) != 0

    with io.open(fname, 'r') as f:
        nb = read(f, 'json')
    remove_outputs(nb)
    base, ext = os.path.splitext(fname)
    new_ipynb = base + ext
    if not inline:
        new_ipynb = "%s_removed%s" % (base, ext)

    with io.open(new_ipynb, 'w', encoding='utf8') as f:
        write(nb, f, 'json')
    print("wrote %s" % new_ipynb)
