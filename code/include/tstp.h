// EPOS Trustful SpaceTime Protocol Declarations

#include <ieee802_15_4.h>

#ifndef __tstp_common_h
#define __tstp_common_h

#include <utility/geometry.h>
#include <rtc.h>

__BEGIN_SYS

class TSTP_Common: public IEEE802_15_4
{
protected:
    static const unsigned int RADIO_RANGE = 1700; // Approximated radio range of nodes, in centimeters
    static const bool drop_expired = true;

public:
    static const unsigned int PAN = 10; // Nodes
    static const unsigned int SAN = 100; // Nodes
    static const unsigned int LAN = 10000; // Nodes
    static const unsigned int NODES = Traits<Build>::NODES;

    // Version
    // This field is packed first and matches the Frame Type field in the Frame Control in IEEE 802.15.4 MAC.
    // A version number above 4 renders TSTP into the reserved frame type zone and should avoid interference.
    enum Version {
        V0 = 4
    };

    // MAC definitions
    typedef CPU::Reg16 Frame_ID;
    typedef CPU::Reg32 Hint;
    typedef NIC_Common::CRC16 CRC;
    typedef unsigned short MF_Count;

    // Packet Types
    typedef unsigned char Type;
    enum {
        INTEREST  = 0,
        RESPONSE  = 1,
        COMMAND   = 2,
        CONTROL   = 3
    };

    // Scale for local network's geographic coordinates
    enum Scale {
        CMx50_8  = 0,
        CM_16    = 1,
        CMx25_16 = 2,
        CM_32    = 3
    };
    static const Scale SCALE = (NODES <= PAN) ? CMx50_8 : (NODES <= SAN) ? CM_16 : (NODES <= LAN) ? CMx25_16 : CM_32;

    // Time
    typedef RTC::Microsecond Microsecond;
    typedef unsigned long long Time;
    typedef long Time_Offset;

    // Geographic Coordinates
    template<Scale S>
    struct _Coordinates: public Point<char, 3>
    {
        typedef char Number;

        _Coordinates(Number x = 0, Number y = 0, Number z = 0): Point<Number, 3>(x, y, z) {}
    } __attribute__((packed));
    typedef _Coordinates<SCALE> Coordinates;

    // Geographic Region in a time interval (not exactly Spacetime, but ...)
    template<Scale S>
    struct _Region: public Sphere<typename _Coordinates<S>::Number>
    {
        typedef typename _Coordinates<S>::Number Number;
        typedef Sphere<Number> Base;

        _Region(const Coordinates & c, const Number & r, const Time & _t0, const Time & _t1): Base(c, r), t0(_t0), t1(_t1) {}

        bool contains(const Coordinates & c, const Time & t) const { return ((Base::center - c) <= Base::radius) && ((t >= t0) && (t <= t1)); }

        friend Debug & operator<<(Debug & db, const _Region & r) {
            db << "{" << reinterpret_cast<const Base &>(r) << ",t0=" << r.t0 << ",t1=" << r.t1 << "}";
            return db;
        }

        Time t0;
        Time t1;
    } __attribute__((packed));
    typedef _Region<SCALE> Region;

    // MAC Preamble Microframe
    class Microframe
    {
    public:
        Microframe() {}

        Microframe(bool all_listen, const Frame_ID & id, const MF_Count & count, const Hint & hint = 0)
        : _al_count_idh((id & 0xf) | ((count & 0x7ff) << 4) | (static_cast<unsigned int>(all_listen) << 15)), _idl(id & 0xff), _hint(hint) {}

        MF_Count count() const { return (_al_count_idh & (0x7ff << 4)) >> 4; }
        MF_Count dec_count() {
            MF_Count c = count();
            _al_count_idh &= ~(0x7ff << 4);
            _al_count_idh |= (c-1) << 4;
            return c;
        }

        Frame_ID id() const { return ((_al_count_idh | 0xf) << 8) + _idl; }
        void id(Frame_ID id) {
            _al_count_idh &= 0xfff0;
            _al_count_idh |= (id & 0xf00) >> 8;
            _idl = id & 0xff;
        }

        void all_listen(bool all_listen) {
            if(all_listen)
                _al_count_idh |= (1 << 15);
            else
                _al_count_idh &= ~(1 << 15);
        }
        bool all_listen() const { return _al_count_idh & (1 << 15); }

