// EPOS Light Protocol Declarations

#ifndef __elp_h
#define __elp_h

#include <system/config.h>

__BEGIN_SYS

class ELP
{
    template<int unit> friend void call_init();

public:

private:
    static void init(unsigned int unit) {}

private:
};

__END_SYS

#endif
