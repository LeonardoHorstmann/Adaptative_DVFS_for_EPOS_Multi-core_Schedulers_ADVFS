// EPOS TI CC2538 IEEE 802.15.4 NIC Mediator Implementation

#include <system/config.h>

#ifdef __mmod_emote3__

#include <machine/cortex/machine.h>
#include <machine/cortex/cc2538.h>
#include <utility/malloc.h>
#include <utility/random.h>

__BEGIN_SYS

// Class attributes
volatile CC2538RF::Reg32 CC2538RF::Timer::_overflow_count;
volatile CC2538RF::Reg32 CC2538RF::Timer::_ints;
CC2538RF::Timer::Time_Stamp CC2538RF::Timer::_int_request_time;
CC2538RF::Timer::Time_Stamp CC2538RF::Timer::_offset;
IC::Interrupt_Handler CC2538RF::Timer::_handler;
bool CC2538RF::Timer::_overflow_match;
bool CC2538RF::Timer::_msb_match;

// TODO: Static because of TSTP_MAC
bool CC2538RF::_cca_done;

CC2538::Device CC2538::_devices[UNITS];

// Methods
CC2538::~CC2538()
{
    db<CC2538>(TRC) << "~CC2538(unit=" << _unit << ")" << endl;
}

int CC2538::send(const Address & dst, const Type & type, const void * data, unsigned int size)
{
    db<CC2538>(TRC) << "CC2538::send(s=" << address() << ",d=" << dst << ",p=" << hex << type << dec << ",d=" << data << ",s=" << size << ")" << endl;

    Buffer * b = alloc(reinterpret_cast<NIC*>(this), dst, type, 0, 0, size);
    memcpy(b->frame()->data<void>(), data, size);
    return send(b);
}

int CC2538::receive(Address * src, Type * type, void * data, unsigned int size)
{
    db<CC2538>(TRC) << "CC2538::receive(s=" << *src << ",p=" << hex << *type << dec << ",d=" << data << ",s=" << size << ") => " << endl;

    Buffer * buf;
    for(buf = 0; !buf; ++_rx_cur_consume %= RX_BUFS) { // _xx_cur_xxx are simple accelerators to avoid scanning the ring buffer from the beginning.
                                                       // Losing a write in a race condition is assumed to be harmless. The FINC + CAS alternative seems too expensive.
        unsigned int idx = _rx_cur_consume;
        if(_rx_bufs[idx]->lock()) {
            if(_rx_bufs[idx]->size() > 0)
                buf = _rx_bufs[idx];
            else
                _rx_bufs[idx]->unlock();
        }
    }

    Address dst;
    unsigned int ret = MAC::unmarshal(buf, src, &dst, type, data, size);
    free(buf);

    db<CC2538>(INF) << "CC2538::received " << ret << " bytes" << endl;

    return ret;
}

CC2538::Buffer * CC2538::alloc(NIC * nic, const Address & dst, const Type & type, unsigned int once, unsigned int always, unsigned int payload)
{
    db<CC2538>(TRC) << "CC2538::alloc(s=" << address() << ",d=" << dst << ",p=" << hex << type << dec << ",on=" << once << ",al=" << always << ",ld=" << payload << ")" << endl;

    // Initialize the buffer
    Buffer * buf = new (SYSTEM) Buffer(nic, once + always + payload + sizeof(MAC::Header));
    MAC::marshal(buf, address(), dst, type);

    return buf;
}

int CC2538::send(Buffer * buf)
{
    db<CC2538>(TRC) << "CC2538::send(buf=" << buf << ")" << endl;
    db<CC2538>(INF) << "CC2538::send:frame=" << buf->frame() << " => " << *(buf->frame()) << endl;

    unsigned int size = MAC::send(buf);

    if(size) {
        _statistics.tx_packets++;
        _statistics.tx_bytes += size;
    } else
        db<CC2538>(WRN) << "CC2538::send(buf=" << buf << ")" << " => failed!" << endl;

    return size;
}

void CC2538::free(Buffer * buf)
{
    db<CC2538>(TRC) << "CC2538::free(buf=" << buf << ")" << endl;

    _statistics.rx_packets++;
    _statistics.rx_bytes += buf->size();

    buf->size(0);
    buf->unlock();
}

void CC2538::reset()
{
    db<CC2538>(TRC) << "CC2538::reset()" << endl;

    // Reset statistics
    new (&_statistics) Statistics;
}

void CC2538::handle_int()
{
    char rssi = xreg(RSSI); // TODO: get the RSSI appended by hardware at the end of the frame when filtering is enabled

    db<CC2538>(TRC) << "CC2538::handle_int()" << endl;

    Reg32 irqrf0 = sfr(RFIRQF0);
    Reg32 irqrf1 = sfr(RFIRQF1);
    Reg32 errf = sfr(RFERRF);
    sfr(RFIRQF0) = irqrf0 & INT_RXPKTDONE; //INT_RXPKTDONE is polled by rx_done()
    sfr(RFIRQF1) = irqrf1 & INT_TXDONE; //INT_TXDONE is polled by tx_done()
    sfr(RFERRF) = 0;
    db<CC2538>(INF) << "CC2538::handle_int:RFIRQF0=" << hex << irqrf0 << endl;
    db<CC2538>(INF) << "CC2538::handle_int:RFIRQF1=" << hex << irqrf1 << endl;
    db<CC2538>(INF) << "CC2538::handle_int:RFERRF=" << hex << errf << endl;

    if(irqrf0 & INT_FIFOP) { // Frame received
        db<CC2538>(TRC) << "CC2538::handle_int:receive()" << endl;
        if(CC2538RF::filter()) {
            Buffer * buf = 0;
            unsigned int idx = _rx_cur_produce;
            for(unsigned int count = RX_BUFS; count; count--, ++idx %= RX_BUFS) {
                if(_rx_bufs[idx]->lock()) {
                    buf = _rx_bufs[idx];
                    break;
                }
            }
            _rx_cur_produce = (idx + 1) % RX_BUFS;

            if(buf) {
                buf->rssi = rssi;
                buf->size(CC2538RF::copy_from_nic(buf->frame()));
                if(MAC::pre_notify(buf)) {
                    db<CC2538>(TRC) << "CC2538::handle_int:receive(b=" << buf << ") => " << *buf << endl;
                    bool notified = notify(reinterpret_cast<IEEE802_15_4::Header *>(buf->frame())->type(), buf);
                    if(!MAC::post_notify(buf) && !notified)
                        buf->unlock(); // No one was waiting for this frame, so make it available for receive()
                } else {
                    db<CC2538>(TRC) << "CC2538::handle_int: frame dropped by MAC"  << endl;
                    buf->size(0);
                    buf->unlock();
                }
            } else
                CC2538RF::drop();
        }
    }
}

void CC2538::int_handler(const IC::Interrupt_Id & interrupt)
{
    CC2538 * dev = get_by_interrupt(interrupt);

    db<CC2538>(TRC) << "Radio::int_handler(int=" << interrupt << ",dev=" << dev << ")" << endl;

    if(!dev)
        db<CC2538>(WRN) << "Radio::int_handler: handler not assigned!" << endl;
    else
        dev->handle_int();
}


// TSTP binding
template<typename Radio>
void TSTP_MAC<Radio>::free(Buffer * b) { b->nic()->free(reinterpret_cast<NIC::Buffer*>(b)); }
template void TSTP_MAC<CC2538RF>::free(Buffer * b);

__END_SYS

#endif
