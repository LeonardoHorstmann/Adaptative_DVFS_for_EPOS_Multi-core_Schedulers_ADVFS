// EPOS IA32 Time-Stamp Counter Mediator Declarations

#ifndef __ia32_thermal_h
#define __ia32_thermal_h

#include <cpu.h>

__BEGIN_SYS

class Thermal
{


public:
    Thermal() {}

    static unsigned int temperature() {
	CPU::Reg32 IA32_THERM_STATUS = 0x19c;
	CPU::Reg32 IA32_TEMPERATURE_TARGET = 0x1a2;	        
	CPU::Reg64 therm_read = CPU::rdmsr(IA32_THERM_STATUS);
        CPU::Reg64 temp_target_read = CPU::rdmsr(IA32_TEMPERATURE_TARGET);
	int bits = 22 - 16 + 1;
	therm_read >>= 16;
	therm_read &= (1ULL << bits) - 1;
	bits = 23 - 16 + 1;
	temp_target_read >>= 16;
	temp_target_read &= (1ULL << bits) - 1;
	unsigned int temp = (unsigned int)(temp_target_read - therm_read);
	return temp;
    }
};

__END_SYS

#endif