        Hint hint() const { return _hint; }
        void hint(const Hint & h) { _hint = h; }

        friend Debug & operator<<(Debug & db, const Microframe & m) {
            db << "{al=" << m.all_listen() << ",c=" << m.count() << ",id=" << m.id() << ",h=" << m._hint << ",crc=" << m._crc << "}";
            return db;
        }

    private:
        unsigned short _al_count_idh; // all_listen : 1
                                      // count : 11
                                      // id MSBs: 4
        unsigned char _idl;           // id LSBs: 8
        Hint _hint;
        CRC _crc;
    } __attribute__((packed));

    // Packet Header
    template<Scale S>
    class _Header
    {
        // Format
        // Bit 0      3    5  6    0                0         0         0         0         0         0         0         0
        //     +------+----+--+----+----------------+--- ~ ---+--- ~ ---+--- ~ ---+--- ~ ---+--- ~ ---+--- ~ ---+--- ~ ---+--- ~ ---+
        //     | ver  |type|tr|scal|   confidence   |   o.t   |   o.x   |   o.y   |   o.z   |   l.t   |   l.x   |   l.y   |   l.z   |
        //     +------+----+--+----+----------------+--- ~ ---+--- ~ ---+--- ~ ---+--- ~ ---+--- ~ ---+--- ~ ---+--- ~ ---+--- ~ ---+
        // Bits          8                  8            64     8/16/32   8/16/32   8/16/32      64     8/16/32   8/16/32   8/16/32

    public:
        _Header(const Type & t, bool tr = false, unsigned char c = 0, const Time & ot = 0, const Coordinates & o = 0, const Coordinates & l = 0, const Version & v = V0)
        : _config((S & 0x03) << 6 | tr << 5 | (t & 0x03) << 3 | (v & 0x07)), _confidence(c), _time(ot), _origin(o), _last_hop(l) {}

        Version version() const { return static_cast<Version>(_config & 0x07); }
        void version(const Version & v) { _config = (_config & 0xf8) | (v & 0x07); }

        Type type() const { return static_cast<Type>((_config >> 3) & 0x03); }
        void type(const Type & t) { _config = (_config & 0xe7) | ((t & 0x03) << 3); }

        bool time_request() const { return (_config >> 5) & 0x01; }
        void time_request(bool tr) { _config = (_config & 0xdf) | (tr << 5); }

        Scale scale() const { return static_cast<Scale>((_config >> 6) & 0x03); }
        void scale(const Scale & s) { _config = (_config & 0x3f) | (s & 0x03) << 6; }

        const Coordinates & origin() const { return _origin; }
        void origin(const Coordinates & c) { _origin = c; }

        const Coordinates & last_hop() const { return _last_hop; }
        void last_hop(const Coordinates & c) { _last_hop = c; }

        Time time() const { return _time; }
        void time(const Time & t) { _time = t; }

        Time last_hop_time() const { return _last_hop_time; }
        void last_hop_time(const Time & t) { _last_hop_time = t; }

        friend Debug & operator<<(Debug & db, const _Header & h) {
            db << "{v=" << h.version() - V0 << ",t=" << ((h.type() == INTEREST) ? 'I' :  (h.type() == RESPONSE) ? 'R' : (h.type() == COMMAND) ? 'C' : 'P') << ",tr=" << h.time_request() << ",s=" << h.scale() << ",ot=" << h._time << ",o=" << h._origin << ",lt=" << h._last_hop_time << ",l=" << h._last_hop << "}";
            return db;
        }

    protected:
        unsigned char _config;
        unsigned char _confidence;
        Time _time;
        Coordinates _origin;
        Time _last_hop_time; // TODO: change to Time_Offset
        Coordinates _last_hop;
    } __attribute__((packed));
    typedef _Header<SCALE> Header;

    // Frame
    template<Scale S>
    class _Frame: public Header
    {
    public:
        static const unsigned int MTU = 100;
        typedef unsigned char Data[MTU];

    public:
        _Frame() {}
        _Frame(const Type & type, const Address & src, const Address & dst): Header(type) {} // Just for NIC compatibility
        _Frame(const Type & type, const Address & src, const Address & dst, const void * data, unsigned int size): Header(type) { memcpy(_data, data, size); }

        Header * header() { return this; }

