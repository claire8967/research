#ifndef __scrubbing_table__
#define __scrubbing_table__

#include "params/scrubbing_table.hh"
#include "sim/sim_object.hh"

namespace gem5
{
    class ScrubbingObject : public SimObject
    {
        public:
            HelloObject(const HelloObjectParams &p);
    };

} // namespace gem5


#endif // __scrubbing_table__