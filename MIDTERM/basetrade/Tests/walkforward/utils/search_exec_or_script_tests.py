#!/usr/bin/env python
"""
Test for walkforward.utils.search_exec_or_script.search_script, search_exec
search_exec -e sim_strategy
search_exec -s script_name

"""
import os
import sys
from os.path import expanduser
import argparse
import unittest

from walkforward.utils.search_exec_or_script import search_script, search_exec


class TestDateMethods(unittest.TestCase):

    def test_search_exec(self):
        HOME = os.getenv("HOME")
        path_found = search_exec("sim_strategy", list_of_additional_paths=[
                                 HOME + '/cvquant_install/'], search_deeper=False)

        self.assertEqual(HOME + '/LiveExec/bin/sim_strategy', path_found)

    def test_search_script(self):
        HOME = os.getenv("HOME")
        path_found = search_script("run_simulations.pl", list_of_additional_paths=[
                                   HOME + '/basetrade/'], search_deeper=True)

        self.assertEqual(HOME + "/basetrade/ModelScripts/run_simulations.pl", path_found)


if __name__ == "__main__":
    unittest.main()