        Reg8 length() const { return MTU; } // Fixme: placeholder

        template<typename T>
        T * data() { return reinterpret_cast<T *>(&_data); }

        friend Debug & operator<<(Debug & db, const _Frame & p) {
            db << "{h=" << reinterpret_cast<const Header &>(p) << ",d=" << p._data << "}";
            return db;
        }

    private:
        Data _data;
    } __attribute__((packed, may_alias));
    typedef _Frame<SCALE> Frame;

    typedef Frame PDU;


    // TSTP encodes SI Units similarly to IEEE 1451 TEDs
    class Unit
    {
    public:
        // Formats
        // Bit       31                                 16                                     0
        //         +--+----------------------------------+-------------------------------------+
        // Digital |0 | type                             | dev                                 |
        //         +--+----------------------------------+-------------------------------------+

        // Bit       31   29   27     24     21     18     15     12      9      6      3      0
        //         +--+----+----+------+------+------+------+------+------+------+------+------+
        // SI      |1 |NUM |MOD |sr+4  |rad+4 |m+4   |kg+4  |s+4   |A+4   |K+4   |mol+4 |cd+4  |
        //         +--+----+----+------+------+------+------+------+------+------+------+------+
        // Bits     1   2    2     3      3      3      3      3      3      3      3      3


        // Valid values for field SI
        enum {
            DIGITAL = 0 << 31, // The Unit is plain digital data. Subsequent 15 bits designate the data type. Lower 16 bits are application-specific, usually a device selector.
            SI      = 1 << 31  // The Unit is SI. Remaining bits are interpreted as specified here.
        };

        // Valid values for field NUM
        enum {
            I32 = 0 << 29, // Value is an integral number stored in the 32 last significant bits of a 32-bit big-endian integer.
            I64 = 1 << 29, // Value is an integral number stored in the 64 last significant bits of a 64-bit big-endian integer.
            F32 = 2 << 29, // Value is a real number stored as an IEEE 754 binary32 big-endian floating point.
            D64 = 3 << 29, // Value is a real number stored as an IEEE 754 binary64 big-endian doulbe precision floating point.
            NUM = D64      // AND mask to select NUM bits
        };

        // Valid values for field MOD
        enum {
            DIR     = 0 << 27, // Unit is described by the product of SI base units raised to the powers recorded in the remaining fields.
            DIV     = 1 << 27, // Unit is U/U, where U is described by the product SI base units raised to the powers recorded in the remaining fields.
            LOG     = 2 << 27, // Unit is log_e(U), where U is described by the product of SI base units raised to the powers recorded in the remaining fields.
            LOG_DIV = 3 << 27, // Unit is log_e(U/U), where U is described by the product of SI base units raised to the powers recorded in the remaining fields.
            MOD = D64          // AND mask to select MOD bits
        };

        // Masks to select the SI units
        enum {
            SR      = 7 << 24,
            RAD     = 7 << 21,
            M       = 7 << 18,
            KG      = 7 << 15,
            S       = 7 << 12,
            A       = 7 <<  9,
            K       = 7 <<  6,
            MOL     = 7 <<  3,
            CD      = 7 <<  0
        };

