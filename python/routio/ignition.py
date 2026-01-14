
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from builtins import super
from future import standard_library
standard_library.install_aliases()

try:
    # Python 3.10 changes
    from collections.abc import Mapping
except ImportError:
    from collections import Mapping

try:
    from ignition.plugin import Plugin
except ImportError:
    raise ImportError("Ignition package is required to use this module")

class Mapping(Plugin):

    def __init__(self):
        super().__init__()

    def on_program_init(self, program):
        mapping = program.auxiliary.get("remap", {})

        if not isinstance(mapping, Mapping):
            return
        maplist = []
        for k, v in mapping.items():
            maplist.append("%s=%s" % (k, v))

        program.environment["ROUTIO_MAP"] = ";".join(maplist)

