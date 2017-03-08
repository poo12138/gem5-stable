#
# -------------------------------------------------------------------------------------------------
#  This file is a part of the GEM5 LIBRARY from LIRMM
#  Copyright (C) 2012 BUTKO Anastasiia <nastya.butko@yahoo.com>
# -------------------------------------------------------------------------------------------------
#  File: TraceModel.hh
#  Author: Butko Anastasiia
# -------------------------------------------------------------------------------------------------
#

from MemObject import MemObject
from m5.params import *
from m5.proxy import *

class TraceModel(MemObject):
    type = 'TraceModel'
    trace_file = Param.String("", "File with traces")
    icache_port = MasterPort("Port to the memory system to test")
    dcache_port = MasterPort("Port to the memory system to test")
    arbiter_port = MasterPort("Port to the trace arbiter")
    trace_id = Param.Int(0, "CPU identifier")
    start_tick = Param.Tick(1000, "Number of simulation cycles")
    addr_shift = Param.Addr(0, "Shift init")
    create_shift = Param.Tick(0, "Shift of thread_create")
    system = Param.System(Parent.any, "System we belong to")

    def addPrivateSplitL1Caches(self, ic, dc, iwc = None, dwc = None):
        self.icache = ic
        self.dcache = dc
        self.icache_port = ic.cpu_side
        self.dcache_port = dc.cpu_side
        self._cached_ports = ['icache.mem_side', 'dcache.mem_side']
        if iwc and dwc:
            self.itb_walker_cache = iwc
            self.dtb_walker_cache = dwc
            self.itb.walker.port = iwc.cpu_side
            self.dtb.walker.port = dwc.cpu_side
            self._cached_ports += ["itb_walker_cache.mem_side", "dtb_walker_cache.mem_side"]
        else:
            self._cached_ports += ["itb.walker.port", "dtb.walker.port"]