        // Typical SI Quantities
        enum Quantity {
             //                        si      | mod       | sr            | rad           |  m            |  kg           |  s            |  A            |  K            |  mol          |  cd
             Length                  = 1 << 31 | DIR << 27 | (4 + 0) << 24 | (4 + 0) << 21 | (4 + 1) << 18 | (4 + 0) << 15 | (4 + 0) << 12 | (4 + 0) << 9  | (4 + 0) << 6  | (4 + 0) << 3  | (4 + 0),
             Mass                    = 1 << 31 | DIR << 27 | (4 + 0) << 24 | (4 + 0) << 21 | (4 + 0) << 18 | (4 + 1) << 15 | (4 + 0) << 12 | (4 + 0) << 9  | (4 + 0) << 6  | (4 + 0) << 3  | (4 + 0),
             Time                    = 1 << 31 | DIR << 27 | (4 + 0) << 24 | (4 + 0) << 21 | (4 + 0) << 18 | (4 + 0) << 15 | (4 + 1) << 12 | (4 + 0) << 9  | (4 + 0) << 6  | (4 + 0) << 3  | (4 + 0),
             Current                 = 1 << 31 | DIR << 27 | (4 + 0) << 24 | (4 + 0) << 21 | (4 + 0) << 18 | (4 + 0) << 15 | (4 + 0) << 12 | (4 + 1) << 9  | (4 + 0) << 6  | (4 + 0) << 3  | (4 + 0),
             Electric_Current        = Current,
             Temperature             = 1 << 31 | DIR << 27 | (4 + 0) << 24 | (4 + 0) << 21 | (4 + 0) << 18 | (4 + 0) << 15 | (4 + 0) << 12 | (4 + 0) << 9  | (4 + 1) << 6  | (4 + 0) << 3  | (4 + 0),
             Amount_of_Substance     = 1 << 31 | DIR << 27 | (4 + 0) << 24 | (4 + 0) << 21 | (4 + 0) << 18 | (4 + 0) << 15 | (4 + 0) << 12 | (4 + 0) << 9  | (4 + 0) << 6  | (4 + 1) << 3  | (4 + 0),
             Liminous_Intensity      = 1 << 31 | DIR << 27 | (4 + 0) << 24 | (4 + 0) << 21 | (4 + 0) << 18 | (4 + 0) << 15 | (4 + 0) << 12 | (4 + 0) << 9  | (4 + 0) << 6  | (4 + 0) << 3  | (4 + 1),
             Area                    = 1 << 31 | DIR << 27 | (4 + 0) << 24 | (4 + 0) << 21 | (4 + 2) << 18 | (4 + 0) << 15 | (4 + 0) << 12 | (4 + 0) << 9  | (4 + 0) << 6  | (4 + 0) << 3  | (4 + 0),
             Volume                  = 1 << 31 | DIR << 27 | (4 + 0) << 24 | (4 + 0) << 21 | (4 + 3) << 18 | (4 + 0) << 15 | (4 + 0) << 12 | (4 + 0) << 9  | (4 + 0) << 6  | (4 + 0) << 3  | (4 + 0),
             Speed                   = 1 << 31 | DIR << 27 | (4 + 0) << 24 | (4 + 0) << 21 | (4 + 1) << 18 | (4 + 0) << 15 | (4 - 1) << 12 | (4 + 0) << 9  | (4 + 0) << 6  | (4 + 0) << 3  | (4 + 0),
             Velocity                = Speed,
             Acceleration            = 1 << 31 | DIR << 27 | (4 + 0) << 24 | (4 + 0) << 21 | (4 + 1) << 18 | (4 + 0) << 15 | (4 - 2) << 12 | (4 + 0) << 9  | (4 + 0) << 6  | (4 + 0) << 3  | (4 + 0)
         };

        // SI Factors
        typedef char Factor;
        enum {
         // Name           Code         Symbol    Factor
            ATTO        = (8 - 8), //     a       0.000000000000000001
            FEMTO       = (8 - 7), //     f       0.000000000000001
            PICO        = (8 - 6), //     p       0.000000000001
            NANO        = (8 - 5), //     n       0.000000001
            MICRO       = (8 - 4), //     μ       0.000001
            MILI        = (8 - 3), //     m       0.001
            CENTI       = (8 - 2), //     c       0.01
            DECI        = (8 - 1), //     d       0.1
            NONE        = (8    ), //     -       1
            DECA        = (8 + 1), //     da      10
            HECTO       = (8 + 2), //     h       100
            KILO        = (8 + 3), //     k       1000
            MEGA        = (8 + 4), //     M       1000000
            GIGA        = (8 + 5), //     G       1000000000
            TERA        = (8 + 6), //     T       1000000000000
            PETA        = (8 + 7)  //     P       1000000000000000
        };


        template<int N>
        struct Get { typedef typename SWITCH<N, CASE<I32, long, CASE<I64, long long, CASE<DEFAULT, long>>>>::Result Type; };

        template<typename T>
        struct GET { enum { NUM = I32 }; };

    public:
        Unit(unsigned long u) { _unit = u; }

        operator unsigned long() const { return _unit; }

