/*
 * Copyright (c) 2017 Jason Lowe-Power
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
// #ifndef __LEARNING_GEM5_HELLO_OBJECT_HH__
// #define __LEARNING_GEM5_HELLO_OBJECT_HH__

#include "learning_gem5/part2/hello_object.hh"
#include "mem/abstract_mem.hh"
#include "mem/abstractmem_method.hh"
#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/HelloExample.hh"
#include <gsl/gsl_integration.h>
#include <limits>
#include <boost/math/distributions/binomial.hpp>
#include <cmath>
#include <vector>
#include <random>

//std:: ofstream ofs_scrubbing_error("scr_map.txt",std::ios::app);
double bound[5][2] = { {0,0},{2.61,3.39},{3.39,4.39},{4.39,5.39},{5.39,6.39} };
int dis[5][3] = { {0,0,0}, {1,1,2}, {1,2,1}, {1,2,1}, {2,1,1}};
double mean_std_data[5][2] = { {0,0},{0.005,0.02},{0.08,0.032},{0.09,0.036},{0.1,0.04} };
double temp_mean[5][2] = { {0,0}, {3.0,0.13}, {4.0,0.13}, {5.0,0.13}, {6.0,0.13} };
int count_for_val_scrubbing = 0;
int count_for_val_swap = 0;
int rewrite_count = 0;
int total_count = 0;
double rewrite_energy = 0;
double scanning_energy = 0;
double scanning_latency = 0;

double gaussian_pdf(double x, double mean, double stddev) {
    return (1.0 / (stddev * sqrt(2.0 * M_PI)) )* exp(-0.5 * pow((x - mean) / stddev, 2));
}

double integrate_gaussian(double mean, double stddev, double lower_bound, double upper_bound) {
    gsl_function F;
    F.function = [](double x, void *params) { 
        double mean = *((double *)params);
        return gaussian_pdf(x, mean, *((double *)params + 1));
    };
    double params[] = {mean, stddev};
    F.params = &params;

    gsl_integration_workspace *w = gsl_integration_workspace_alloc(1000);

    double result, error;
    gsl_integration_qags(&F, lower_bound, upper_bound, 0, 1e-7, 1000, w, &result, &error);

    gsl_integration_workspace_free(w);

    return result;
}


double error_count ( uint64_t t, uint64_t t0, int level, int error_tolerant ) {
    int n = 512; // 512 bit per line
    double mean = temp_mean[level][0] + mean_std_data[level][0] * ( (log(t)/log(2))-(log(t0)/log(2)) );
    double std = sqrt( pow(temp_mean[level][1],2) + pow(mean_std_data[level][1],2) * pow( (log(t)/log(2))-(log(t0)/log(2)),2 )); 
    std::vector<double> integral_value;
    int j = 1;
    double sum = 0;
    while ( j <= 4 ) {
        if( j != level ) {
            integral_value.push_back(integrate_gaussian(mean, std, bound[j][0], bound[j][1]));
        }
        j++;
    }

    for ( int i = 1; i < 4; i++ ) {
        sum += integral_value[i-1] * dis[level][i-1];
    }
    sum = sum * 0.125;

    boost::math::binomial_distribution<> binom(n,sum);
    double pro = 0;
    double res = 0;
    for( int k = 0; k <= error_tolerant; k++ ) {      
        pro = boost::math::pdf(binom,k);
        res = res + pro;
    }
    return res;
}

double error_page ( uint64_t t, uint64_t t0, uint64_t error_tolerant ) {
    double error_rate = 0;
    for ( int i = 1; i <=4; i++ ) {
        error_rate = error_rate + error_count(t,t0,i,error_tolerant);
    }
    error_rate = error_rate / 4;
    return error_rate;
}

HelloObject::HelloObject(const HelloObjectParams &params) :
    SimObject(params),
    // This is a C++ lambda. When the event is triggered, it will call the
    // processEvent() function. (this must be captured)
    event([this]{ processEvent(); }, name() + ".event"),
    // Note: This is not needed as you can *always* reference this->name()
    myName(params.name),
    latency(params.time_to_wait),
    timesLeft(params.number_of_fires)
{
    DPRINTF(HelloExample, "Created the hello object\n");
    //panic_if(!goodbye, "HelloObject must have a non-null GoodbyeObject");
}

void
HelloObject::startup()
{
    // Before simulation starts, we need to schedule the event
    schedule(event, latency);
}

void
HelloObject::processEvent()
{
    timesLeft--;
    // DPRINTF(HelloExample, "Hello world! Processing the event! %d left\n",timesLeft);
    // std::cout << "global_read_counter is : " << global_read_counter << std::endl;
    // std::cout << "I am simobject " << std::endl;
    uint64_t current_time = curTick() / pow(10,12);
    double accesstime = 0;
    srand(time(0));
    std::cout << "current_time is " << current_time << std::endl;
    std::cout << "RH count is : " << RH << std::endl;
    std::cout << "RC count is : " << RC << std::endl;
    std::cout << "WH count is : " << WH << std::endl;
    std::cout << "light write count is " << light_write_count << std::endl;
    std::cout << "heavy write count is " << heavy_write_count << std::endl;
    std::cout << "rw_flag count is " << rw_flag_count << std::endl;
    
    std::cout << "space_overhead_for_short_ECC count is " << space_overhead_for_short_ECC << std::endl;
    std::cout << "space_overhead_for_medium_ECC count is " << space_overhead_for_medium_ECC << std::endl;
    std::cout << "space_overhead_for_long_ECC count is " << space_overhead_for_long_ECC << std::endl;
    std::cout << "space_overhead_for_heavy_ECC count is " << space_overhead_for_heavy_ECC << std::endl;
    std::cout << "space overhead is " << 10 * space_overhead_for_heavy_ECC + 19 * space_overhead_for_short_ECC + 37 * space_overhead_for_medium_ECC + 73 *space_overhead_for_long_ECC << std::endl;

    if ( current_time % 4 == 0 ) {
        std::cout << " short zone " << current_time << std::endl;
        for ( auto it = scrubbing_state_table[1][true].begin(); it != scrubbing_state_table[1][true].end(); ++it ) {
            // std::cout << "object info 1 " << std::endl;
            for ( auto f : scrubbing_state_table[1][true][it->first] ) {
                // std::cout << "object info 2 " << std::endl;
                //std::cout << "swap space time : " << swap_space[it->first][true][f][0].access_time << std::endl;
                //std::cout << "current time is : " << curTick() << std::endl;
                accesstime = page_table[it->first][f][0].access_time / pow(10,12);
                if ( accesstime <= 1 ) {
                    accesstime = 1;
                }
                // std::cout << "access time is " << accesstime << std::endl;
                //ofs_scrubbing_error << "pro id " << it->first << " page id " << f << " error " << error_page( current_time, accesstime, 1 ) << "\n";
                int temp_swap_space_id = -1;
                temp_swap_space_id = page_in_swap_space ( it->first, f);
                
                for ( int iteration = 0; iteration < 64; iteration++ ) {
                    scanning_energy += 32 * pow(10,-12);
                    scanning_latency += 600 * pow(10,-9);
                    accesstime = page_table[it->first][f][0].line_access_time[iteration] / pow(10,12);
                    if ( accesstime <= 1 ) {
                        accesstime = 1;
                    }
                    // std::cout << "error 1" << std::endl;
                    // std::cout << "error 2 " << temp_swap_space_id << std::endl;
                    scrubbing_desired_times_table[temp_swap_space_id][0]++;
                    if( error_page( current_time, accesstime, 1 ) <= (((double)rand()) / RAND_MAX)  ) {
                        // std::cout << "error 3" << std::endl;
                        scrubbing_desired_times_table[temp_swap_space_id][1]++;
                        rewrite_count++;
                        page_table[it->first][f][0].line_access_time[iteration]= curTick();
                    } else {
                        total_count++;
                    }
                    // std::cout << "error 4" << std::endl;
                }
                // std::cout << "object info 3 " << std::endl;
            }
        }
    }

    if ( current_time % 8 == 0 ) {
        std::cout << " long zone " << current_time << std::endl;
        for ( auto it = scrubbing_state_table[2][true].begin(); it != scrubbing_state_table[2][true].end(); ++it ) {
            //std::cout << "object info 1 " << std::endl;
            for ( auto f : scrubbing_state_table[2][true][it->first] ) {
                //std::cout << "object info 2 " << std::endl;
                //std::cout << "swap space time : " << swap_space[it->first][true][f][0].access_time << std::endl;
                //std::cout << "current time is : " << curTick() << std::endl;
                accesstime = page_table[it->first][f][0].access_time / pow(10,12);
                if ( accesstime <= 1 ) {
                    accesstime = 1;
                }
                //std::cout << "access time is " << accesstime << std::endl;
                //ofs_scrubbing_error << "pro id " << it->first << " page id " << f << " error " << error_page( current_time, accesstime, 1 ) << "\n";
                int temp_swap_space_id = -1;
                temp_swap_space_id = page_in_swap_space ( it->first, f);
                //std::cout << "error 53 " << temp_swap_space_id << std::endl;
                for ( int iteration = 0; iteration < 64; iteration++ ) {
                    //std::cout << "error 54 " << std::endl;
                    scanning_energy += 32 * pow(10,-12);
                    scanning_latency += 600 * pow(10,-9); // 450ns read latency + 150ns scan latency
                    //std::cout << "error 55 " << std::endl;
                    accesstime = page_table[it->first][f][0].line_access_time[iteration] / pow(10,12);
                    //std::cout << "error 56 " << std::endl;
                    if ( accesstime <= 1 ) {
                        accesstime = 1;
                    }
                    //std::cout << "error 1" << std::endl;
                    //std::cout << "error 2 " << temp_swap_space_id << std::endl;
                    scrubbing_desired_times_table[temp_swap_space_id][0]++;
                    if( error_page( current_time, accesstime, 8 ) <= (((double)rand()) / RAND_MAX)  ) {
                        //std::cout << "error 3" << std::endl;
                        scrubbing_desired_times_table[temp_swap_space_id][1]++;
                        rewrite_count++;
                        page_table[it->first][f][0].line_access_time[iteration]= curTick();
                        light_write_latency += 1000 * pow(10,-9);
                    } else {
                        total_count++;
                    }
                    //std::cout << "error 4" << std::endl;
                }
                //std::cout << "object info 3 " << std::endl;
            }
        }
    }
    // aging process
    if( current_time % 10 == 0 ) {
        std::cout << " Aging !!! " << std::endl;
        int temp_process_id = -1;
        int temp_page_id = -1;
        for ( int i = 0; i < page_map.size(); i++ ) {
            temp_process_id = page_map[i][0];
            temp_page_id = page_map[i][1];
            page_table[temp_process_id][temp_page_id][0].read_counter = page_table[temp_process_id][temp_page_id][0].read_counter / 2;
            page_table[temp_process_id][temp_page_id][0].write_counter = page_table[temp_process_id][temp_page_id][0].write_counter / 2;
        }
        global_read_counter = global_read_counter / 2;
        global_write_counter = global_write_counter / 2;
    }

    
    //std::cout << "total count is : " << total_count << std::endl;
    //std::cout << "rewrite count is : " << rewrite_count << std::endl;
    rewrite_energy += 945 * rewrite_count * pow(10,-12);
    std::cout << "rewrite energy consumption is : " << rewrite_energy << std::endl;
    std::cout << "light write energy consumption is : " << light_write_energy << std::endl;
    std::cout << "heavy write energy consumption is : " << heavy_write_energy << std::endl;
    std::cout << "scanning energy consumption is : " << scanning_energy << std::endl; 
    std::cout << "total energy consumption is : " << rewrite_energy + light_write_energy + heavy_write_energy + scanning_energy << std::endl;
    std::cout << "light write latency is : " << light_write_latency << std::endl;
    std::cout << "heavy write latency is : " << heavy_write_latency << std::endl;
    std::cout << "scanning latency is : " << scanning_latency << std::endl;
    std::cout << "total latency is : " << light_write_latency + heavy_write_latency + scanning_latency << std::endl;
    rewrite_count = 0;
    schedule(event, curTick() + latency);
    
}
//#endif // __LEARNING_GEM5_HELLO_OBJECT_HH__




// if ( current_time % 8 == 0 ) {
//     std::cout << " long zone " << current_time << std::endl;
//     for ( auto it = scrubbing_state_table[2][true].begin(); it != scrubbing_state_table[2][true].end(); ++it ) {
//         for ( auto f : scrubbing_state_table[2][true][it->first] ) {
//             //std::cout << "swap space time : " << swap_space[it->first][true][f][0].access_time << std::endl;
//             //std::cout << "current time is : " << curTick() << std::endl;
//             for ( int iteration = 0; iteration < 64; iteration++ ) {
//                 accesstime = swap_space[it->first][true][f][0].access_time[iteration] / pow(10,12);
//                 if ( accesstime <= 1 )
//                     accesstime = 1;
                    
//                 if ( error_page( current_time, accesstime, 1 ) < (((double)rand()) / RAND_MAX) ) {
//                     rewrite_count++;
//                     swap_space[it->first][true][f][0].access_time[iteration] = curTick();
//                 }
//             }
//         }
//     }
// }





