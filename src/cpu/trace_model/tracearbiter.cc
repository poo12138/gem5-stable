/*
 *-------------------------------------------------------------------------------------------------
 *  This file is a part of the GEM5 LIBRARY from LIRMM
 *  Copyright (C) 2013 BUTKO Anastasiia <nastya.butko@yahoo.com>
 *-------------------------------------------------------------------------------------------------
 *  File: TraceArbiter.cc
 *  Author: Butko Anastasiia
 *-------------------------------------------------------------------------------------------------
 */

#include "base/misc.hh"
#include "base/statistics.hh"
#include "cpu/trace_model/tracearbiter.hh"
#include "dev/io_device.hh"
#include "mem/mem_object.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "mem/port.hh"
#include "mem/request.hh"

#include "sim/sim_events.hh"
#include "sim/stats.hh"
#include "sim/system.hh"

#include <stdio.h>
#include <stdlib.h>
#include <vector>

//using namespace std;

TraceArbiter::TraceArbiter(const Params *p)
    : MemObject(p),
      masterId(p->system->getMasterId(name()))
{
	// create the master ports based on the number of connected ports
    for (size_t i = 0; i < p->port_master_connection_count; ++i) {
        masterPorts.push_back(new ArbiterMasterPort(csprintf("%s-master%d", name(), i),
                                         this));
    }

    // create the slave ports based on the number of connected ports
    for (size_t i = 0; i < p->port_slave_connection_count; ++i) {
        slavePorts.push_back(new ArbiterSlavePort(csprintf("%s-slave%d", name(), i),
										this));
    }
}

BaseMasterPort &
TraceArbiter::getMasterPort(const std::string &if_name, PortID idx)
{
    if (if_name == "master" && idx < masterPorts.size()) {
        return *masterPorts[idx];
    } else {
        return MemObject::getMasterPort(if_name, idx);
    }
}

BaseSlavePort &
TraceArbiter::getSlavePort(const std::string &if_name, PortID idx)
{
    if (if_name == "slave" && idx < slavePorts.size()) {
        return *slavePorts[idx];
    } else {
        return MemObject::getSlavePort(if_name, idx);
    }
}

TraceArbiter::ArbiterMasterPort::ArbiterMasterPort(const std::string &_name, TraceArbiter *_arbiter)
    : MasterPort(_name, _arbiter), trace_arbiter(_arbiter) 
{
	warn("creating master port %s\n", _name);
}

TraceArbiter::ArbiterSlavePort::ArbiterSlavePort(const std::string &_name, TraceArbiter *_arbiter)
    : SimpleTimingPort(_name, _arbiter), trace_arbiter(_arbiter)  
{
   warn("creating slave port %s\n", _name);
}


void
TraceArbiter::init()
{
	warn("TraceArbiter init start ...\n");
	
	CPU_number = slavePorts.size();
	
	cpu_states.resize(CPU_number);
	
	for (int i = 0; i < cpu_states.size(); i++){
		cpu_states[i].thread_created = 0;
		cpu_states[i].thread_barrier = 0;
		cpu_states[i].thread_joined = 0;
		cpu_states[i].join_time = 0;
		cpu_states[i].state = 0;
	}
	
	warn("TraceArbiter init DONE\n");
}


Tick
TraceArbiter::ArbiterSlavePort::recvAtomic(PacketPtr pkt)
{
	trace_arbiter->changeState(pkt);
	trace_arbiter->updateCpuState(pkt->getSrc());
    delete pkt;
	return 1;
}