        int sr()  const { return ((_unit & SR)  >> 24) - 4 ; }
        int rad() const { return ((_unit & RAD) >> 21) - 4 ; }
        int m()   const { return ((_unit & M)   >> 18) - 4 ; }
        int kg()  const { return ((_unit & KG)  >> 15) - 4 ; }
        int s()   const { return ((_unit & S)   >> 12) - 4 ; }
        int a()   const { return ((_unit & A)   >>  9) - 4 ; }
        int k()   const { return ((_unit & K)   >>  6) - 4 ; }
        int mol() const { return ((_unit & MOL) >>  3) - 4 ; }
        int cd()  const { return ((_unit & CD)  >>  0) - 4 ; }

        friend Debug & operator<<(Debug & db, const Unit & u) {
            if(u & SI) {
                db << "{SI";
                switch(u & MOD) {
                case DIR: break;
                case DIV: db << "[U/U]"; break;
                case LOG: db << "[log(U)]"; break;
                case LOG_DIV: db << "[log(U/U)]";
                };
                switch(u & NUM) {
                case I32: db << ".I32"; break;
                case I64: db << ".I64"; break;
                case F32: db << ".F32"; break;
                case D64: db << ".D64";
                }
                db << ':';
                if(u.sr())
                    db << "sr^" << u.sr();
                if(u.rad())
                    db << "rad^" << u.rad();
                if(u.m())
                    db << "m^" << u.m();
                if(u.kg())
                    db << "kg^" << u.kg();
                if(u.s())
                    db << "s^" << u.s();
                if(u.a())
                    db << "A^" << u.a();
                if(u.k())
                    db << "K^" << u.k();
                if(u.mol())
                    db << "mol^" << u.mol();
                if(u.cd())
                    db << "cdr^" << u.cd();
            } else
                db << "{D:" << "l=" <<  (u >> 16);
            db << "}";
            return db;
        }

    private:
        unsigned long _unit;
    } __attribute__((packed));

    // SI values (either integer32, integer64, float32, double64)
    template<int NUM>
    class Value
    {
    public:
        Value(long int v): _value(v) {}

        operator long int() { return _value; }

    private:
        long int _value;
    };

    // Precision or Error in SI values, expressed as 10^Error
    typedef char Precision;
    typedef char Error;
};

template<>
struct TSTP_Common::_Coordinates<TSTP_Common::CM_16>: public Point<short, 3>
{
    typedef short Number;

    _Coordinates(Number x = 0, Number y = 0, Number z = 0): Point<Number, 3>(x, y, z) {}
} __attribute__((packed));

template<>
struct TSTP_Common::_Coordinates<TSTP_Common::CMx25_16>: public Point<short, 3>
{
    typedef short Number;

    _Coordinates(Number x = 0, Number y = 0, Number z = 0): Point<Number, 3>(x, y, z) {}
} __attribute__((packed));

template<>
struct TSTP_Common::_Coordinates<TSTP_Common::CM_32>: public Point<long, 3>
{
    typedef long Number;

    _Coordinates(Number x = 0, Number y = 0, Number z = 0): Point<Number, 3>(x, y, z) {}
} __attribute__((packed));

template<>
class TSTP_Common::Value<TSTP_Common::Unit::I64>
{
public:
    Value(long long int v): _value(v) {}

    operator long long int() { return _value; }

public:
    long long int _value;
};

template<>
class TSTP_Common::Value<TSTP_Common::Unit::F32>
{
public:
    Value(float v): _value(v) {}

    operator float() { return _value; }

private:
    float _value;
};

template<>
class TSTP_Common::Value<TSTP_Common::Unit::D64>
{
public:
    Value(double v): _value(v) {}

    operator double() { return _value; }

private:
    double _value;
};

__END_SYS

#endif

#ifndef __tstp_h
#define __tstp_h

#include <utility/observer.h>
#include <utility/buffer.h>
#include <utility/hash.h>
#include <network.h>

__BEGIN_SYS

class TSTP: public TSTP_Common, private NIC::Observer
{
    template<typename> friend class Smart_Data;

public:
    // Buffer wrapper to compile TSTP when it's disabled
    template<typename T>
    class Buffer_Wrapper: public T, public NIC_Common::TSTP_Metadata {};
    typedef IF<Traits<TSTP>::enabled, NIC::Buffer, Buffer_Wrapper<NIC::Buffer>>::Result Buffer;

    // Packet
    static const unsigned int MTU = NIC::MTU - sizeof(Header);
    template<Scale S>
    class _Packet: public Header
    {
    private:
        typedef unsigned char Data[MTU];

    public:
        _Packet() {}

