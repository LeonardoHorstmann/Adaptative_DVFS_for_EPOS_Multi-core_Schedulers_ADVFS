/*=======================================================================*/
/* eposcfg.cc                                                            */
/*                                                                       */
/* Desc: Tool to query EPOS configuration.                               */
/*                                                                       */
/* Parm: <configuration name>                                            */
/*                                                                       */
/* Auth: Davi Resner                                                     */
/*=======================================================================*/

// Traits are included in config.h
#include <system/config.h>

// Using only bare C to avoid conflicts with EPOS
#include <stdio.h>
#include <string.h>

using namespace EPOS;

// Constants
const unsigned int ARRAY_MAX = 128;
const unsigned int STR_SIZE_MAX = 128;

// Configurations that return an int
char int_cfg_names[STR_SIZE_MAX][ARRAY_MAX] = {
    "-CPUS",
    "-NODES",
};
// Their values
int int_cfg_values[ARRAY_MAX] = {
    Traits<Build>::CPUS,
    Traits<Build>::NODES
};

// Configurations that return (a) string(s)
char str_cfg_names[STR_SIZE_MAX][ARRAY_MAX] = {
    "-ARCHITECTURE",
    "-MACHINE",
    "-MODEL",
    "-COMPONENTS",
    "-MEDIATORS",
};
// Values for single-string configurations
// populated at populate_strings()
char str_cfg_values[STR_SIZE_MAX][ARRAY_MAX]; 

// List of EPOS components. Changes here must be replicated at populate_strings()
char components[STR_SIZE_MAX][ARRAY_MAX] = {
    "System",
    "Application",
    "Thread",
    "Active",
    "Periodic_Thread",
    "RT_Thread",
    "Task",
    "Scheduler",
    "Address_Space",
    "Segment",
    "Synchronizer",
    "Mutex",
    "Semaphore",
    "Condition",
    "Clock",
    "Chronometer",
    "Alarm",
    "Delay",
    "Network",
    "ELP",
    "TSTP",
    "ARP",
    "IP",
    "ICMP",
    "UDP",
    "TCP",
    "DHCP",
    "IPC",
    "Link",
    "Port",
    "Smart_Data",
};
// populated at populate_strings()
bool enabled_components[ARRAY_MAX];

// List of EPOS mediators. Changes here must be replicated at populate_strings()
char mediators[STR_SIZE_MAX][ARRAY_MAX] = {
    "CPU",
    "TSC",
    "MMU",
    "FPU",
    "PMU",
    "Machine",
    "PCI",
    "IC",
    "Timer",
    "RTC",
    "UART",
    "USB",
    "EEPROM",
    "Display",
    "Serial_Display",
    "Keyboard",
    "Serial_Keyboard",
    "Scratchpad",
    "GPIO",
    "I2C",
    "ADC",
    "FPGA",
    "NIC",
    "Ethernet",
    "IEEE802_15_4",
    "PCNet32",
    "C905",
    "E100",
    "CC2538",
    "AT86RF",
    "GEM",
};
// populated at populate_strings()
bool enabled_mediators[ARRAY_MAX];


// Prototypes

// Finds the index of given configuration name in names_array
// returns -1 if not found
int find_cfg(char names_array[STR_SIZE_MAX][ARRAY_MAX], const char * cfg_name);

// Populates the values for the string configurations
void populate_strings();



// Main
int main(int argc, char **argv)
{
    if(argc != 2) {
        fprintf(stderr, "Usage: %s <configuration>\n", argv[0]);
        fprintf(stderr, "Possible values for <configuration> are:\n");
        for(unsigned int i = 0; i < ARRAY_MAX && strlen(int_cfg_names[i]); i++)
            puts(int_cfg_names[i]);
        for(unsigned int i = 0; i < ARRAY_MAX && strlen(str_cfg_names[i]); i++)
            puts(str_cfg_names[i]);

        return 1;
    }

    if(strlen(argv[1]) > STR_SIZE_MAX) {
        fprintf(stderr, "ERROR! Configuration name too long\n");
        return 1;
    }

    populate_strings();

    if(!strcmp(argv[1], "-COMPONENTS")) {
        for(unsigned int i = 0; i < ARRAY_MAX; i++)
            if(enabled_components[i])
                puts(components[i]);
        return 0;
    }

    if(!strcmp(argv[1], "-MEDIATORS")) {
        for(unsigned int i = 0; i < ARRAY_MAX; i++)
            if(enabled_mediators[i])
                puts(mediators[i]);
        return 0;
    }

    int idx;
    if((idx = find_cfg(int_cfg_names, argv[1])) >= 0)
        printf("%d\n", int_cfg_values[idx]);
    else if((idx = find_cfg(str_cfg_names, argv[1])) >= 0)
        printf("%s\n", str_cfg_values[idx]);
    else {
        fprintf(stderr, "ERROR! Configuration \"%s\" not supported!\n", argv[1]);
        return 1;
    }

    return 0;
}

