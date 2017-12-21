// EPOS Trustful SpaceTime Protocol Initialization

#include <system/config.h>
#ifndef __no_networking__

#include <tstp.h>

__BEGIN_SYS

TSTP::TSTP()
{
    db<TSTP>(TRC) << "TSTP::TSTP()" << endl;
    _nic->attach(this, NIC::TSTP);
}

void TSTP::Locator::bootstrap()
{
    db<TSTP>(TRC) << "TSTP::Locator::bootstrap()" << endl;
}

void TSTP::Timekeeper::bootstrap()
{
    db<TSTP>(TRC) << "TSTP::Timekeeper::bootstrap()" << endl;
}

void TSTP::Router::bootstrap()
{
    db<TSTP>(TRC) << "TSTP::Router::bootstrap()" << endl;
}

void TSTP::Security::bootstrap()
{
    db<TSTP>(TRC) << "TSTP::Security::bootstrap()" << endl;
}

void TSTP::init(unsigned int unit)
{
    db<Init, TSTP>(TRC) << "TSTP::init(u=" << unit << ")" << endl;
    _nic = new (SYSTEM) NIC(unit);
    new (SYSTEM) TSTP::Locator;
    new (SYSTEM) TSTP::Timekeeper;
    new (SYSTEM) TSTP::Router;
    new (SYSTEM) TSTP::Security;
    new (SYSTEM) TSTP;

    TSTP::Locator::bootstrap();
    TSTP::Timekeeper::bootstrap();
    TSTP::Router::bootstrap();
    TSTP::Security::bootstrap();
}

__END_SYS

#endif
