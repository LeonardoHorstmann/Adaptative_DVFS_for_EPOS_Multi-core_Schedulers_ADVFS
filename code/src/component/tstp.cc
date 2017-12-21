// EPOS Trustful SpaceTime Protocol Implementation

#include <system/config.h>
#ifndef __no_networking__

#include <tstp.h>
#include <utility/math.h>

__BEGIN_SYS

// TSTP::Locator
// Class attributes

// Methods
void TSTP::Locator::update(NIC::Observed * obs, NIC::Protocol prot, NIC::Buffer * b)
{
    Buffer * buf = reinterpret_cast<Buffer*>(b);
    db<TSTP>(TRC) << "TSTP::Locator::update(obs=" << obs << ",buf=" << buf << ")" << endl;
    if(buf->is_microframe)
        buf->sender_distance = buf->frame()->data<Microframe>()->hint();
    else
        buf->my_distance = here() - TSTP::destination(buf).center;
}

void TSTP::Locator::marshal(Buffer * buf)
{
    db<TSTP>(TRC) << "TSTP::Locator::marshal(buf=" << buf << ")" << endl;
    buf->my_distance = here() - TSTP::destination(buf).center;
    buf->sender_distance = buf->my_distance;
}

TSTP::Locator::~Locator()
{
    db<TSTP>(TRC) << "TSTP::~Locator()" << endl;
    TSTP::_nic->detach(this, 0);
}

TSTP::Coordinates TSTP::Locator::here() { return Coordinates(5,5,5); } // TODO

// TSTP::Timekeeper
// Class attributes

// Methods
void TSTP::Timekeeper::update(NIC::Observed * obs, NIC::Protocol prot, NIC::Buffer * b)
{
    Buffer * buf = reinterpret_cast<Buffer*>(b);
    db<TSTP>(TRC) << "TSTP::Timekeeper::update(obs=" << obs << ",buf=" << buf << ")" << endl;
    buf->expiry = TSTP::destination(buf).t1;
}

void TSTP::Timekeeper::marshal(Buffer * buf)
{
    db<TSTP>(TRC) << "TSTP::Timekeeper::marshal(buf=" << buf << ")" << endl;
    buf->expiry = TSTP::destination(buf).t1;
}

TSTP::Timekeeper::~Timekeeper()
{
    db<TSTP>(TRC) << "TSTP::~Timekeeper()" << endl;
    TSTP::_nic->detach(this, 0);
}

// TSTP::Router
// Class attributes

// Methods
void TSTP::Router::update(NIC::Observed * obs, NIC::Protocol prot, NIC::Buffer * b)
{
    Buffer * buf = reinterpret_cast<Buffer*>(b);
    db<TSTP>(TRC) << "TSTP::Router::update(obs=" << obs << ",buf=" << buf << ")" << endl;
    if(buf->is_microframe && !buf->relevant) {
        TSTP::Coordinates::Number distance = TSTP::here() - TSTP::sink();
        assert(distance >= 0);
        buf->relevant = static_cast<unsigned long long>(distance) < buf->sender_distance;
    }
    if(!buf->is_microframe) {
        buf->destined_to_me = TSTP::destination(buf).contains(TSTP::here(), TSTP::now());
        if(buf->my_distance < buf->sender_distance) {
            // Forward the message

            Buffer * send_buf = TSTP::alloc(buf->size());

            // Copy frame contents
            memcpy(send_buf->frame(), buf->frame(), buf->size());

            // Copy Buffer Metainformation
            send_buf->id = buf->id;
            send_buf->destined_to_me = buf->destined_to_me;
            send_buf->downlink = buf->downlink;
            send_buf->expiry = buf->expiry;
            send_buf->origin_time = buf->origin_time;
            send_buf->my_distance = buf->my_distance;
            send_buf->sender_distance = buf->sender_distance;
            send_buf->is_new = false;
            send_buf->is_microframe = false;

            // Calculate offset
            offset(send_buf);

            TSTP::_nic->send(send_buf);
        }
    }
}

