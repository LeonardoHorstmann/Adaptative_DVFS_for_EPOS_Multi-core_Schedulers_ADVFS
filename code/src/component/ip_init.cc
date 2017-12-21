// EPOS IP Protocol Initialization

#include <system/config.h>
#ifndef __no_networking__

#include <ip.h>

__BEGIN_SYS

template<unsigned int UNIT>
IP::IP(unsigned int nic)
: _nic(nic), _arp(&_nic, this), _address(Traits<IP>::Config<UNIT>::ADDRESS), _netmask(Traits<IP>::Config<UNIT>::NETMASK), _broadcast((_address & _netmask) | ~_netmask), _gateway(Traits<IP>::Config<UNIT>::GATEWAY)
{
    db<IP>(TRC) << "IP::IP(nic=" << &_nic << ") => " << this << endl;

    _nic.attach(this, NIC::IP);

    if(Traits<IP>::Config<UNIT>::TYPE == Traits<IP>::MAC)
        config_by_mac();
    else if(Traits<IP>::Config<UNIT>::TYPE == Traits<IP>::INFO)
        config_by_info();
    else if(Traits<IP>::Config<UNIT>::TYPE == Traits<IP>::RARP)
        config_by_rarp();
    else if(Traits<IP>::Config<UNIT>::TYPE == Traits<IP>::DHCP)
        config_by_dhcp();

    _router.insert(&_nic, this, &_arp, _address & _netmask, _address, _netmask);

    if(_gateway) {
        _router.insert(&_nic, this, &_arp, Address::NULL, _gateway, Address::NULL); // Default route must be the last one in table
        _arp.resolve(_gateway);
    }
}

void IP::init(unsigned int unit)
{
    db<Init, IP>(TRC) << "IP::init(u=" << unit << ")" << endl;

    _networks[unit] = new (SYSTEM) IP(unit);
}

__END_SYS

#endif