void
TraceArbiter::changeState(PacketPtr pkt)
{
	tmpData = pkt->get<uint32_t>();
	tmpSrc = pkt->getSrc();
	/*
	 * tmpData == Thread Event : 0 - thread_create; 1 - thread_barrier; 2 - thread_join
	 * flags: join,barrier,create
	 * default 	=> 000		unlock
	 * if 0 	=> 001		unlock
	 * if 1 	=> 011		lock 	=>	if all 011 => 001	unlock
	 * if 2		=> 111		lock
	 */
	warn("Thread #%d event %d at %d tick\n", tmpSrc, tmpData, curTick());
	
	if(tmpData == 0) {
		cpu_states[tmpSrc].thread_created = 1;
		cpu_states[tmpSrc].thread_barrier = 0;
		cpu_states[tmpSrc].thread_joined = 0;
	}
	else if(tmpData == 1){
		cpu_states[tmpSrc].thread_created = 1;
		cpu_states[tmpSrc].thread_barrier = 1;
		cpu_states[tmpSrc].thread_joined = 0;
	}
	else if(tmpData == 2){
		cpu_states[tmpSrc].thread_created = 1;
		cpu_states[tmpSrc].thread_barrier = 1;
		cpu_states[tmpSrc].thread_joined = 1;
		cpu_states[tmpSrc].join_time = curTick();
	}
}

void 
TraceArbiter::updateCpuState(int CPU_ID)
{
	if(cpu_states[CPU_ID].thread_joined == 0 && cpu_states[CPU_ID].thread_barrier == 0 && cpu_states[CPU_ID].thread_created == 1) {			//	001
		cpu_states[CPU_ID].state = 0;		// unlock
		sendAtomicPkt(CPU_ID);
	}
	else if (cpu_states[CPU_ID].thread_joined == 0 && cpu_states[CPU_ID].thread_barrier == 1 && cpu_states[CPU_ID].thread_created == 1)		//	011
		LockCheck(CPU_ID, 0);				// unlock/lock check
	else if (cpu_states[CPU_ID].thread_joined == 1 && cpu_states[CPU_ID].thread_barrier == 1 && cpu_states[CPU_ID].thread_created == 1)		// 	111
		LockCheck(CPU_ID, 1);				// unlock/lock check
}

void
TraceArbiter::LockCheck(int CPUPort, bool barr_join)
{
	int i = 0;
	and_flag = 1;
	
	while (and_flag) {
		if (!barr_join)		//	0 - barrier; 1 - join
			and_flag = and_flag & cpu_states[i].thread_barrier;
		else
			and_flag = and_flag & cpu_states[i].thread_joined;
		
		if (i < CPU_number - 1)
			i = i + 1;
		else {
			break;
		}
	}
	
	if(and_flag) {
		for (i = 0; i < CPU_number; i++) {
		// If barrier : falgs 011 => 001
			if(!barr_join) {			
				cpu_states[i].thread_created = 1;
				cpu_states[i].thread_barrier = 0;
				cpu_states[i].thread_joined = 0;
			}
			cpu_states[i].state = 0;		// unlock
			sendAtomicPkt(i);
		}
	}
	else {
		cpu_states[CPUPort].state = 1;		// lock
		sendAtomicPkt(CPUPort);
	}
}

void 
TraceArbiter::sendAtomicPkt(int CPUPort)
{
	Request *req = new Request();
	Request::Flags flags;
	MemCmd::Command requestType;	
	requestType = MemCmd::WriteReq;
	Addr paddr = 0x00000000;
	
	/*	
	Initializes just physical address, size, flags, and timestamp4
	size:	1 - uint8_t, 2 - uint16_t, 4 - uint32_t, 8 - uint64_t
	*/
    req->setPhys(paddr, 4, flags, 0);	

	PacketPtr pkt = new Packet(req, requestType);
	pkt->setSrc(0); 		//not used ?
	pkt->setDest(CPUPort);	//not used ?
	pkt->dataDynamicArray(new uint32_t[1]);
	pkt->setData((uint8_t*)&cpu_states[CPUPort].state);
	pkt->senderState = NULL;
	
	slavePorts[CPUPort]->sendAtomicSnoop(pkt);
}

AddrRangeList
TraceArbiter::ArbiterSlavePort::getAddrRanges() const
{
	AddrRangeList ranges;
    return ranges;
}

TraceArbiter *
TraceArbiterParams::create()
{
    return new TraceArbiter(this);
}