void TSTP::Router::marshal(Buffer * buf)
{
    db<TSTP>(TRC) << "TSTP::Router::marshal(buf=" << buf << ")" << endl;
    TSTP::Region dest = TSTP::destination(buf);
    buf->downlink = dest.center == TSTP::sink();
    buf->destined_to_me = dest.contains(TSTP::here(), TSTP::now());

    offset(buf);
}

TSTP::Router::~Router()
{
    db<TSTP>(TRC) << "TSTP::~Router()" << endl;
    TSTP::_nic->detach(this, 0);
}

// TSTP::Security
// Class attributes

// Methods
void TSTP::Security::update(NIC::Observed * obs, NIC::Protocol prot, NIC::Buffer * b)
{
    Buffer * buf = reinterpret_cast<Buffer*>(b);
    db<TSTP>(TRC) << "TSTP::Security::update(obs=" << obs << ",buf=" << buf << ")" << endl;
}

void TSTP::Security::marshal(Buffer * buf)
{
    db<TSTP>(TRC) << "TSTP::Security::marshal(buf=" << buf << ")" << endl;
}

TSTP::Security::~Security()
{
    db<TSTP>(TRC) << "TSTP::~Security()" << endl;
    TSTP::_nic->detach(this, 0);
}


// TSTP
// Class attributes
NIC * TSTP::_nic;
TSTP::Interests TSTP::_interested;
TSTP::Responsives TSTP::_responsives;
TSTP::Observed TSTP::_observed;

// Methods
TSTP::~TSTP()
{
    db<TSTP>(TRC) << "TSTP::~TSTP()" << endl;
    _nic->detach(this, 0);
}

void TSTP::update(NIC::Observed * obs, NIC::Protocol prot, NIC::Buffer * b)
{
    Buffer * buf = reinterpret_cast<Buffer*>(b);
    db<TSTP>(TRC) << "TSTP::update(obs=" << obs << ",buf=" << buf << ")" << endl;

    if(buf->is_microframe)
        return;

    Packet * packet = buf->frame()->data<Packet>();
    switch(packet->type()) {
    case INTEREST: {
        Interest * interest = reinterpret_cast<Interest *>(packet);
        db<TSTP>(INF) << "TSTP::update:interest=" << interest << " => " << *interest << endl;
        // Check for local capability to respond and notify interested observers
        Responsives::List * list = _responsives[interest->unit()]; // TODO: What if sensor can answer multiple formats (e.g. int and float)
        if(list)
            for(Responsives::Element * el = list->head(); el; el = el->next()) {
                Responsive * responsive = el->object();
                if(interest->region().contains(responsive->origin(), now())) {
                    notify(responsive, buf);
                }
            }
    } break;
    case RESPONSE: {
        Response * response = reinterpret_cast<Response *>(packet);
        db<TSTP>(INF) << "TSTP::update:response=" << response << " => " << *response << endl;
        // Check region inclusion and notify interested observers
        Interests::List * list = _interested[response->unit()];
        if(list)
            for(Interests::Element * el = list->head(); el; el = el->next()) {
                Interested * interested = el->object();
                if(interested->region().contains(response->origin(), response->time()))
                    notify(interested, buf);
            }
    } break;
    case COMMAND: {
        Command * command = reinterpret_cast<Command *>(packet);
        db<TSTP>(INF) << "TSTP::update:command=" << command << " => " << *command << endl;
        // Check for local capability to respond and notify interested observers
        Responsives::List * list = _responsives[command->unit()]; // TODO: What if sensor can answer multiple formats (e.g. int and float)
        if(list)
            for(Responsives::Element * el = list->head(); el; el = el->next()) {
                Responsive * responsive = el->object();
                if(command->region().contains(responsive->origin(), now()))
                    notify(responsive, buf);
            }
    } break;
    case CONTROL: break;
    }

    _nic->free(buf);
}

__END_SYS

#endif
