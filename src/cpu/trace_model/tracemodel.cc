/*
 *-------------------------------------------------------------------------------------------------
 *  This file is a part of the GEM5 LIBRARY from LIRMM
 *  Copyright (C) 2012 BUTKO Anastasiia <nastya.butko@yahoo.com>
 *-------------------------------------------------------------------------------------------------
 *  File: TraceModel.cc
 *  Author: Butko Anastasiia
 *-------------------------------------------------------------------------------------------------
 */

#include <cmath>
#include <iomanip>
#include <set>
#include <string>
#include <vector>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>

#include "base/misc.hh"
#include "base/statistics.hh"
#include "cpu/trace_model/tracemodel.hh"
#include "debug/TraceModelFlag.hh"
#include "debug/TraceMissDelay.hh"
#include "debug/LoosedPkts.hh"

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

using namespace std;

int TRACE_MODEL=0;

TraceModel::TraceModel(const Params *p)
    : MemObject(p),
      tickEvent(this),
      generatePktEvent(this),
      deleteEventEvent(this),
      icachePort("icache_port", this),
      dcachePort("dcache_port", this),
      arbiterPort("arbiter_port", this),
      traceFile(p->trace_file),
      traceId(p->trace_id),
      startTick(p->start_tick),
      addrShift(p->addr_shift),
      thread_create_shift(p->create_shift),
      masterId(p->system->getMasterId(name()))
{
    // set up counters
    schedule(tickEvent, startTick);

    id = TRACE_MODEL++;
}

void
TraceModel::schedule_call(Tick tmp_local)
{
	//if (!tickEvent.scheduled()) {
	
	//warn("%d schedule %d next %d", systemID, curTick(), curTick() + tmp_local);
	schedule(tickEvent, curTick() + tmp_local);
	//}
	
//	if (wait_for_lock) {
//		if(stateFlagsCnt == 0)
//			sendAtomicPkt();
//		wait_for_lock = false;
//	}
}
void
TraceModel::init()
{
	tmpValue = 0;
	latency = 1;
	progress = 0;
	losed = 0;
	recived = 0;
	sent = 0;
	diff = 0;
	
	shift = 0;
	
    /* Trace file param init	*/
    openInputFile(traceFile);
    eof = false;
    
    /* CPU state */
    
    stateFlagsCnt = 0;
    state = 0;
    thread_line = 0;
    
    stateFlags.done = 1;
	stateFlags.thread_event = 0;
	state_flags.push_back(stateFlags);
	
    /* CPU ID */
	string str = name();
	str = str.substr(str.find(".") + 4, str.size());
	systemID = atoi(str.c_str());
	
    /* Packet Struct init		*/	//Remove (erase function inside of the getValue) 
    tracePkt.cycle = 0;
	tracePkt.value = (uint32_t)0;
	tracePkt.addrDst = 0x0;
	tracePkt.pkt_size = 0;	
	tracePkt.ready = false;
	tracePkt.system = 0;
	tracePkt.traceID = 0;
	tracePkt.ifetch = false;
	tracePkt.isread = false;
	tracePkt.missDelay = 0;

	satisfyCpu = true;
	wait_for_lock = false;
	barrier_event = false;
	sent_flag = false;
	correct_time = false;
	trace_response = 0;
	
	thread_create_start = 0;
	create_shift_flag = true;
	
	setEvent();
	
#if 0
	getValue();	
	
	/** Event Queue **/

	events.done = false;
	events.tickCycle = tracePkt.cycle;
	events.nextCycle = tracePkt.missDelay;
	events.type = 0;
	vector_events.push_back(events);
#endif

	//warn("systemID: %d masterId: %d\n", systemID, masterId);
	virtual_cycle = 0;
	iEvent = 0;
}

/** Main loop **/

