#!/usr/bin/env python3
"""DsRunner Art launcher script.

Without arguments (e.g. double-click on Windows) the GUI opens. With arguments it runs as a
command line tool, see `py dsrunner_art.py --help`.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from dsrunnerart.cli import main  # noqa: E402

if __name__ == "__main__":
    sys.exit(main())
