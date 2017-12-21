// EPOS TI CC2538 IEEE 802.15.4 NIC Mediator Initialization

#include <system/config.h>
#if defined(__NIC_H) && defined(__mmod_emote3__)

#include <machine/cortex/machine.h>
#include <machine/cortex/cc2538.h>

__BEGIN_SYS

CC2538::CC2538(unsigned int unit): _unit(unit), _rx_cur_consume(0), _rx_cur_produce(0)
{
    db<CC2538>(TRC) << "CC2538(unit=" << unit << ")" << endl;

    // Initialize RX buffer pool
    for(unsigned int i = 0; i < RX_BUFS; i++)
        _rx_bufs[i] = new (SYSTEM) Buffer(0, 0);

    // Set Address
    ffsm(SHORT_ADDR0) = _address[0];
    ffsm(SHORT_ADDR1) = _address[1];
    _address[0] = ffsm(SHORT_ADDR0);
    _address[1] = ffsm(SHORT_ADDR1);

    // TODO: Define what will be used for each MAC
    //xreg(FRMFILT0) |= FRAME_FILTER_EN; // Enable frame filtering
    xreg(FRMFILT0) &= ~FRAME_FILTER_EN; // Disable frame filtering
    xreg(FRMFILT1) &= ~ACCEPT_FT2_ACK; // ACK frames are handled only when expected

    //xreg(SRCMATCH) |= SRC_MATCH_EN; // Enable automatic source address matching
    xreg(SRCMATCH) &= ~SRC_MATCH_EN; // Disable automatic source address matching

    xreg(FRMCTRL0) |= AUTO_CRC; // Enable auto-CRC

    channel(13);

    xreg(FRMCTRL0) |= AUTO_ACK; // Enable auto ACK

    reset(); // Reset statistics

    xreg(FRMCTRL1) &= ~SET_RXENMASK_ON_TX; // Do not enter receive mode after ISTXON

    // Enable useful device interrupts
    // WARNING: do not enable INT_TXDONE, because CC2538RF::tx_done() handles it
    // WARNING: do not enable INT_RXPKTDONE, because CC2538RF::rx_done() handles it
    xreg(RFIRQM0) = INT_FIFOP;
    xreg(RFIRQM1) = 0;
    xreg(RFERRM) = 0;

    MAC::constructor_epilogue(); // Device is configured, let the MAC use it
}


void CC2538::init(unsigned int unit)
{
    db<Init, CC2538>(TRC) << "Radio::init(unit=" << unit << ")" << endl;

    // Enable debug signals
    xreg(RFC_OBS_CTRL0) = SIGNAL_TX_ACTIVE;
    xreg(RFC_OBS_CTRL1) = SIGNAL_RX_ACTIVE;
    cctest(CCTEST_OBSSEL4) = OBSSEL_SIG0;
    cctest(CCTEST_OBSSEL6) = OBSSEL_SIG1;

    Timer::init();

    // Initialize the device
    CC2538 * dev = new (SYSTEM) CC2538(unit);

    // Register the device
    _devices[unit].device = dev;
    _devices[unit].interrupt = IC::INT_NIC0_RX;

    // Install interrupt handler
    IC::int_vector(_devices[unit].interrupt, &int_handler);

    // Enable interrupts for device
    IC::enable(IC::INT_NIC0_RX);
}

void CC2538::Timer::init()
{
    db<Init, CC2538>(TRC) << "Radio::Timer::init()" << endl;
    mactimer(MTCTRL) &= ~MTCTRL_RUN; // Stop counting
    mactimer(MTIRQM) = 0; // Mask interrupts
    mactimer(MTIRQF) = 0; // Clear interrupts
    mactimer(MTCTRL) &= ~MTCTRL_SYNC; // We can't use the sync feature because we want to change the count and overflow values when the timer is stopped
    mactimer(MTCTRL) |= MTCTRL_LATCH_MODE; // count and overflow will be latched at once
    IC::int_vector(IC::INT_NIC0_TIMER, &int_handler); // Register and enable interrupt at IC
    IC::enable(IC::INT_NIC0_TIMER);
    int_enable(INT_OVERFLOW_PER); // Enable overflow interrupt
    mactimer(MTCTRL) |= MTCTRL_RUN; // Start counting
}

__END_SYS

#endif
