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

double bound[5][2] = { {0,0},{2.61,3.39},{3.39,4.39},{4.39,5.39},{5.39,6.39} };
int dis[5][3] = { {0,0,0}, {1,1,2}, {1,2,1}, {1,2,1}, {2,1,1}};
double mean_std_data[5][2] = { {0,0},{0.005,0.02},{0.08,0.032},{0.09,0.036},{0.1,0.04} };
double temp_mean[5][2] = { {0,0}, {3.0,0.13}, {4.0,0.13}, {5.0,0.13}, {6.0,0.13} };
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


double error_count ( int t, int t0, int level, int error_tolerant ) {
    int n = 256;
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

double error_page ( int t, int t0, int error_tolerant ) {
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
    DPRINTF(HelloExample, "Hello world! Processing the event! %d left\n",timesLeft);
    std::cout << "global_read_counter is : " << global_read_counter << std::endl;
    std::cout << "I am simobject " << std::endl;
    std::cout << "error experiment is : " << error_page( 11020,9996,1 ) << std::endl;

    schedule(event, curTick() + latency);
    
}
//#endif // __LEARNING_GEM5_HELLO_OBJECT_HH__