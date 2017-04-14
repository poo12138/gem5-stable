/*
 *-------------------------------------------------------------------------------------------------
 *  This file is a part of the GEM5 LIBRARY from LIRMM
 *  Copyright (C) 2012 BUTKO Anastasiia <nastya.butko@yahoo.com>
 *-------------------------------------------------------------------------------------------------
 *  File: TraceCPU.hh
 *  Author: Butko Anastasiia
 *-------------------------------------------------------------------------------------------------
 */

#ifndef __CPU_TRACEMODEL_TRACEMODEL_HH__
#define __CPU_TRACEMODEL_TRACEMODEL_HH__

#include <set>
#include <string>

#include "base/statistics.hh"
#include "mem/mem_object.hh"
#include "mem/port.hh"
#include "params/TraceModel.hh"
#include "sim/eventq.hh"
#include "sim/sim_exit.hh"
#include "sim/sim_object.hh"
#include "sim/stats.hh"

#include <vector>

using namespace std;

class Packet;
class TraceModel : public MemObject
{
  public:
    typedef TraceModelParams Params;
    TraceModel(const Params *p);

	// State Flags: Each CPU has his state. Must be improve! 
	//int State;
	
	int systemID;
	int cpu_number;
	
	System *system;
	
	struct StateFlags {
		uint32_t thread_event;				
		bool done;
		Tick thread_time;						
	};
		
	std::vector<StateFlags> state_flags;
	
	StateFlags stateFlags;
	
	int stateFlagsCnt;
	
	uint32_t state;
	uint32_t old_state;
	
	struct Events {
		bool done;
		Tick tickCycle;
		int type;
		Tick nextCycle;
		bool operator < (const Events &Other)const{
			return tickCycle < Other.tickCycle;
		}
	};
	
	std::vector<Events> vector_events;
	
	Events events;

	struct RcvPkt {
		bool in_advance;
		bool in_time;
		bool delay;
	};
	
	RcvPkt rcvPkt;

    // Trace Reader Variables	
	
	struct TracePkt	{
		unsigned long int cycle;
		string type;
		uint64_t value;
		int system;
		string src;
		Addr addrDst;
		unsigned int pkt_size;
		int traceID;
		bool ready;
		
		bool isuncacheable;
		bool ifetch;
		bool isclearll;
		bool isread;
		bool iswrite;
		bool meminhibitasserted;
		unsigned long int missDelay;
	};
	
	TracePkt tracePkt;
	TracePkt currentTrace;
	TracePkt nextTrace;
	
    ifstream infile;
    string data;
    bool eof;
	int progress;
	
    uint32_t tmpValue;
    
    //Tick virtual_cycle;
    unsigned int virtual_cycle;
    virtual void init();

    Tick ticks(Tick numCycles) const { return numCycles; }

    // main simulation loop (one cycle)
    void tick();

    virtual BaseMasterPort &getMasterPort(const std::string &if_name,
                                          PortID idx = InvalidPortID);

    void openInputFile(const std::string &filename);

    bool getValue();
    
    void Read();
    
  protected:
    class TickEvent : public Event
    {
      private:
        TraceModel *cpu;

      public:
        TickEvent(TraceModel *c) : Event(CPU_Tick_Pri), cpu(c) {}
        void process() { cpu->tick(); }
        virtual const char *description() const { return "TraceModel tick"; }
    };

    TickEvent tickEvent;

    class CpuPort : public MasterPort
    {
        TraceModel *trace_cpu;

      public:

        CpuPort(const std::string &_name, TraceModel *_trace_cpu)
            : MasterPort(_name, _trace_cpu), trace_cpu(_trace_cpu)
        { }

      protected:

        virtual bool recvTimingResp(PacketPtr pkt);
        
        virtual void recvTimingSnoopReq(PacketPtr pkt) { }
        
        virtual void recvFunctionalSnoop(PacketPtr pkt) { }
        
        virtual Tick recvAtomicSnoop(PacketPtr pkt);
        
        virtual void recvRetry();
        
    };

    CpuPort icachePort;

	CpuPort dcachePort;
	
	CpuPort arbiterPort;
	
    class TraceModelSenderState : public Packet::SenderState
    {
      public:
        /** Constructor. */
        TraceModelSenderState(uint8_t *_data)
            : data(_data)
        { }

        // Hold onto data pointer
        uint8_t *data;
    };

    PacketPtr retryPkt;
    unsigned size;
    int id;
	int latency;
    unsigned blockSizeBits;
	bool satisfyCpu;
	unsigned long losed;
	unsigned long recived;
	unsigned long sent;
	int diff;
	bool wait_for_lock;
	bool barrier_event;
	Tick tmp_shift;
    Tick noResponseCycles;
	Tick tmp;
    Tick simCycles;
    Tick startTick;
    Tick shift;
    
    bool flag;
    bool thread_line;
    bool sent_flag;
    bool correct_time;
    
    // Dependency time variables
    Tick tmp_global;
    Tick tmp_global_tick;
    std::string traceFile;
    MasterID masterId;
    int iEvent;
	int traceId;
	
	Tick wait_start;
	Tick wait_stop;
	
	Tick recv_barrier_time;
	Tick barrier_event_time;
	Tick barrier_shift;
	
	Tick trace_response;
	
    void completeRequest(PacketPtr pkt);
    

    void generateWritePkt();
    void generateReadPkt();
    void sendPkt(PacketPtr pkt);
	void printTrace();
    void doRetry();
	void erase(struct TracePkt *ptr);
	
	/* Dependency functions*/
	void sendAtomicPkt();
	void changeState(PacketPtr pkt);
	
	/* Cycle shift for replication variables*/
	Tick thread_create_start;
	bool create_shift_flag;
	Tick thread_create_shift;
	
	/* Address shift for replication variables*/
	Addr addrShift;
	
	
	void setEvent();
	void processEvent();	
	void insertEvent(Tick tickTime, int eventType, Tick nextTime);
	
	void schedule_call(Tick tmp_local);
	
	// generatePkt Event
	void generatePkt();	
	friend class EventWrapper<TraceModel, &TraceModel::generatePkt>;
	EventWrapper<TraceModel, &TraceModel::generatePkt> generatePktEvent;
	
	void deleteEvent();
	friend class EventWrapper<TraceModel, &TraceModel::deleteEvent>;
	EventWrapper<TraceModel, &TraceModel::deleteEvent> deleteEventEvent;
	
    friend class MemCompleteEvent;
};

#endif // __CPU_TRACECPU_TRACECPU_HH__



