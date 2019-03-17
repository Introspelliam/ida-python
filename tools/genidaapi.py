from __future__ import print_function

import sys
import string

try:
    from argparse import ArgumentParser
except:
    print("Failed to import module 'argparse'. Upgrade to Python 2.7, copy argparse.py to this directory or try 'apt-get install python-argparse'")
    raise

parser = ArgumentParser()
parser.add_argument("-i", "--input", required=True)
parser.add_argument("-o", "--output", required=True)
parser.add_argument("-m", "--modules", required=True)
args = parser.parse_args()

with open(args.input, "rb") as fin:
    with open(args.output, "wb") as fout:
        template = string.Template(fin.read())
        kvps = {
            "MODULES" : args.modules,
            "IMPORTS" : "\n".join([("from ida_%s import *" % mod) for mod in args.modules.split(",")])
            }
        fout.write(template.substitute(kvps))