void
TraceModel::tick()
{
	//if(systemID)
	//	warn("Tick #%d: %d", systemID, curTick());
	
	if (curTick() == vector_events[iEvent].tickCycle){
//14.02		if(iEvent)
//14.02			deleteEvent();
		generatePkt();
		sent_flag = true;
		sent = sent + 1;
		vector_events[0].done = true;
		processEvent();

		//if (vector_events[0].done)
		//	vector_events.erase(vector_events.begin());
# if 0
		if (!eof) {
			//tmp = vector_events[iEvent].tickCycle - curTick();		// Генерация следующего события если нет ответа
			//losed = losed + 1;
			
			//warn("%d Tick current %d next: %d\n", traceId, curTick(), tmp);
			if (!tickEvent.scheduled())
				schedule(tickEvent, curTick());
		}
		//else if(traceId == lastReq)
			//exitSimLoop("Trace CPU completed simCycles");
#endif
# if 0
	else {
		warn("Lost pkt");
		//exitSimLoop("Trace CPU completed simCycles");
		//Delete Event whith losed packet
		//At this stage iEvent will be 0
		deleteEvent();
		losed = losed + 1;
		processEvent();
		// Check delay
		diff = curTick() - vector_events[iEvent].tickCycle;
		tmp_shift = curTick() - vector_events[iEvent].tickCycle;
		//warn("%d Tick losed %d shift: %d diff: %d Next send: %d \n", traceId, curTick(), tmp_shift, diff, vector_events[iEvent].tickCycle)
		if (tmp_shift < 0)	{
			tmp = vector_events[iEvent].tickCycle - curTick();
			//warn("%d Cur: %d vector tick: %d\n", traceId, curTick(), vector_events[iEvent].tickCycle);
			if (!tickEvent.scheduled())
				schedule(tickEvent, curTick() + tmp);
		}
		else {
			//Correct global delay variable
			shift = shift + tmp_shift;
			
			//At this stage iEvent will be 1, insert Next Event
			generatePkt();
			vector_events[0].done = true;
			
			//Correct current Event for reponse waiting
		    vector_events[0].tickCycle = vector_events[0].tickCycle + tmp_shift;
			vector_events[0].nextCycle = vector_events[0].nextCycle + tmp_shift;
			//warn("%d tickCycle: %d nextCycle: %d\n", traceId, vector_events[0].tickCycle, vector_events[0].nextCycle);
			vector_events[1].tickCycle = vector_events[1].tickCycle + tmp_shift;
			vector_events[1].nextCycle = vector_events[1].nextCycle + tmp_shift;
			
			//Schedule next Tick() in the same way;
			
			if (!tickEvent.scheduled())
				schedule(tickEvent, curTick() );	
		}
		DPRINTF(LoosedPkts, "loosed: %d shift: %d\n", losed, tmp_shift);
#endif	
	}
}

void
TraceModel::setEvent()
{
	while (getValue())
		{};
	
	events.done = false;
	events.tickCycle = tracePkt.cycle;
	events.nextCycle = tracePkt.missDelay;
	events.type = 0;
	vector_events.push_back(events);
}

void
TraceModel::processEvent()
{
	if (!eof) {
		while (getValue())
			{};
			
		insertEvent(tracePkt.cycle, 0, tracePkt.missDelay);
		iEvent = iEvent + 1;
	}
}

void
TraceModel::insertEvent(Tick tickTime, int eventType, Tick nextTime)
{
	events.done = false;
	events.tickCycle = tickTime;
	events.type = eventType;
	events.nextCycle = nextTime;
	vector_events.push_back(events);
	std::sort(vector_events.begin(), vector_events.end());
}

void
TraceModel::deleteEvent()
{
	vector_events.erase(vector_events.begin());
	iEvent = iEvent - 1;
}
/* Generate packet */
void
TraceModel::generatePkt()
{
	if (tracePkt.isread) {
		generateReadPkt();
	}
	else
		generateWritePkt();
}

void
TraceModel::generateWritePkt()
{
   
	Request *req = new Request();
	Request::Flags flags;
	
	MemCmd::Command requestType;
	if (!tracePkt.type.compare("WriteReq"))
		requestType = MemCmd::WriteReq;
	else
		requestType = MemCmd::StoreCondReq;
			
	

	req->setPhys(tracePkt.addrDst, tracePkt.pkt_size, flags, masterId-6);

	PacketPtr pkt = new Packet(req, requestType);
	
	///pkt->setSrc(masterId); 	//not used ?
	///pkt->setDest(1);			//not used ?
	
    pkt->dataDynamicArray(new uint32_t[8]);
	
	//pkt->dataDynamicArray(new uint8_t[req->getSize()]);
    pkt->setData((uint8_t*)&tracePkt.value);
    pkt->senderState = NULL;
    
    assert(pkt->needsResponse());
	assert(pkt->isRequest());
	assert(pkt->isWrite());
	
	if (!tracePkt.type.compare("WriteReq")){
		assert(pkt->needsExclusive());
		assert(pkt->hasData());
	}
		
    sendPkt(pkt);
}

void
TraceModel::generateReadPkt()
{
	Request *req = new Request();
	Request::Flags flags;
	
	if (tracePkt.ifetch)
		flags.set(Request::INST_FETCH);
		
	MemCmd::Command requestType;	
	if (!tracePkt.type.compare("ReadReq"))
		requestType = MemCmd::ReadReq;
	else
		requestType = MemCmd::LoadLockedReq;
		
	req->setPhys(tracePkt.addrDst, tracePkt.pkt_size, flags, masterId-6);
	if (tracePkt.pkt_size < 4)	
		req->setPhys(tracePkt.addrDst, 4, flags, masterId-6);
	
	///req->setThreadContext(id,0);
	
	req->setThreadContext(masterId-6,masterId-6);
	
	PacketPtr pkt = new Packet(req, requestType);

    ///pkt->dataDynamicArray(new uint32_t[8]);

	///pkt->setSrc(masterId); 	//not used ?
	///pkt->setDest(1);	//not used ?

    pkt->dataDynamicArray(new uint8_t[tracePkt.pkt_size]);

    pkt->senderState = NULL; 
    
	assert(pkt->needsResponse());
	assert(pkt->isRequest());
	assert(pkt->isRead());
	
	if (!tracePkt.type.compare("LoadLockedReq"))
		assert(pkt->isLLSC());
	
    sendPkt(pkt);
//    completeRequest(pkt);
}

void
TraceModel::doRetry()
{
    if (icachePort.sendAtomic(retryPkt)) {
        retryPkt = NULL;
    }
}

void
TraceModel::completeRequest(PacketPtr pkt)
{
	Request *req = pkt->req;
	if(pkt->isRead())	{
		tmpValue = pkt->get<uint32_t>();
	}
	
	assert(pkt->isResponse());
    delete req;
    delete pkt;
    
  
    if(iEvent) {
		
		tmp_shift = curTick() - vector_events[0].nextCycle;
		trace_response = vector_events[0].nextCycle;
		
		if (tmp_shift > 0)
			DPRINTF(TraceModelFlag, "cur %d next %d delay %d \n", curTick(), vector_events[0].nextCycle, tmp_shift);
		else if (tmp_shift == 0)
			DPRINTF(TraceModelFlag, "in time %d\n", tmp_shift);
		else if (tmp_shift < 0)
			DPRINTF(TraceModelFlag, "in advance %d\n", tmp_shift);
		deleteEvent();
		sent_flag = false;
#if 1
//14.02		if (curTick() != vector_events[0].tickCycle) {
			//warn("Next %d Delay %d\n", vector_events[0].tickCycle, vector_events[0].nextCycle);
			//warn("%d Recieved s usloviem 2 %d \n", traceId, curTick());

/*		if (curTick() > (thread_create_start + shift) && !create_shift_flag && systemID > 1) {
			shift = shift + thread_create_shift * systemID;
			create_shift_flag = true;
			warn("Shift replicated traces %d", systemID);
		}
*/		
		recived = recived + 1;
		shift = shift + tmp_shift;
			
		if (create_shift_flag)	{
		/** 1-thread replication **/
			shift = shift + thread_create_shift * systemID;
		/**/
		
		/**	2-thread replication 
			if(systemID > 1) {
				if((int)systemID % 2 == 0) {
					shift = shift + thread_create_shift * systemID;
				}
				else {
					shift = shift + thread_create_shift * 2;
				}
			}
		/** **/
		
		/**	4-thread replication 
			if(systemID > 3) {
				if((int)systemID % 4 == 0) {
					shift = shift + thread_create_shift * systemID;
				}
				else {
					shift = shift + thread_create_shift * 4;
				}
			}
		/** **/
			create_shift_flag = false;
			warn("cpu # %d cycle shift=%d\n", systemID, shift);
		}
		
		vector_events[0].tickCycle = vector_events[0].tickCycle + tmp_shift;
		vector_events[0].nextCycle = vector_events[0].nextCycle + tmp_shift;
		
		tmp_global = vector_events[0].tickCycle - curTick();
		tmp_global_tick = curTick();
		
		if(!eof) {
			schedule_call(tmp_global);
		
			if (wait_for_lock) {
				for (int i = 0; i <= stateFlagsCnt; i++)
					state_flags[i].thread_time = state_flags[i].thread_time + tmp_shift;
				sendAtomicPkt();
				wait_for_lock = false;
			}	
		}
#endif
	}
}

/* Trace file proccess */

void
TraceModel::openInputFile(const string &filename)
{
	infile.open(filename.c_str());
}

bool
TraceModel::getValue()
{
	size_t pos_start, pos_end;
	string str;
	erase(&tracePkt);

	if(!eof && satisfyCpu) {
		getline(infile, data);
		
		/* Check for pkt duplicates */
		if (data.find("resp") < 100) {
			getline(infile, data);
		}
		/* Check for synchronization traces */
		if(data.find("thread") < 100) {
			pos_end = data.find(":");
			str = data.substr(0, pos_end);
			stateFlags.thread_time = atol(str.c_str()) + shift;

			pos_start = data.find("thread") + 7;
			pos_end = data.find("\n");
			str = data.substr(pos_start, pos_end - pos_start);
			if (!str.compare("create")){
				//warn("thread %d create at %d \n", systemID, stateFlags.thread_time);
				stateFlags.thread_event = 0;
			}
			else if (!str.compare("barrier")){
				//warn("outside thread %d barrier at %d \n", systemID, stateFlags.thread_time);
				stateFlags.thread_event = 1;
			}
			else if(!str.compare("join")){
				//warn("thread %d join at %d\n", systemID, stateFlags.thread_time);
				stateFlags.thread_event = 2;
			}
			
			if (state_flags[stateFlagsCnt].done) {
				if (stateFlags.thread_event)
					state_flags[stateFlagsCnt].done = 0;
				state_flags[stateFlagsCnt].thread_event = stateFlags.thread_event;
				state_flags[stateFlagsCnt].thread_time = stateFlags.thread_time;
			}
			else {
				stateFlags.done = 0;
				state_flags.push_back(stateFlags);
				stateFlagsCnt = stateFlagsCnt + 1; 
				//warn("push_back %d event %d", stateFlagsCnt, stateFlags.thread_event);
			}
			
			//if (!stateFlagsCnt)
			wait_for_lock = true;
			//sendAtomicPkt();
				
			thread_line = true;
		}
		else {
			thread_line = false;
		}
	}
	
	if(data.find("!") < 10)	{
		eof = true;
		warn("End of file cpu # %d. Tick: %d Number of sent pkts: %d Number of received pkts: %d, Global shift: %d", systemID, curTick(), sent-1, recived, shift);
		infile.close();
		return 0; 
	}
	else if(!thread_line){
		
		/** Trace Format
		 *  2977169020000: system.cpu.icache: cmd: ReadReq f: 1 1 a: 570280 s: 4 v: 3947020032 
		 *  2977169051000: system.cpu.icache: resp
		 *  **/
		
		/* Get Packet Cycle */
		pos_end = data.find(":");
		str = data.substr (0, pos_end);
		tracePkt.cycle = atol(str.c_str()) + shift;
			
		/* Get Packet CPU ID */
		pos_start = pos_end + 12;
		pos_end = data.find("cache") - 2;
		str = data.substr(pos_start, pos_end - pos_start);
		tracePkt.traceID = atoi(str.c_str());
				
		/* Get Packet Source */ //chaneged to bool
		pos_start = pos_end + 1;
		pos_end = pos_end + 2;
		tracePkt.src = data.substr(pos_start, pos_end - pos_start);
		
		/* Get Request Type */
		pos_start = data.find("cmd") + 5;
		pos_end = data.find("Req") + 3;
		tracePkt.type = data.substr(pos_start, pos_end - pos_start);
		
		/* I Fetch ? */
		pos_start = data.find("f:") + 3;
		pos_end = pos_start + 1;
		str = data.substr(pos_start, pos_end - pos_start);
		if (atoi(str.c_str()))
			tracePkt.ifetch = true;
		else
			tracePkt.ifetch = false;
		
		/* IsRead ? */
		pos_start = pos_end + 1;
		pos_end = pos_start + 1;
		str = data.substr(pos_start, pos_end - pos_start);
		if (atoi(str.c_str()))
			tracePkt.isread = true;
		else
			tracePkt.isread = false;
		
		/* Get Request Address */	
		pos_start = data.find("a:") + 3;
		pos_end = data.find("s:") - 1;
		str = data.substr(pos_start, pos_end - pos_start);
		sscanf(str.c_str(), "%x", &tracePkt.addrDst);

		if (systemID > 0 && tracePkt.addrDst < 0xfd00000 && !tracePkt.src.compare("d"))
			tracePkt.addrDst = (systemID) * addrShift + tracePkt.addrDst;
		else if (systemID > 0 && tracePkt.addrDst >= 0xfd00000 && !tracePkt.src.compare("d"))
			tracePkt.addrDst = tracePkt.addrDst - (systemID) * addrShift;
			
		/* Get Packet Size */
		pos_start = pos_end + 3;
		pos_end = data.find("v") - 1;
		str = data.substr(pos_start, pos_end - pos_start);
		tracePkt.pkt_size = atoi(str.c_str());
		
		/* Get Packet Value */
		pos_start = data.find("v") + 3;
		pos_end = data.find("\n") - 1;
		str = data.substr(pos_start, pos_end - pos_start);
		tracePkt.value = atol(str.c_str());

		/* Get Packet Miss delay */ 
		getline(infile, data);

		
		if(data.find("thread") < 100) {
			pos_end = data.find(":");
			str = data.substr (0, pos_end);
			stateFlags.thread_time = atol(str.c_str()) + shift;
			
			pos_start = data.find("thread") + 7;
			pos_end = data.find("\n");
			str = data.substr(pos_start, pos_end - pos_start);
			if (!str.compare("create")){
				//warn("thread %d create at %d\n", systemID, stateFlags.thread_time);
				stateFlags.thread_event = 0;
			}
			else if (!str.compare("barrier")){
				//warn("inside thread %d barrier at %d\n", systemID, stateFlags.thread_time);
				stateFlags.thread_event = 1;
			}
			else if(!str.compare("join")){
				//warn("thread %d join at %d\n", systemID, stateFlags.thread_time);
				stateFlags.thread_event = 2;
			}
			
			if (state_flags[0].done) {
				state_flags[stateFlagsCnt].thread_event = stateFlags.thread_event;
				state_flags[stateFlagsCnt].thread_time = stateFlags.thread_time;
			}
			else {
				stateFlags.done = 0;
				state_flags.push_back(stateFlags);
				stateFlagsCnt = stateFlagsCnt + 1; 
				//warn("push_back %d event %d", stateFlagsCnt, stateFlags.thread_event);
			}	
			
			wait_for_lock = true;
			correct_time = true;
			getline(infile, data);
		}
		
		if (data.find("resp") < 100) {
			pos_end = data.find(":");
			str = data.substr (0, pos_end);
			tracePkt.missDelay = atol(str.c_str()) + shift;
			//warn("%d\n", tracePkt.missDelay - tracePkt.cycle);
			DPRINTF(TraceMissDelay, "Miss Resp: %d\n", tracePkt.missDelay - tracePkt.cycle);
			satisfyCpu = true;
		}
		else {
			satisfyCpu = false;
			tracePkt.missDelay = 0;
			warn("tracePkt.missDelay = 0 !!!!");
			warn("Tick: %d", tracePkt.cycle - shift);
		}
		
			
		tracePkt.ready = true; 
		}
	//printTrace();
	return thread_line;
}

void
TraceModel::printTrace()
{
	warn("%s Cycle: %d CPU: !%d! src: %s Type: %s ifetch: %d isRead: %d Addr: %x Size: %d val: %d\n", 
	name(), tracePkt.cycle, tracePkt.traceID, tracePkt.src, tracePkt.type, tracePkt.ifetch, tracePkt.isread, 
	tracePkt.addrDst, tracePkt.pkt_size, tracePkt.value);
}

void
TraceModel::erase(struct TracePkt *ptr)
{
	ptr->cycle = 0;
	ptr->type = "NULL";
	ptr->value = 0;
	ptr->src = "NULL";
	ptr->pkt_size = 0;
	ptr->traceID = 0;
	ptr->ready = false;
	ptr->ifetch = false;
	ptr->isread = false;
	ptr->addrDst = 0x0;
	ptr->missDelay = 0;
}

/* Cache Port Methods */

bool
TraceModel::CpuPort::recvTimingResp(PacketPtr pkt)
{
    trace_cpu->completeRequest(pkt);
    return true;
}

Tick
TraceModel::CpuPort::recvAtomicSnoop(PacketPtr pkt)
{
	trace_cpu->changeState(pkt);
	return 1;
}

void
TraceModel::CpuPort::recvRetry()
{
    trace_cpu->doRetry();
}

void
TraceModel::changeState(PacketPtr pkt){
	/*
	 * If CPU was UNLOCK and he recieves LOCK signal => deschedule next event and wait for new UNLOCK signal
	 * 
	 * else if CPU was LOCK and he recieves UNLOCK signal => calculate new delay and schedule new next event
	 */
	old_state = state;
	
	if (state == 0 && pkt->get<uint32_t>() == 1) {
		//if (tickEvent.scheduled())
		deschedule(tickEvent);
		state_flags[stateFlagsCnt].done = 0;
		warn("CPU #%d LOCK at %d \n", systemID, curTick());
		state = pkt->get<uint32_t>();	// 0 - unlock;	1 - lock;
	}
	else if(state == 1 && pkt->get<uint32_t>() == 0 && stateFlagsCnt == 0){
		/*
		recv_barrier_time = state_flags[0].thread_time - tmp_global_tick;
		barrier_event_time = tmp_global - recv_barrier_time;
				
		barrier_shift = curTick() - state_flags[0].thread_time;
		
		if (barrier_event_time < 0) {
			barrier_event_time = 1000;
			
			Tick tmp = vector_events[0].nextCycle - vector_events[0].tickCycle;
			 
			vector_events[0].tickCycle = curTick() + barrier_event_time;
			vector_events[0].nextCycle = vector_events[0].tickCycle + tmp;
			//warn("not true");
		}
		else {
			shift = shift + barrier_shift;
			vector_events[0].tickCycle = vector_events[0].tickCycle + barrier_shift;
			vector_events[0].nextCycle = vector_events[0].nextCycle + barrier_shift;
			
			//warn("barrier_shift %d shift %d", barrier_shift, shift);
		} 	*/
		
		if (correct_time) {
			state_flags[0].thread_time = tmp_global_tick;
			correct_time = false;
		}
				
		recv_barrier_time = state_flags[0].thread_time - tmp_global_tick;
		barrier_event_time = tmp_global - recv_barrier_time;
				
		barrier_shift = curTick() - state_flags[0].thread_time;
		
		
		shift = shift + barrier_shift;
		vector_events[0].tickCycle = vector_events[0].tickCycle + barrier_shift;
		vector_events[0].nextCycle = vector_events[0].nextCycle + barrier_shift;		
		
		warn("CPU #%d UNLOCK at %d\n", systemID, curTick());
		
		schedule_call(barrier_event_time);
		
		state_flags[0].done = 1;		
		
		state = pkt->get<uint32_t>();	// 0 - unlock;	1 - lock;
			
#if 0
		tmp_shift = curTick() - tmp_global_tick;
		shift = shift + tmp_shift;
		
		vector_events[0].tickCycle = vector_events[0].tickCycle + tmp_shift;
		vector_events[0].nextCycle = vector_events[0].nextCycle + tmp_shift;
		
		warn("CPU #%d UNLOCK at %d\n", systemID, curTick());
		schedule_call(tmp_global);
		state_flags[stateFlagsCnt].done = 1;		
		
		state = pkt->get<uint32_t>();	// 0 - unlock;	1 - lock;
#endif		
	/*	tmp_shift = curTick() - tmp_global_tick;

		//shift = shift + tmp_shift;
	
		vector_events[0].tickCycle = vector_events[0].tickCycle + tmp_shift;
		vector_events[0].nextCycle = vector_events[0].nextCycle + tmp_shift;
		
		warn("shift 0 %d shift 1 %d", shift, shift + tmp_shift);
		
		//tmp_global = vector_events[0].tickCycle - curTick();


		//warn("Schedule %d thread_time %d next %d", curTick(), state_flags[stateFlagsCnt].thread_time, curTick() + tmp_global);
				
		if (barrier_event) {
			tmp_shift = curTick() - state_flags[stateFlagsCnt].thread_time;
			shift = shift + tmp_shift;
		}
		else {
			tmp_shift = curTick() - tmp_global_tick;
			shift = shift + tmp_shift;
		}
		
		vector_events[0].tickCycle = vector_events[0].tickCycle + tmp_shift;
		vector_events[0].nextCycle = vector_events[0].nextCycle + tmp_shift;

		if (barrier_event) {
			tmp_global = vector_events[0].tickCycle - curTick();
			barrier_event = false;
		}
		*/
	}
	else if(state == 1 && pkt->get<uint32_t>() == 0 && stateFlagsCnt){
		//warn("erase %d event %d", stateFlagsCnt, state_flags[0].thread_event);
		state_flags.erase(state_flags.begin());
		stateFlagsCnt = stateFlagsCnt - 1;
		sendAtomicPkt();
		warn("CPU #%d stay LOCK\n", systemID);
		state = 1;
//		barrier_event = true;	
	}
	else if(state == 0 && state == 0){
		state_flags[stateFlagsCnt].done = 1;
	}
	
	warn("Change thread #%d state from %d to %d at %d tick", systemID, old_state, state, curTick());
}

void
TraceModel::sendAtomicPkt()
{
	flag = false;	
	Request *req = new Request();
	Request::Flags flags;
	MemCmd::Command requestType;	
	requestType = MemCmd::WriteReq;
	Addr paddr = 0x00000000; //?
	
	/*	
	Initializes just physical address, size, flags, and timestamp4
	size:	1 - uint8_t, 2 - uint16_t, 4 - uint32_t, 8 - uint64_t
	*/
    req->setPhys(paddr, 4, flags, 0);	

	PacketPtr pkt = new Packet(req, requestType);
	pkt->setSrc(systemID); 	
	pkt->setDest(0);		//not used ?
	pkt->dataDynamicArray(new uint32_t[1]);
	pkt->setData((uint8_t*)&state_flags[0].thread_event);
	pkt->senderState = NULL;
	
	if (!arbiterPort.sendAtomic(pkt))
		warn ("Oups!");
}

void
TraceModel::sendPkt(PacketPtr pkt)
{
		if (!tracePkt.src.compare("i")) {
			if (!icachePort.sendTimingReq(pkt)) {
				//retryPkt = pkt; // RubyPort will retry sending
			}
		}
		else
		{
			if (!dcachePort.sendTimingReq(pkt)) {
				//retryPkt = pkt; // RubyPort will retry sending
			}
		}
}

BaseMasterPort &
TraceModel::getMasterPort(const std::string &if_name, PortID idx)
{
    if (if_name == "icache_port")
        return icachePort;
    else if (if_name == "dcache_port")
        return dcachePort;
    else if (if_name == "arbiter_port")
        return arbiterPort;
    else
        return MemObject::getMasterPort(if_name, idx);
}

TraceModel *
TraceModelParams::create()
{
    return new TraceModel(this);
}
