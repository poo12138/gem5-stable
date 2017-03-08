/*
 *-------------------------------------------------------------------------------------------------
 *  This file is a part of the GEM5 LIBRARY from LIRMM
 *  Copyright (C) 2013 BUTKO Anastasiia <nastya.butko@yahoo.com>
 *-------------------------------------------------------------------------------------------------
 *  File: TraceArbiter.hh
 *  Author: Butko Anastasiia
 *-------------------------------------------------------------------------------------------------
 */

#ifndef __CPU_TraceArbiter_TraceArbiter_HH__
#define __CPU_TraceArbiter_TraceArbiter_HH__

#include "base/misc.hh"
#include "base/statistics.hh"
#include "dev/io_device.hh"
#include "mem/mem_object.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "mem/port.hh"
#include "mem/mport.hh"
#include "mem/tport.hh"
#include "mem/request.hh"

#include "params/TraceArbiter.hh"

#include "sim/sim_events.hh"
#include "sim/stats.hh"
#include "sim/system.hh"

#include <stdio.h>
#include <stdlib.h>
#include <vector>

//using namespace std;

class Packet;

class TraceArbiter : public MemObject
{
  public:
  
    
    typedef TraceArbiterParams Params;  
    TraceArbiter(const Params *p);
    
	virtual ~TraceArbiter() {}
	
    virtual void init();

    virtual BaseMasterPort &getMasterPort(const std::string &if_name,
                                          PortID idx = InvalidPortID);
	
	virtual BaseSlavePort &getSlavePort(const std::string &if_name,
                                          PortID idx = InvalidPortID);
	
	struct cpuState {
		bool thread_created;
		bool thread_barrier;
		bool thread_joined;
		Tick join_time;
		uint32_t state;
		};
		
	std::vector<cpuState> cpu_states;
	
	uint32_t tmpData;
	PortID tmpSrc;
	
	bool and_flag;
	
	int CPU_number;
	
	class ArbiterMasterPort : public MasterPort
    {
		TraceArbiter *trace_arbiter;

        public:

        ArbiterMasterPort(const std::string& _name, TraceArbiter *_arbiter);

        protected:
      
        bool recvTimingResp(PacketPtr pkt) { return 1; };
			
        void recvRetry() {};
			
 //       virtual Tick recvAtomic(PacketPtr pkt);
 
    };
    
    friend class ArbiterMasterPort;
    
    class ArbiterSlavePort : public SimpleTimingPort
    {
		TraceArbiter *trace_arbiter;

        public:

        ArbiterSlavePort(const std::string& _name, TraceArbiter *_arbiter);
        
        virtual ~ArbiterSlavePort()
        {}

        protected:
          
        Tick recvAtomic(PacketPtr pkt);

        AddrRangeList getAddrRanges() const;
    };
    
    friend class ArbiterSlavePort;
    
    std::vector<ArbiterMasterPort*> masterPorts;
    std::vector<ArbiterSlavePort*> slavePorts;
    
    typedef std::vector<ArbiterMasterPort*>::iterator MasterPortIter;
    typedef std::vector<ArbiterSlavePort*>::iterator SlavePortIter;
    
    MasterID masterId;
	
	void sendAtomicPkt(int CPUPort);
	
	void changeState(PacketPtr pkt);
	
	void updateCpuState(int CPU_ID);
	
	void LockCheck(int CPUPort, bool barr_join);

};

#endif 
