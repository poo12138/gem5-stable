#-------------------------------------------------------------------------------------------------
#--  This file is a part of the GEM5 LIBRARY from LIRMM
#--  Copyright (C) 2012 Anastasiia BUTKO <butko@lirmm.fr>
#-------------------------------------------------------------------------------------------------
#-- File: trace_model.py
#-- Author: BUTKO Anastasiia
#-- Description: ...
#-------------------------------------------------------------------------------------------------

#-------------------------------------------
#-- Python classes.
#-------------------------------------------

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.util import addToPath

import os
import optparse
import sys
from os.path import join as joinpath

addToPath('../common')

import Simulation
import CacheConfig
import MemConfig
import Options
from Caches import *
from FSConfig import *
from SysPaths import *
from Benchmarks import *

#-------------------------------------------
#-- Get paths we might need.  It's expected
#--this file is in m5/configs/example.
#-------------------------------------------
config_path = os.path.dirname(os.path.abspath(__file__))
config_root = os.path.dirname(config_path)

parser = optparse.OptionParser()
Options.addCommonOptions(parser)
Options.addFSOptions(parser)

#-------------------------------------------
#-- Protocol specific options.
#-------------------------------------------
execfile(os.path.join(config_root, "common", "Options.py"))

(options, args) = parser.parse_args()

#-------------------------------------------
#-- Set Full System configuration.
#-------------------------------------------
CPUClass = TraceModel
mem_mode = 'timing'

np = 4

traces_4th_v2 = [	'../traces/hotspot/4th_32kb_1s_3ns_30ns_2ghz_0_v2.tra',
				'../traces/hotspot/4th_32kb_1s_3ns_30ns_2ghz_1_v2.tra',
				'../traces/hotspot/4th_32kb_1s_3ns_30ns_2ghz_2_v2.tra',
				'../traces/hotspot/4th_32kb_1s_3ns_30ns_2ghz_3_v2.tra']

starts_4th_v2 = [	9836891284000,
					9837357615000,
					9837461823000,
					9836888949000]

traces_4th_v3 = [	'../traces/hotspot/4th_32kb_1s_3ns_30ns_2ghz_0_v3.tra',
				'../traces/hotspot/4th_32kb_1s_3ns_30ns_2ghz_1_v3.tra',
				'../traces/hotspot/4th_32kb_1s_3ns_30ns_2ghz_2_v3.tra',
				'../traces/hotspot/4th_32kb_1s_3ns_30ns_2ghz_3_v3.tra']

starts_4th_v3 = [	9898251694000,
				9898588551500,
				9898713494500,
				9898823887500]
					
# System
sys0 = makeArmSystem(mem_mode, "RealView_PBX", SysConfig(disk='/home/n/LIRMM/dist/m5/system/disks/linux-arm-ael.img'), False)

# Trace CPUs
sys0.cpu = [TraceModel(trace_file=traces_4th_v2[i], trace_id=i, start_tick=starts_4th_v2[i], addr_shift = 0x000) for i in xrange(np)]

# Trace Arbiter
sys0.trace_arbiter = TraceArbiter();

for i in xrange(np):
	sys0.cpu[i].arbiter_port = sys0.trace_arbiter.slave
	
# I/D Cache configuration
for i in xrange(np):
	sys0.cpu[i].icache = L1Cache(size = '32kB')
	sys0.cpu[i].icache.cpu_side = sys0.cpu[i].icache_port
	sys0.cpu[i].icache.mem_side = sys0.membus.slave
	
	sys0.cpu[i].dcache = L1Cache(size = '32kB')
	sys0.cpu[i].dcache.cpu_side = sys0.cpu[i].dcache_port
	sys0.cpu[i].dcache.mem_side = sys0.membus.slave

sys0.iocache = IOCache(addr_ranges = sys0.mem_ranges)
sys0.iocache.cpu_side = sys0.iobus.master
sys0.iocache.mem_side = sys0.membus.slave

MemConfig.config_mem(options, sys0)

# Create a top-level voltage domain
sys0.voltage_domain = VoltageDomain(voltage = options.sys_voltage)
    
# Create a source clock for the system and set the clock period
sys0.clk_domain = SrcClockDomain(clock =  options.sys_clock,
            voltage_domain = sys0.voltage_domain)
            
# Hierarchy configuration
root = Root(full_system = True)
root.trace_system = sys0

#-------------------------------------------
#-- Run simulation.
#-------------------------------------------
Simulation.setWorkCountOptions(sys0, options)
Simulation.run(options, root, system, None)