        Header * header() { return this; }

        template<typename T>
        T * data() { return reinterpret_cast<T *>(&_data); }

        friend Debug & operator<<(Debug & db, const _Packet & p) {
            db << "{h=" << reinterpret_cast<const Header &>(p) << ",d=" << p._data << "}";
            return db;
        }

    private:
        Data _data;
    } __attribute__((packed));
    typedef _Packet<SCALE> Packet;


    // TSTP observer/d conditioned to a message's address (ID)
    typedef Data_Observer<Buffer, int> Observer;
    typedef Data_Observed<Buffer, int> Observed;


    // Hash to store TSTP Observers by type
    class Interested;
    typedef Hash<Interested, 10, Unit> Interests;
    class Responsive;
    typedef Hash<Responsive, 10, Unit> Responsives;


    // TSTP Messages
    // Each TSTP message is encapsulated in a single package. TSTP does not need nor supports fragmentation.

    // Interest/Response Modes
    enum Mode {
        // Response
        SINGLE = 0, // Only one response is desired for each interest job (desired, but multiple responses are still possible)
        ALL    = 1, // All possible responses (e.g. from different sensors) are desired
        // Interest
        DELETE = 2  // Revoke an interest
    };

    // Interest Message
    class Interest: public Header
    {
    public:
        Interest(const Region & region, const Unit & unit, const Mode & mode, const Error & precision, const Microsecond & expiry, const Microsecond & period = 0)
        : Header(INTEREST, 0, 0, now(), here(), here()), _region(region), _unit(unit), _mode(mode), _precision(0), _expiry(expiry), _period(period) {}

        const Unit & unit() const { return _unit; }
        const Region & region() const { return _region; }
        Microsecond period() const { return _period; }
        Time expiry() const { return _time + _expiry; }
        Mode mode() const { return static_cast<Mode>(_mode); }
        Error precision() const { return static_cast<Error>(_precision); }

        bool time_triggered() { return _period; }
        bool event_driven() { return !time_triggered(); }

        friend Debug & operator<<(Debug & db, const Interest & m) {
            db << reinterpret_cast<const Header &>(m) << ",u=" << m._unit << ",m=" << ((m._mode == ALL) ? 'A' : 'S') << ",e=" << int(m._precision) << ",x=" << m._expiry << ",re=" << m._region << ",p=" << m._period;
            return db;
        }

    protected:
        Region _region;
        Unit _unit;
        unsigned char _mode : 2;
        unsigned char _precision : 6;
        Time_Offset _expiry;
        Microsecond _period;
    } __attribute__((packed));

    // Response (Data) Message
    class Response: public Header
    {
    private:
        typedef unsigned char Data[MTU - sizeof(Unit) - sizeof(Error) - sizeof(Time)];

    public:
        Response(const Unit & unit, const Error & error = 0, const Time & expiry = 0)
        : Header(RESPONSE, 0, 0, now(), here(), here()), _unit(unit), _error(error), _expiry(expiry) {}

        const Unit & unit() const { return _unit; }
        Time expiry() const { return _time + _expiry; }
        Error error() const { return _error; }

        template<typename T>
        void value(const T & v) { *reinterpret_cast<Value<Unit::GET<T>::NUM> *>(&_data) = v; }

        template<typename T>
        T value() { return *reinterpret_cast<Value<Unit::GET<T>::NUM> *>(&_data); }

        template<typename T>
        T * data() { return reinterpret_cast<T *>(&_data); }

        friend Debug & operator<<(Debug & db, const Response & m) {
            db << reinterpret_cast<const Header &>(m) << ",u=" << m._unit << ",e=" << int(m._error) << ",x=" << m._expiry << ",d=" << hex << *const_cast<Response &>(m).data<unsigned>() << dec;
            return db;
        }

    protected:
        Unit _unit;
        Error _error;
        Time_Offset _expiry;
        Data _data;
    } __attribute__((packed));

    // Command Message
    class Command: public Header
    {
    private:
        typedef unsigned char Data[MTU - sizeof(Region) - sizeof(Unit)];

    public:
        Command(const Unit & unit, const Region & region)
        : Header(COMMAND, 0, 0, now(), here(), here()), _region(region), _unit(unit) {}

        const Region & region() const { return _region; }
        const Unit & unit() const { return _unit; }

