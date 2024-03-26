#include "learning_gem5/hello_object.hh"
#include <iostream>

namespace gem5
{
    ScrubbingObject::ScrubbingObject(const ScrubbingObjectParams &params ) : SimObject(params)
    {
        std::cout << " scrubbing ! " << std::endl;
    }
} // namespace gem5