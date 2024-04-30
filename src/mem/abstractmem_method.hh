
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
extern double global_write_counter;
extern double global_read_counter;

extern double read_energy;
extern double light_write_energy;
extern double heavy_write_energy;

extern double light_write_latency;
extern double heavy_write_latency;
extern double read_latency;

extern int RC;
extern int RH;
extern int WH;
extern int light_write_count;
extern int heavy_write_count;
extern int rw_flag_count;

extern int space_overhead;
extern int space_overhead_for_short_ECC;
extern int space_overhead_for_long_ECC;
extern int space_overhead_for_medium_ECC;
extern int space_overhead_for_heavy_ECC; 

// for code
class PageTable {
public:
    int process_id;
    int page_id;
    bool valid;
    bool modified;
    double read_counter;
    double write_counter;

    // for error
    uint64_t access_time;
    std::vector<uint64_t> line_access_time;
    int swap_space_id;

    bool approximate;
    PageTable( int process_id, int page_id, bool valid, bool modified, int read_counter, int write_counter, uint64_t access_time, bool approximate);
    ~PageTable(){};
};

extern std::vector<std::vector<int>> page_map;
// memory_id -> {process_id,page_id}

extern std::vector<std::vector<int>> swap_space;
// swapspace_id -> {valid,process_id,page_id}

extern std::unordered_map <int, std::unordered_map<int, std::vector<PageTable> >> page_table;
// process_id -> page_id -> pagetable

extern std::unordered_map<int, std::unordered_map<bool, std::unordered_map<int,std::set<int> > > > scrubbing_state_table;
// zone_id -> valid/invalid -> process_id -> page_id

extern std::unordered_map <int, std::vector<int> > scrubbing_desired_times_table;
// swap_space_id -> {error times,scrubbing times}


int get_processId();
int page_in_memory ( int process_id, int page_id );

extern int page_in_swap_space ( int process_id, int page_id );
int page_in_swap_space_valid ( int process_id, int page_id );
int page_in_swap_space_invalid ( int process_id, int page_id );

void swap_in ( int process_id, int page_id, bool page_is_write, int swap_space_id );
int swap_out ( int victim_process_id, int victim_page_id );
int find_free_space_in_swap_space ();
int find_free_space_in_memory ();
void add_page_to_memory( int process_id, int page_id, int page_is_write, int memory_id_for_swap_out ); 

void scrubbing_desired_times_table_ini();
void baseline_zone( int victim_process_id, int victim_page_id );

int select_swap_space_victim_page ();
int set_page_state( int process_id, int page_id );; // 1:ReadHot 2:ReadCold 3:WriteHot
void change_page_state( int process_id, int page_id, int state ); // 1:short zone 2:medium zone 3: long zone
std::pair<int,int> select_victim_page ();


#endif //__ABSTRACTMEM_METHOD_HH__