// Finds the index of given configuration name in names_array
// returns -1 if not found
int find_cfg(char names_array[STR_SIZE_MAX][ARRAY_MAX], const char * cfg_name)
{
    for(unsigned int i = 0; i < ARRAY_MAX; i++)
        if(!strcmp(names_array[i], cfg_name))
            return i;
    return -1;
}

// Populates the values for the string configurations
void populate_strings()
{
    int idx;
   
    idx = find_cfg(str_cfg_names, "-ARCHITECTURE");
    if(idx >= 0) {
        switch(Traits<Build>::ARCHITECTURE) {
            case Traits<Build>::IA32:
                strcpy(str_cfg_values[idx], "IA32");
                break;
            case Traits<Build>::ARMv7:
                strcpy(str_cfg_values[idx], "ARMv7");
                break;
            default:
                strcpy(str_cfg_values[idx], "ERROR! Value not supported!");
                break;
        }
    }

    idx = find_cfg(str_cfg_names, "-MACHINE");
    if(idx >= 0) {
        switch(Traits<Build>::MACHINE) {
            case Traits<Build>::PC:
                strcpy(str_cfg_values[idx], "PC");
                break;
            case Traits<Build>::Cortex:
                strcpy(str_cfg_values[idx], "Cortex");
                break;
            default:
                strcpy(str_cfg_values[idx], "ERROR! Value not supported!");
                break;
        }
    }

    idx = find_cfg(str_cfg_names, "-MODEL");
    if(idx >= 0) {
        switch(Traits<Build>::MODEL) {
            case Traits<Build>::Legacy_PC:
                strcpy(str_cfg_values[idx], "Legacy_PC");
                break;
            case Traits<Build>::eMote3:
                strcpy(str_cfg_values[idx], "eMote3");
                break;
            case Traits<Build>::LM3S811:
                strcpy(str_cfg_values[idx], "LM3S811");
                break;
            case Traits<Build>::Zynq:
                strcpy(str_cfg_values[idx], "Zynq");
                break;
            default:
                strcpy(str_cfg_values[idx], "ERROR! Value not supported!");
                break;
        }
    }

    idx = find_cfg(str_cfg_names, "-COMPONENTS");
    if(idx >= 0) {
        for(unsigned int i = 0; i < ARRAY_MAX; i++) {
            if(!strcmp(components[i], "System"))
                enabled_components[i] = Traits<System>::enabled;
            else if(!strcmp(components[i], "Application"))
                enabled_components[i] = Traits<Application>::enabled;
            else if(!strcmp(components[i], "Thread"))
                enabled_components[i] = Traits<Thread>::enabled;
            else if(!strcmp(components[i], "Active"))
                enabled_components[i] = Traits<Active>::enabled;
            else if(!strcmp(components[i], "Periodic_Thread"))
                enabled_components[i] = Traits<Periodic_Thread>::enabled;
            else if(!strcmp(components[i], "RT_Thread"))
                enabled_components[i] = Traits<RT_Thread>::enabled;
            else if(!strcmp(components[i], "Task"))
                enabled_components[i] = Traits<Task>::enabled;
            else if(!strcmp(components[i], "Scheduler"))
                enabled_components[i] = Traits<Scheduler<void>>::enabled;
            else if(!strcmp(components[i], "Address_Space"))
                enabled_components[i] = Traits<Address_Space>::enabled;
            else if(!strcmp(components[i], "Segment"))
                enabled_components[i] = Traits<Segment>::enabled;
            else if(!strcmp(components[i], "Synchronizer"))
                enabled_components[i] = Traits<Synchronizer>::enabled;
            else if(!strcmp(components[i], "Mutex"))
                enabled_components[i] = Traits<Mutex>::enabled;
            else if(!strcmp(components[i], "Semaphore"))
                enabled_components[i] = Traits<Semaphore>::enabled;
            else if(!strcmp(components[i], "Condition"))
                enabled_components[i] = Traits<Condition>::enabled;
            else if(!strcmp(components[i], "Clock"))
                enabled_components[i] = Traits<Clock>::enabled;
            else if(!strcmp(components[i], "Chronometer"))
                enabled_components[i] = Traits<Chronometer>::enabled;
            else if(!strcmp(components[i], "Alarm"))
                enabled_components[i] = Traits<Alarm>::enabled;
            else if(!strcmp(components[i], "Delay"))
                enabled_components[i] = Traits<Delay>::enabled;
            else if(!strcmp(components[i], "Network"))
                enabled_components[i] = Traits<Network>::enabled;
            else if(!strcmp(components[i], "ELP"))
                enabled_components[i] = Traits<ELP>::enabled;
            else if(!strcmp(components[i], "TSTP"))
                enabled_components[i] = Traits<TSTP>::enabled;
            else if(!strcmp(components[i], "ARP"))
                enabled_components[i] = Traits<ARP<void,void,0>>::enabled;
            else if(!strcmp(components[i], "IP"))
                enabled_components[i] = Traits<IP>::enabled;
            else if(!strcmp(components[i], "ICMP"))
                enabled_components[i] = Traits<ICMP>::enabled;
            else if(!strcmp(components[i], "UDP"))
                enabled_components[i] = Traits<UDP>::enabled;
            else if(!strcmp(components[i], "TCP"))
                enabled_components[i] = Traits<TCP>::enabled;
            else if(!strcmp(components[i], "DHCP"))
                enabled_components[i] = Traits<DHCP>::enabled;
            else if(!strcmp(components[i], "IPC"))
                enabled_components[i] = Traits<IPC>::enabled;
            else if(!strcmp(components[i], "Link"))
                enabled_components[i] = Traits<Link<void,false>>::enabled;
            else if(!strcmp(components[i], "Port"))
                enabled_components[i] = Traits<Port<void,false>>::enabled;
            else if(!strcmp(components[i], "Smart_Data"))
                enabled_components[i] = Traits<Smart_Data<void>>::enabled;
            else
                enabled_components[i] = false;
        }
    }

    idx = find_cfg(str_cfg_names, "-MEDIATORS");
    if(idx >= 0) {
        for(unsigned int i = 0; i < ARRAY_MAX; i++) {
            if(!strcmp(mediators[i], "CPU"))
                enabled_mediators[i] = Traits<CPU>::enabled;
            else if(!strcmp(mediators[i], "TSC"))
                enabled_mediators[i] = Traits<TSC>::enabled;
            else if(!strcmp(mediators[i], "MMU"))
                enabled_mediators[i] = Traits<MMU>::enabled;
            else if(!strcmp(mediators[i], "FPU"))
                enabled_mediators[i] = Traits<FPU>::enabled;
            else if(!strcmp(mediators[i], "PMU"))
                enabled_mediators[i] = Traits<PMU>::enabled;
            else if(!strcmp(mediators[i], "Machine"))
                enabled_mediators[i] = Traits<Machine>::enabled;
            else if(!strcmp(mediators[i], "PCI"))
                enabled_mediators[i] = Traits<PCI>::enabled;
            else if(!strcmp(mediators[i], "IC"))
                enabled_mediators[i] = Traits<IC>::enabled;
            else if(!strcmp(mediators[i], "Timer"))
                enabled_mediators[i] = Traits<Timer>::enabled;
            else if(!strcmp(mediators[i], "RTC"))
                enabled_mediators[i] = Traits<RTC>::enabled;
            else if(!strcmp(mediators[i], "UART"))
                enabled_mediators[i] = Traits<UART>::enabled;
            else if(!strcmp(mediators[i], "USB"))
                enabled_mediators[i] = Traits<USB>::enabled;
            else if(!strcmp(mediators[i], "EEPROM"))
                enabled_mediators[i] = Traits<EEPROM>::enabled;
            else if(!strcmp(mediators[i], "Display"))
                enabled_mediators[i] = Traits<Display>::enabled;
            else if(!strcmp(mediators[i], "Serial_Display"))
                enabled_mediators[i] = Traits<Serial_Display>::enabled;
            else if(!strcmp(mediators[i], "Keyboard"))
                enabled_mediators[i] = Traits<Keyboard>::enabled;
            else if(!strcmp(mediators[i], "Serial_Keyboard"))
                enabled_mediators[i] = Traits<Serial_Keyboard>::enabled;
            else if(!strcmp(mediators[i], "Scratchpad"))
                enabled_mediators[i] = Traits<Scratchpad>::enabled;
            else if(!strcmp(mediators[i], "GPIO"))
                enabled_mediators[i] = Traits<GPIO>::enabled;
            else if(!strcmp(mediators[i], "I2C"))
                enabled_mediators[i] = Traits<I2C>::enabled;
            else if(!strcmp(mediators[i], "ADC"))
                enabled_mediators[i] = Traits<ADC>::enabled;
            else if(!strcmp(mediators[i], "FPGA"))
                enabled_mediators[i] = Traits<FPGA>::enabled;
            else if(!strcmp(mediators[i], "NIC"))
                enabled_mediators[i] = Traits<NIC>::enabled;
            else if(!strcmp(mediators[i], "Ethernet"))
                enabled_mediators[i] = Traits<Ethernet>::enabled;
            else if(!strcmp(mediators[i], "IEEE802_15_4"))
                enabled_mediators[i] = Traits<IEEE802_15_4>::enabled;
            else if(!strcmp(mediators[i], "PCNet32"))
                enabled_mediators[i] = Traits<PCNet32>::enabled;
            else if(!strcmp(mediators[i], "C905"))
                enabled_mediators[i] = Traits<C905>::enabled;
            else if(!strcmp(mediators[i], "E100"))
                enabled_mediators[i] = Traits<E100>::enabled;
            else if(!strcmp(mediators[i], "CC2538"))
                enabled_mediators[i] = Traits<CC2538>::enabled;
            else if(!strcmp(mediators[i], "AT86RF"))
                enabled_mediators[i] = Traits<AT86RF>::enabled;
            else if(!strcmp(mediators[i], "GEM"))
                enabled_mediators[i] = Traits<GEM>::enabled;
            else
                enabled_mediators[i] = false;
        }
    }
}
