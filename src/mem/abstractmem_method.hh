
#ifndef __ABSTRACTMEM_METHOD_HH__
#define __ABSTRACTMEM_METHOD_HH__

#include "base/types.hh"
#include <unordered_map>
#include <string>
#include <queue>
#include <set>
#include <limits.h>
#include "sim/stats.hh"
#include "base/time.hh"
class PageTable;


extern int page_in_memory_count;
extern int page_in_swap_space_count;
extern int page_limit_count;
extern int swap_space_limit_count;
extern int global_swap_in_count;
extern int global_swap_out_count;
extern uint64_t global_access_time;
extern int global_write_counter;
extern int global_read_counter;


// for code
class PageTable {
public:
    int process_id;
    int page_id;
    bool valid;
    bool modified;
    int read_counter;
    int write_counter;
    int access_time;
    bool approximate;
    PageTable( int process_id, int page_id, bool valid, bool modified, int read_counter, int write_counter, int access_time, bool approximate);
    ~PageTable(){};
};
extern std::unordered_map<int, std::unordered_map<int,std::vector<PageTable>>> page_map;
extern std::unordered_map<int, std::unordered_map<bool, std::unordered_map<int,std::vector<PageTable>>>> swap_space;
extern std::unordered_map<int, std::unordered_map<bool, std::unordered_map<int,std::set<int> > > > scrubbing_state_table;
// zone_id -> valid/invalid -> process_id -> page_id
int get_processId();
void swap_in ( int process_id, int page_id );
void swap_out ( int process_id, int page_id );
int set_page_state( int process_id, int page_id );; // 1:ReadHot 2:ReadCold 3:WriteHot
void change_page_state( int process_id, int page_id, int state ); // 1:short zone 2:medium zone 3: long zone
std::pair<int,int> select_victim_page ();


#endif //__ABSTRACTMEM_METHOD_HH__