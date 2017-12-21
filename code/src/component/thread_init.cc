// EPOS Thread Component Initialization

#include <system.h>
#include <thread.h>
#include <alarm.h>

__BEGIN_SYS

void Thread::init()
{
    // The installation of the scheduler timer handler must precede the
    // creation of threads, since the constructor can induce a reschedule
    // and this in turn can call timer->reset()
    // Letting reschedule() happen during thread creation is harmless, since
    // MAIN is created first and dispatch won't replace it nor by itself
    // neither by IDLE (which has a lower priority)
    if(Criterion::timed && (Machine::cpu_id() == 0))
        _timer = new (SYSTEM) Scheduler_Timer(QUANTUM, time_slicer);

    // Install an interrupt handler to receive forced reschedules
    if(smp) {
        if(Machine::cpu_id() == 0)
            IC::int_vector(IC::INT_RESCHEDULER, rescheduler);
        IC::enable(IC::INT_RESCHEDULER);
    }

    //_cpu_temperature[Machine::cpu_id()] = Thermal::temperature();

    //Inicialization of PMU Channels to the statistics collecting in energy aware policies
    // Capturing INSTRUCTION, LLC_MISS, DVS_CLOCK.
    if(Criterion::energy_aware) {
    	_cpu_temperature[Machine::cpu_id()] = Thermal::temperature();
        
        APIC::disable_perf_int();

        PMU::stop(3);
        PMU::stop(4);
        PMU::stop(5);
        
        PMU::reset(3);
        PMU::reset(4);
        PMU::reset(5);

        PMU::write(3, 0);
        PMU::write(4, 0);
        PMU::write(5, 0);

        PMU::config(3, PMU::INSTRUCTION);
        PMU::config(4, PMU::LLC_MISS);
        PMU::config(5, PMU::DVS_CLOCK);
    
        PMU::start(3);
        PMU::start(4);
        PMU::start(5);
            
        APIC::enable_perf_int();
    }

}

__END_SYS
