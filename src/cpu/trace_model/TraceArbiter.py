#
# -------------------------------------------------------------------------------------------------
#  This file is a part of the GEM5 LIBRARY from LIRMM
#  Copyright (C) 2013 BUTKO Anastasiia <nastya.butko@yahoo.com>
# -------------------------------------------------------------------------------------------------
#  File: Arbiter.py
#  Author: Butko Anastasiia
# -------------------------------------------------------------------------------------------------
#

from MemObject import MemObject
from m5.params import *
from m5.proxy import *

class TraceArbiter(MemObject):
    type = 'TraceArbiter'
    master = VectorMasterPort("vector port for connecting slaves")
    slave = VectorSlavePort("vector port for connecting masters")
    system = Param.System(Parent.any, "System we belong to")

