#include "pico/stdlib.h"
#include "../../IrqFifo.h"

namespace ir {
    namespace nec {

        class DataFrame {
            public:
                const uint8_t address;
                const uint8_t command;
                const uint64_t startTime; 
                const bool valid;
                DataFrame(uint32_t data, uint64_t startTime);
        };

        // One NEC remote at a time
        void initOnGpio(uint gpio);
        IrqFifo<DataFrame>& outFifo();
    }
}