        template<typename T>
        T * command() { return reinterpret_cast<T *>(&_data); }

        template<typename T>
        T * data() { return reinterpret_cast<T *>(&_data); }

        friend Debug & operator<<(Debug & db, const Command & m) {
            db << reinterpret_cast<const Header &>(m) << ",u=" << m._unit << ",reg=" << m._region;
            return db;
        }

    protected:
        Region _region;
        Unit _unit;
        Data _data;
    } __attribute__((packed));

    // Control Message
    class Control: public Header
    {
    private:
        typedef unsigned char Data[MTU - sizeof(Region) - sizeof(Unit)];

    public:
        Control(const Unit & unit, const Region & region)
        : Header(CONTROL, 0, 0, now(), here(), here()), _region(region), _unit(unit) {}

        const Region & region() const { return _region; }
        const Unit & unit() const { return _unit; }

        template<typename T>
        T * command() { return reinterpret_cast<T *>(&_data); }

        template<typename T>
        T * data() { return reinterpret_cast<T *>(&_data); }

        friend Debug & operator<<(Debug & db, const Control & m) {
            db << reinterpret_cast<const Header &>(m) << ",u=" << m._unit << ",reg=" << m._region;
            return db;
        }

    protected:
        Region _region;
        Unit _unit;
        Data _data;
    } __attribute__((packed));


    // TSTP Smart Data bindings
    // Interested (binder between Interest messages and Smart Data)
    class Interested: public Interest
    {
    public:
        template<typename T>
        Interested(T * data, const Region & region, const Unit & unit, const Mode & mode, const Precision & precision, const Microsecond & expiry, const Microsecond & period = 0)
        : Interest(region, unit, mode, precision, expiry, period), _link(this, T::UNIT) {
            db<TSTP>(TRC) << "TSTP::Interested(d=" << data << ",r=" << region << ",p=" << period << ") => " << reinterpret_cast<const Interest &>(*this) << endl;
            _interested.insert(&_link);
            advertise();
        }
        ~Interested() {
            db<TSTP>(TRC) << "TSTP::~Interested(this=" << this << ")" << endl;
            _interested.remove(&_link);
            revoke();
        }

        void advertise() { send(); }
        void revoke() { _mode = DELETE; send(); }

    private:
        void send() {
            db<TSTP>(TRC) << "TSTP::Interested::send() => " << reinterpret_cast<const Interest &>(*this) << endl;
            Buffer * buf = alloc(sizeof(Interest));
            memcpy(buf->frame()->data<Interest>(), this, sizeof(Interest));
            TSTP::marshal(buf);
            _nic->send(buf);
        }

    private:
        Interests::Element _link;
    };

    // Responsive (binder between Smart Data (Sensors) and Response messages)
    class Responsive: public Response
    {
    public:
        template<typename T>
        Responsive(T * data, const Unit & unit, const Error & error, const Time & expiry)
        : Response(unit, error, expiry), _size(sizeof(Response) + sizeof(typename T::Value)), _link(this, T::UNIT) {
            db<TSTP>(TRC) << "TSTP::Responsive(d=" << data << ",s=" << _size << ") => " << this << endl;
            db<TSTP>(INF) << "TSTP::Responsive() => " << reinterpret_cast<const Response &>(*this) << endl;
            _responsives.insert(&_link);
        }
        ~Responsive() {
            db<TSTP>(TRC) << "TSTP::~Responsive(this=" << this << ")" << endl;
            _responsives.remove(&_link);
        }


        using Header::time;
        using Header::origin;

        void respond(const Time & expiry) { send(expiry); }

    private:
        void send(const Time & expiry) {
            assert(expiry > now());
            db<TSTP>(TRC) << "TSTP::Responsive::send(x=" << expiry << ")" << endl;
            _expiry = expiry - now();
            Buffer * buf = alloc(_size);
            memcpy(buf->frame()->data<Response>(), this, _size);
            TSTP::marshal(buf);
            db<TSTP>(INF) << "TSTP::Responsive::send:response=" << this << " => " << reinterpret_cast<const Response &>(*this) << endl;
            _nic->send(buf);
        }

    private:
        unsigned int _size;
        Responsives::Element _link;
    };


