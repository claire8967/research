/*
 * Copyright (c) 2010-2012,2017-2019 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2001-2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mem/abstract_mem.hh"
#include "mem/abstractmem_method.hh"
#include <vector>

#include "arch/locked_mem.hh"
#include "base/loader/memory_image.hh"
#include "base/loader/object_file.hh"
#include "cpu/thread_context.hh"
#include "debug/LLSC.hh"
#include "debug/MemoryAccess.hh"
#include "mem/packet_access.hh"
#include "sim/system.hh"

// my method
#include "base/types.hh"
#include <unordered_map>
#include <string>

int page_in_memory_count = 0;
int page_in_swap_space_count = 0;
int page_limit_count = 25;
int swap_space_limit_count = 524288;
int global_swap_in_count = 0;
int global_swap_out_count = 0;
uint64_t global_access_time = 0;

int global_write_counter = 0;
int global_read_counter = 0;
int process_id;
int page_id;
bool page_type;
bool temp_approximate;
int zone_id;
//std:: ofstream ofsss("memoryaccess.txt",std::ios::app);
//std:: ofstream ofs_page_map("page_map.txt",std::ios::app);
std::unordered_map<int, std::unordered_map<int,std::vector<PageTable>>> page_map; //process_id -> page_id -> PageTable
std::unordered_map<int, std::unordered_map<bool, std::unordered_map<int,std::vector<PageTable>>>> swap_space;
std::unordered_map<int, std::unordered_map<bool, std::unordered_map<int,std::set<int> > > > scrubbing_state_table;

PageTable::PageTable( int process_id, int page_id, bool valid, bool modified, int swap_in_counter, int swap_out_counter, int access_time, bool approximate ) :
    process_id(process_id),page_id(page_id),valid(true),modified(false),read_counter(0),write_counter(0),access_time(global_access_time),approximate(temp_approximate) {
}


AbstractMemory::AbstractMemory(const Params &p) :
    ClockedObject(p), range(p.range), pmemAddr(NULL),
    backdoor(params().range, nullptr,
             (MemBackdoor::Flags)(MemBackdoor::Readable |
                                  MemBackdoor::Writeable)),
    confTableReported(p.conf_table_reported), inAddrMap(p.in_addr_map),
    kvmMap(p.kvm_map), _system(NULL),
    stats(*this)
{
    panic_if(!range.valid() || !range.size(),
             "Memory range %s must be valid with non-zero size.",
             range.to_string());
}

void
AbstractMemory::initState()
{
    ClockedObject::initState();

    const auto &file = params().image_file;
    if (file == "")
        return;

    auto *object = Loader::createObjectFile(file, true);
    fatal_if(!object, "%s: Could not load %s.", name(), file);

    Loader::debugSymbolTable.insert(*object->symtab().globals());
    Loader::MemoryImage image = object->buildImage();

    AddrRange image_range(image.minAddr(), image.maxAddr());
    if (!range.contains(image_range.start())) {
        warn("%s: Moving image from %s to memory address range %s.",
                name(), image_range.to_string(), range.to_string());
        image = image.offset(range.start());
        image_range = AddrRange(image.minAddr(), image.maxAddr());
    }
    panic_if(!image_range.isSubset(range), "%s: memory image %s doesn't fit.",
             name(), file);

    PortProxy proxy([this](PacketPtr pkt) { functionalAccess(pkt); },
                    system()->cacheLineSize());

    panic_if(!image.write(proxy), "%s: Unable to write image.");
}

void
AbstractMemory::setBackingStore(uint8_t* pmem_addr)
{
    // If there was an existing backdoor, let everybody know it's going away.
    if (backdoor.ptr())
        backdoor.invalidate();

    // The back door can't handle interleaved memory.
    backdoor.ptr(range.interleaved() ? nullptr : pmem_addr);

    pmemAddr = pmem_addr;
}

AbstractMemory::MemStats::MemStats(AbstractMemory &_mem)
    : Stats::Group(&_mem), mem(_mem),
    ADD_STAT(swap_in_test, UNIT_COUNT, 
             "Number of bytes read from this memory global swap counter"),
    ADD_STAT(swap_out_test, UNIT_COUNT, 
             "Number of bytes read from this memory global swap counter"),
    ADD_STAT(bytesInstRead, UNIT_BYTE,
             "Number of instructions bytes read from this mmmmemory"),
    ADD_STAT(bytesWritten, UNIT_BYTE,
             "Number of bytes written to this memory"),
    ADD_STAT(numReads, UNIT_COUNT,
             "Number of read requests responded to by this memory"),
    ADD_STAT(numWrites, UNIT_COUNT,
             "Number of write requests responded to by this memory"),
    ADD_STAT(numOther, UNIT_COUNT,
             "Number of other requests responded to by this memory"),
    ADD_STAT(bwRead, UNIT_RATE(Stats::Units::Byte, Stats::Units::Second),
             "Total read bandwidth from this memory"),
    ADD_STAT(bwInstRead, UNIT_RATE(Stats::Units::Byte, Stats::Units::Second),
             "Instruction read bandwidth from this memory"),
    ADD_STAT(bwWrite, UNIT_RATE(Stats::Units::Byte, Stats::Units::Second),
             "Write bandwidth from this memory"),
    ADD_STAT(bwTotal, UNIT_RATE(Stats::Units::Byte, Stats::Units::Second),
             "Total bandwidth to/from this memory")
{
}

void
AbstractMemory::MemStats::regStats()
{
    using namespace Stats;

    Stats::Group::regStats();

    System *sys = mem.system();
    assert(sys);
    const auto max_requestors = sys->maxRequestors();

    bytesRead
        .init(max_requestors)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < max_requestors; i++) {
        bytesRead.subname(i, sys->getRequestorName(i));
    }

    bytesInstRead
        .init(max_requestors)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < max_requestors; i++) {
        bytesInstRead.subname(i, sys->getRequestorName(i));
    }

    bytesWritten
        .init(max_requestors)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < max_requestors; i++) {
        bytesWritten.subname(i, sys->getRequestorName(i));
    }

    numReads
        .init(max_requestors)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < max_requestors; i++) {
        numReads.subname(i, sys->getRequestorName(i));
    }

    numWrites
        .init(max_requestors)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < max_requestors; i++) {
        numWrites.subname(i, sys->getRequestorName(i));
    }

    numOther
        .init(max_requestors)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < max_requestors; i++) {
        numOther.subname(i, sys->getRequestorName(i));
    }

    bwRead
        .precision(0)
        .prereq(bytesRead)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < max_requestors; i++) {
        bwRead.subname(i, sys->getRequestorName(i));
    }

    bwInstRead
        .precision(0)
        .prereq(bytesInstRead)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < max_requestors; i++) {
        bwInstRead.subname(i, sys->getRequestorName(i));
    }

    bwWrite
        .precision(0)
        .prereq(bytesWritten)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < max_requestors; i++) {
        bwWrite.subname(i, sys->getRequestorName(i));
    }

    bwTotal
        .precision(0)
        .prereq(bwTotal)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < max_requestors; i++) {
        bwTotal.subname(i, sys->getRequestorName(i));
    }

    bwRead = bytesRead / simSeconds;
    bwInstRead = bytesInstRead / simSeconds;
    bwWrite = bytesWritten / simSeconds;
    bwTotal = (bytesRead + bytesWritten) / simSeconds;
}

AddrRange
AbstractMemory::getAddrRange() const
{
    return range;
}

// Add load-locked to tracking list.  Should only be called if the
// operation is a load and the LLSC flag is set.
void
AbstractMemory::trackLoadLocked(PacketPtr pkt)
{
    const RequestPtr &req = pkt->req;
    Addr paddr = LockedAddr::mask(req->getPaddr());

    // first we check if we already have a locked addr for this
    // xc.  Since each xc only gets one, we just update the
    // existing record with the new address.
    std::list<LockedAddr>::iterator i;

    for (i = lockedAddrList.begin(); i != lockedAddrList.end(); ++i) {
        if (i->matchesContext(req)) {
            DPRINTF(LLSC, "Modifying lock record: context %d addr %#x\n",
                    req->contextId(), paddr);
            i->addr = paddr;
            return;
        }
    }

    // no record for this xc: need to allocate a new one
    DPRINTF(LLSC, "Adding lock record: context %d addr %#x\n",
            req->contextId(), paddr);
    lockedAddrList.push_front(LockedAddr(req));
    backdoor.invalidate();
}


// Called on *writes* only... both regular stores and
// store-conditional operations.  Check for conventional stores which
// conflict with locked addresses, and for success/failure of store
// conditionals.
bool
AbstractMemory::checkLockedAddrList(PacketPtr pkt)
{
    const RequestPtr &req = pkt->req;
    Addr paddr = LockedAddr::mask(req->getPaddr());
    bool isLLSC = pkt->isLLSC();

    // Initialize return value.  Non-conditional stores always
    // succeed.  Assume conditional stores will fail until proven
    // otherwise.
    bool allowStore = !isLLSC;

    // Iterate over list.  Note that there could be multiple matching records,
    // as more than one context could have done a load locked to this location.
    // Only remove records when we succeed in finding a record for (xc, addr);
    // then, remove all records with this address.  Failed store-conditionals do
    // not blow unrelated reservations.
    std::list<LockedAddr>::iterator i = lockedAddrList.begin();

    if (isLLSC) {
        while (i != lockedAddrList.end()) {
            if (i->addr == paddr && i->matchesContext(req)) {
                // it's a store conditional, and as far as the memory system can
                // tell, the requesting context's lock is still valid.
                DPRINTF(LLSC, "StCond success: context %d addr %#x\n",
                        req->contextId(), paddr);
                allowStore = true;
                break;
            }
            // If we didn't find a match, keep searching!  Someone else may well
            // have a reservation on this line here but we may find ours in just
            // a little while.
            i++;
        }
        req->setExtraData(allowStore ? 1 : 0);
    }
    // LLSCs that succeeded AND non-LLSC stores both fall into here:
    if (allowStore) {
        // We write address paddr.  However, there may be several entries with a
        // reservation on this address (for other contextIds) and they must all
        // be removed.
        i = lockedAddrList.begin();
        while (i != lockedAddrList.end()) {
            if (i->addr == paddr) {
                DPRINTF(LLSC, "Erasing lock record: context %d addr %#x\n",
                        i->contextId, paddr);
                ContextID owner_cid = i->contextId;
                assert(owner_cid != InvalidContextID);
                ContextID requestor_cid = req->hasContextId() ?
                                           req->contextId() :
                                           InvalidContextID;
                if (owner_cid != requestor_cid) {
                    ThreadContext* ctx = system()->threads[owner_cid];
                    TheISA::globalClearExclusive(ctx);
                }
                i = lockedAddrList.erase(i);
            } else {
                i++;
            }
        }
    }

    return allowStore;
}
//my method
int
get_processId ( uint16_t id ) {

    if( id == 5 || id == 6 ) {
        temp_approximate = false;
        return 1;
    } else if( id == 9 || id == 10 ) {
        temp_approximate = true;
        return 2;
    }

    return -1;
}

void swap_in ( int process_id, int page_id ) {
    //ofsss << "swap in process id : " << process_id << " page_id : " << page_id << "\n";
    //ofsss << "global_swap_in_count" << "\n";
    global_swap_in_count++;
    swap_space[process_id][true][page_id][0].valid = false;
    
    if ( swap_space[process_id][true][page_id][0].modified == 1 ) {
        swap_space[process_id][true][page_id][0].write_counter++;
        global_read_counter++;   
    } else {
        swap_space[process_id][true][page_id][0].read_counter++;
        global_write_counter++;
    }
    page_map[process_id][page_id].push_back(swap_space[process_id][true][page_id][0]);
    page_map[process_id][page_id].erase(page_map[process_id][page_id].begin());
    page_map[process_id][page_id][0].access_time = curTick();
    swap_space[process_id][true].erase(page_id);
    swap_space[process_id][false][page_id].push_back(page_map[process_id][page_id][0]);

    for( int i = 1; i <= 3; i++ ) {
        if( scrubbing_state_table[i][true][process_id].find(page_id) != scrubbing_state_table[i][true][process_id].end() ) {
            scrubbing_state_table[i][false][process_id].erase(page_id);
            scrubbing_state_table[i][false][process_id].insert(page_id);   
        }
    }

}

void swap_out ( int process_id, int page_id ) { // 要考慮 swap_space 已滿的情況
    //ofsss << "swap out process id : " << process_id << " page_id : " << page_id << "\n";
    //ofsss << "global_swap_out_count" << "\n";
    global_swap_out_count++;
    page_in_swap_space_count++;
    page_in_memory_count--;

    if( page_in_swap_space_count >= swap_space_limit_count ) {
        std::cout << "select_swap_space_victim_page" << std::endl;
        //select_swap_space_victim_page();
    }
    //ofsss <<"21" << std::endl;
    page_map[process_id][page_id][0].valid = true;
    //ofsss <<"22" << std::endl;
    
    swap_space[process_id][true][page_id].push_back(page_map[process_id][page_id][0]);
    swap_space[process_id][true][page_id].erase(swap_space[process_id][true][page_id].begin());
    //ofsss <<"23" << std::endl;
    int state = set_page_state( process_id, page_id );
    change_page_state( process_id, page_id, state );

    if ( swap_space[process_id][false].find(page_id) != swap_space[process_id][false].end() ) {
        swap_space[process_id][false].erase(page_id);
    } 
    page_map[process_id].erase(page_id);
    
}
// //zone -> valid -> process_id -> set()
// void print_scrubbing_state_table () {
// }

std::pair<int,int> select_victim_page () {
    int value = INT32_MAX;
    int victim_process_id = 0;
    int victim_page_id = 0;
    int temp_value = 0;

    for (const auto& process_pair : page_map ) {
        const auto& temp_map = process_pair.second;
        for (const auto& page_pair : temp_map) {
            temp_value = page_pair.second[0].access_time;
            if( temp_value < value ) {
                value = temp_value;
                victim_process_id = process_pair.first;
                victim_page_id = page_pair.first;
            }
        }
    }
    //ofsss << "select_victim_page process id : " << victim_process_id << " page_id : " << victim_page_id << "\n";
    return {victim_process_id,victim_page_id};
}

int set_page_state ( int process_id, int page_id ) {
    int read_value = swap_space[process_id][true][page_id][0].read_counter;
    int write_value = swap_space[process_id][true][page_id][0].write_counter;
    int return_value = 0;
    //int threshold = global
    if ( write_value > global_write_counter ) { // write hot page
        return_value = 1;
    } else if ( read_value > global_read_counter ) { // read hot page
        return_value = 2;
    } else { 
        return_value = 3; // read cold
    }
    //ofsss << "set_page_state for process_id : " << process_id << " page_id : " << page_id << " state : " << return_value << "\n";
    return return_value;
} 
void change_page_state (int process_id, int page_id, int state ) {
    for( int i = 1; i <= 3; i++ ) {
        if( scrubbing_state_table[i][false][process_id].find(page_id) != scrubbing_state_table[i][false][process_id].end() ) {
            scrubbing_state_table[i][false][process_id].erase(page_id); 
        }
    }
    if( state == 1 && swap_space[process_id][true][page_id][0].approximate == 1 ) { // write hot & approximate data
        scrubbing_state_table[2][true][process_id].insert(page_id);
    } else if ( state == 1 && swap_space[process_id][true][page_id][0].approximate == 0 ) { // write hot & precise data
        scrubbing_state_table[2][true][process_id].insert(page_id);
    } else if ( state == 2 && swap_space[process_id][true][page_id][0].approximate == 1 ) { // read hot &  approximate data
        scrubbing_state_table[1][true][process_id].insert(page_id);
    } else if ( state == 2 && swap_space[process_id][true][page_id][0].approximate == 0 ) { // read hot &  precise data
        scrubbing_state_table[3][true][process_id].insert(page_id);
    } else if ( state == 3 && swap_space[process_id][true][page_id][0].approximate == 1 ) { // read cold & approximate data
        scrubbing_state_table[2][true][process_id].insert(page_id);
    } else if ( state == 3 && swap_space[process_id][true][page_id][0].approximate == 0 ) { // read cold & precise data
        scrubbing_state_table[2][true][process_id].insert(page_id);
    } else {
        std::cout << "change page state error " << std::endl;
    }
}

#if TRACING_ON
static inline void
tracePacket(System *sys, const char *label, PacketPtr pkt)
{
    //std::cout<< "sys tick is : " << curTick() << std::endl;
    int size = pkt->getSize();
    if (size == 1 || size == 2 || size == 4 || size == 8) {
        ByteOrder byte_order = sys->getGuestByteOrder();
        // DPRINTF(MemoryAccess, "%s from %s of size %i on address %#x data "
        //         "%#x %c\n", label, sys->getRequestorName(pkt->req->
        //         requestorId()), size, pkt->getAddr(),
        //         size, pkt->getAddr(), pkt->getUintX(byte_order),
        //         pkt->req->isUncacheable() ? 'U' : 'C');
        
        DPRINTF(MemoryAccess,"data is %#x \n",pkt->getUintX(byte_order));
        
        //ini
        page_id = pkt->getAddr() / 4096;
        process_id = get_processId( pkt->requestorId() );
        bool page_is_write = !pkt->isRead(); // 1:write, 0:read
        global_access_time++;
        //ofsss << "access in process id : " << process_id << " page_id : " << page_id << std::endl;
        

        //PageTable temp_page(process_id,page_id,true,page_is_write,0,0,global_access_time,temp_approximate);
    
        
        //allocate
        
        if ( page_map.find(process_id) != page_map.end() ) { // process in page_map 
            if ( page_map[process_id].find(page_id) != page_map[process_id].end() ) { // 在 Mem valid (case1)
                page_map[process_id][page_id][0].access_time = curTick();
                page_map[process_id][page_id][0].modified = page_is_write;
            
            } else { // memory full -> do swap_out()

                if( page_in_memory_count >= page_limit_count ) { 
                    int victim_process_id, victim_page_id;
                    std::tie( victim_process_id, victim_page_id ) = select_victim_page();
                    //ofsss << "swap out " << std::endl;
                    swap_out( victim_process_id, victim_page_id );
                    page_in_memory_count--;
                }

                if ( swap_space[process_id][true].find(page_id) != swap_space[process_id][true].end() ) { // page in swap_space valid -> do swap_in ()
                    //ofsss << "swap in " << std::endl;
                    swap_in( process_id, page_id );
                    page_in_memory_count++;                    

                } else {
                    PageTable temp_page(process_id,page_id,true,page_is_write,0,0,curTick(),temp_approximate);
                    page_map[process_id][page_id].push_back(temp_page);
                    page_in_memory_count++;
                }
            
            } 

        } else { // process not in page_map

            if ( page_in_memory_count >= page_limit_count ) {
                int victim_process_id, victim_page_id;
                std::tie( victim_process_id, victim_page_id ) = select_victim_page();
                swap_out( victim_process_id, victim_page_id );
            }

            if ( swap_space[process_id][true].find(page_id) != swap_space[process_id][true].end() ) {
                swap_in( process_id, page_id );   
                //ofsss << "swap in " << std::endl;
                page_in_memory_count++;
            } else {
                PageTable temp_page(process_id,page_id,true,page_is_write,0,0,curTick(),temp_approximate);
                page_map[process_id][page_id].push_back(temp_page);
                page_in_memory_count++;
            }

        }
        return;
    }


    DPRINTF(MemoryAccess, "%s from %s of size %i on address %#x %c\n",
            label, sys->getRequestorName(pkt->req->requestorId()),
            size, pkt->getAddr(), pkt->req->isUncacheable() ? 'U' : 'C');
    DDUMP(MemoryAccess, pkt->getConstPtr<uint8_t>(), pkt->getSize());

}

#   define TRACE_PACKET(A) tracePacket(system(), A, pkt)
#else
#   define TRACE_PACKET(A)
#endif

void
AbstractMemory::access(PacketPtr pkt)
{
    global_swap_in_count = 0;
    global_swap_out_count = 0;
    //std::cout << "access " << std::endl;
    //ByteOrder byte_order = "LITTLE_ENDIAN";
    //System *sys;
    //ByteOrder byte_order = sys->getGuestByteOrder();

    //ofsss << "reqid " << pkt->req->requestorId() << "\n";
    //ofsss << "addr " << pkt->getAddr() << "\n";
    //ofsss << "data " << pkt->getUintX(byte_order) << "\n";

    if (pkt->cacheResponding()) {
        DPRINTF(MemoryAccess, "Cache responding to %#llx: not responding\n",
                pkt->getAddr());
        return;
    }

    if (pkt->cmd == MemCmd::CleanEvict || pkt->cmd == MemCmd::WritebackClean) {
        DPRINTF(MemoryAccess, "CleanEvict  on 0x%x: not responding\n",
                pkt->getAddr());
      return;
    }

    assert(pkt->getAddrRange().isSubset(range));

    uint8_t *host_addr = toHostAddr(pkt->getAddr());

    if (pkt->cmd == MemCmd::SwapReq) {
        if (pkt->isAtomicOp()) {
            if (pmemAddr) {
                pkt->setData(host_addr);
                (*(pkt->getAtomicOp()))(host_addr);
            }
        } else {
            std::vector<uint8_t> overwrite_val(pkt->getSize());
            uint64_t condition_val64;
            uint32_t condition_val32;

            panic_if(!pmemAddr, "Swap only works if there is real memory " \
                     "(i.e. null=False)");

            bool overwrite_mem = true;
            // keep a copy of our possible write value, and copy what is at the
            // memory address into the packet
            pkt->writeData(&overwrite_val[0]);
            pkt->setData(host_addr);

            if (pkt->req->isCondSwap()) {
                if (pkt->getSize() == sizeof(uint64_t)) {
                    condition_val64 = pkt->req->getExtraData();
                    overwrite_mem = !std::memcmp(&condition_val64, host_addr,
                                                 sizeof(uint64_t));
                } else if (pkt->getSize() == sizeof(uint32_t)) {
                    condition_val32 = (uint32_t)pkt->req->getExtraData();
                    overwrite_mem = !std::memcmp(&condition_val32, host_addr,
                                                 sizeof(uint32_t));
                } else
                    panic("Invalid size for conditional read/write\n");
            }

            if (overwrite_mem)
                std::memcpy(host_addr, &overwrite_val[0], pkt->getSize());

            assert(!pkt->req->isInstFetch());
            TRACE_PACKET("Read/Write");
            stats.swap_in_test += global_swap_in_count;
            stats.swap_out_test += global_swap_out_count;
            stats.numOther[pkt->req->requestorId()]++;
        }
    } else if (pkt->isRead()) {
        assert(!pkt->isWrite());
        if (pkt->isLLSC()) {
            assert(!pkt->fromCache());
            // if the packet is not coming from a cache then we have
            // to do the LL/SC tracking here
            trackLoadLocked(pkt);
        }
        if (pmemAddr) {
            pkt->setData(host_addr);
        }
        TRACE_PACKET(pkt->req->isInstFetch() ? "IFetch" : "Read");
        stats.swap_in_test += global_swap_in_count;
        stats.swap_out_test += global_swap_out_count;
        stats.numReads[pkt->req->requestorId()]++;
        stats.bytesRead[pkt->req->requestorId()] += pkt->getSize();
        if (pkt->req->isInstFetch())
            stats.bytesInstRead[pkt->req->requestorId()] += pkt->getSize();
    } else if (pkt->isInvalidate() || pkt->isClean()) {
        assert(!pkt->isWrite());
        // in a fastmem system invalidating and/or cleaning packets
        // can be seen due to cache maintenance requests

        // no need to do anything
    } else if (pkt->isWrite()) {
        if (writeOK(pkt)) {
            if (pmemAddr) {
                pkt->writeData(host_addr);
                DPRINTF(MemoryAccess, "%s write due to %s\n",
                        __func__, pkt->print());
            }
            assert(!pkt->req->isInstFetch());
            TRACE_PACKET("Write");
            stats.swap_in_test += global_swap_in_count;
            stats.swap_out_test += global_swap_out_count;
            stats.numWrites[pkt->req->requestorId()]++;
            stats.bytesWritten[pkt->req->requestorId()] += pkt->getSize();
        }
    } else {
        panic("Unexpected packet %s", pkt->print());
    }
    if (pkt->needsResponse()) {
        pkt->makeResponse();
    }
    
}

void
AbstractMemory::functionalAccess(PacketPtr pkt)
{
    assert(pkt->getAddrRange().isSubset(range));

    uint8_t *host_addr = toHostAddr(pkt->getAddr());

    if (pkt->isRead()) {
        if (pmemAddr) {
            pkt->setData(host_addr);
        }
        TRACE_PACKET("Read");
        pkt->makeResponse();
    } else if (pkt->isWrite()) {
        if (pmemAddr) {
            pkt->writeData(host_addr);
        }
        TRACE_PACKET("Write");
        pkt->makeResponse();
    } else if (pkt->isPrint()) {
        Packet::PrintReqState *prs =
            dynamic_cast<Packet::PrintReqState*>(pkt->senderState);
        assert(prs);
        // Need to call printLabels() explicitly since we're not going
        // through printObj().
        prs->printLabels();
        // Right now we just print the single byte at the specified address.
        ccprintf(prs->os, "%s%#x\n", prs->curPrefix(), *host_addr);
    } else {
        panic("AbstractMemory: unimplemented functional command %s",
              pkt->cmdString());
    }
}
