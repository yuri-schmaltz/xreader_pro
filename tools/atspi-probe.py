#!/usr/bin/env python3
"""Dump the AT-SPI tree (debug aid for the e2e job)."""

import sys

from dogtail.tree import root


def dump(node, depth=0):
    try:
        name = node.name
        role = node.roleName
    except Exception:
        return
    print("  " * depth + "%r [%s]" % (name, role), flush=True)
    for child in node.children:
        dump(child, depth + 1)


dump(root)