    // TSTP Locator
    class Locator: private NIC::Observer
    {
    public:
        Locator() {
            db<TSTP>(TRC) << "TSTP::Locator()" << endl;
            TSTP::_nic->attach(this, NIC::TSTP);
        }
        ~Locator();

        static Coordinates here();// { return Coordinates(50,50,50); } // TODO

        static void bootstrap();

        static void marshal(Buffer * buf);

        void update(NIC::Observed * obs, NIC::Protocol prot, NIC::Buffer * buf);
    };


    // TSTP Timekeeper
    class Timekeeper: private NIC::Observer
    {
    public:
        Timekeeper() {
            db<TSTP>(TRC) << "TSTP::Timekeeper()" << endl;
            TSTP::_nic->attach(this, NIC::TSTP);
        }
        ~Timekeeper();

        static Time now() { return 0; }// { return NIC::Timer::read() * 1000000ll / NIC::Timer::frequency(); };

        static void bootstrap();

        static void marshal(Buffer * buf);

        void update(NIC::Observed * obs, NIC::Protocol prot, NIC::Buffer * buf);
    };


    // TSTP Router
    class Router: private NIC::Observer
    {
    private:
        static const unsigned int CCA_TX_GAP = IEEE802_15_4::CCA_TX_GAP;
        static const unsigned int RADIO_RANGE = 1700;
        static const unsigned int PERIOD = 250000;

    public:
        Router() {
            db<TSTP>(TRC) << "TSTP::Router()" << endl;
            TSTP::_nic->attach(this, NIC::TSTP);
        }
        ~Router();

        static void bootstrap();

        static void marshal(Buffer * buf);

        void update(NIC::Observed * obs, NIC::Protocol prot, NIC::Buffer * buf);

    private:
        static void offset(Buffer * buf) {
            //long long dist = abs(buf->my_distance - (buf->sender_distance - RADIO_RANGE));
            //long long betha = (G * RADIO_RADIUS * 1000000) / (dist * G);
            buf->offset = abs(buf->my_distance - (buf->sender_distance - RADIO_RANGE));
        }

    };


    // TSTP Security
    class Security: private NIC::Observer
    {
    public:
        Security() {
            db<TSTP>(TRC) << "TSTP::Security()" << endl;
            TSTP::_nic->attach(this, NIC::TSTP);
        }
        ~Security();

        static void bootstrap();

        static void marshal(Buffer * buf);

        void update(NIC::Observed * obs, NIC::Protocol prot, NIC::Buffer * buf);
    };


protected:
    TSTP();

public:
    ~TSTP();

    static Coordinates here() { return Locator::here(); }
    static Coordinates sink() { return Coordinates(0, 0, 0); }
    static Time now() { return Timekeeper::now(); }

    static void attach(Observer * obs, void * subject) { _observed.attach(obs, int(subject)); }
    static void detach(Observer * obs, void * subject) { _observed.detach(obs, int(subject)); }
    static bool notify(void * subject, Buffer * buf) { return _observed.notify(int(subject), buf); }

    static void init(unsigned int unit);

private:
    static Region destination(Buffer * buf) {
        switch(buf->frame()->data<Frame>()->type()) {
            case INTEREST:
                return buf->frame()->data<Interest>()->region();
            default:
            case RESPONSE:
                return Region(sink(), 0, buf->frame()->data<Response>()->time(), buf->frame()->data<Response>()->expiry());
            case COMMAND:
                return buf->frame()->data<Command>()->region();
            case CONTROL:
                return buf->frame()->data<Control>()->region();
        }
    }

    static void marshal(Buffer * buf) {
        Locator::marshal(buf);
        Timekeeper::marshal(buf);
        Router::marshal(buf);
        Security::marshal(buf);
    }

    static Buffer * alloc(unsigned int size) {
        assert((!Traits<TSTP>::enabled || EQUAL<NIC::Buffer, Buffer>::Result));
        return reinterpret_cast<Buffer*>(_nic->alloc(NIC::Address::BROADCAST, NIC::TSTP, 0, 0, size));
    }

    static Coordinates absolute(const Coordinates & coordinates) { return coordinates; }
    void update(NIC::Observed * obs, NIC::Protocol prot, NIC::Buffer * buf);

private:
    static NIC * _nic;
    static Interests _interested;
    static Responsives _responsives;
    static Observed _observed; // Channel protocols are singletons
 };

__END_SYS

#endif
