// EPOS Advanced Encryption Standard (AES) Utility Declarations

#ifndef __diffie_hellman_h
#define __diffie_hellman_h

#include <system/config.h>
#include <utility/bignum.h>

__BEGIN_SYS

template <unsigned int SECRET_SIZE>
class Diffie_Hellman
{
private:
    static const unsigned int PUBLIC_KEY_SIZE = 2 * SECRET_SIZE;

    typedef _UTIL::Bignum<SECRET_SIZE> Bignum;

    class ECC_Point
    {
    public:
        ECC_Point() __attribute__((noinline)) {}

        void operator*=(const Bignum & b) __attribute__((noinline)) {
            // Finding last '1' bit of k
            unsigned int t = Bignum::BITS_PER_DIGIT;
            int b_len = sizeof(Bignum::Word) + 1;
            typename Bignum::Digit now; //= x._data[Bignum::word - 1];   
            do {
                now = b.data[(--b_len) - 1];
            } while(now == 0);
            assert(b_len > 0);

            bool bin[t]; // Binary representation of now

            ECC_Point pp(*this);

            for(int j = Bignum::BITS_PER_DIGIT - 1; j >= 0; j--) {
                if(now % 2)
                    t = j + 1;
                bin[j] = now % 2;
                now /= 2;
            }

            for(int i = b_len - 1; i >= 0; i--) {
                for(; t < Bignum::BITS_PER_DIGIT; t++) {
                    jacobian_double();
                    if(bin[t])
                        add_jacobian_affine(pp);
                }
                if(i>0) {
                    now = b.data[i-1];
                    for(int j = Bignum::BITS_PER_DIGIT - 1; j >= 0; j--) {
                        bin[j] = now % 2;
                        now /= 2;
                    }
                    t=0;
                }
            }

            Bignum Z; 
            z.invert();
            Z = z; 
            Z *= z;

            x *= Z;
            Z *= z;

            y *= Z;
            z = 1;
        }

        friend Debug &operator<<(Debug & db, const ECC_Point & a) {
            db << "{x=" << a.x << ",y=" << a.y << ",z=" << a.z << "}";
            return db;
        }

    private:
        void jacobian_double() __attribute__((noinline)) {
            Bignum B, C(x), aux(z);

            aux *= z; C -= aux;
            aux += x; C *= aux;
            C *= 3;

            z *= y; z *= 2;

            y *= y; B = y;

            y *= x; y *= 4;

            B *= B; B *= 8;

            x = C; x *= x;
            aux = y; aux *= 2;
            x -= aux;

            y -= x; y *= C;
            y -= B; 
        }

        void add_jacobian_affine(const ECC_Point & b) __attribute__((noinline)) {
            Bignum A(z), B, C, X, Y, aux, aux2;  

            A *= z;

            B = A;

            A *= b.x;

            B *= z; B *= b.y;

            C = A; C -= x;

            B -= y;

            X = B; X *= B;
            aux = C; aux *= C;

            Y = aux;

            aux2 = aux; aux *= C;
            aux2 *= 2; aux2 *= x;
            aux += aux2; X -= aux;

            aux = Y; Y *= x;
            Y -= X; Y *= B;
            aux *= y; aux *= C;
            Y -= aux;

            z *= C;

            x = X; y = Y;  
        }

    private:
        Diffie_Hellman::Bignum x, y, z;
    };

public:
    typedef ECC_Point Public_Key;
    typedef Bignum Shared_Key;
    typedef unsigned char Base_Point_Data[sizeof(typename Bignum::Word) * sizeof(typename Bignum::Digit)];

public:
    Diffie_Hellman(const Base_Point_Data & x = _def_x, const Base_Point_Data & y = _def_y) __attribute__((noinline)) {
        _base_point.x = Bignum(x);
        _base_point.y = Bignum(y);
        _base_point.z = 1;
        generate_keypair();
    }

    const ECC_Point & public_key() { return _public; }

    void generate_keypair() {
        db<Diffie_Hellman>(TRC) << "Diffie_Hellman::generate_keypair()" << endl;
        _private.random();
        db<Diffie_Hellman>(INF) << "Diffie_Hellman: private=" << _private << endl;
        _public = _base_point;
        db<Diffie_Hellman>(INF) << "Diffie_Hellman: base point=" << _base_point << endl;
        _public *= _private;
        db<Diffie_Hellman>(INF) << "Diffie_Hellman: public=" << _public << endl;
    }

    Shared_Key shared_key(const ECC_Point & public_key) __attribute__((noinline)) {
        db<Diffie_Hellman>(TRC) << "Diffie_Hellman::shared_key(pub=" << public_key << ")" << endl;
        db<Diffie_Hellman>(INF) << "Diffie_Hellman: private=" << _private << endl;

        public_key *= _private;
        public_key.x ^= public_key.y;

        db<Diffie_Hellman>(INF) << "Diffie_Hellman: public=" << _public << endl;
        return public_key.x;
    }

private:
    Bignum _private;
    ECC_Point _base_point;
    ECC_Point _public;

    static const unsigned char _def_x[SECRET_SIZE];
    static const unsigned char _def_y[SECRET_SIZE];
};

template<>
const unsigned char Diffie_Hellman<16>::_def_x[16] = { 0x86, 0x5b, 0x2c, 0xa5,
                                                       0x7c, 0x60, 0x28, 0x0c,
                                                       0x2d, 0x9b, 0x89, 0x8b,
                                                       0x52, 0xf7, 0x1f, 0x16 };

template<>
const unsigned char Diffie_Hellman<16>::_def_y[16] = { 0x83, 0x7a, 0xed, 0xdd,
                                                       0x92, 0xa2, 0x2d, 0xc0,
                                                       0x13, 0xeb, 0xaf, 0x5b,
                                                       0x39, 0xc8, 0x5a, 0xcf };

__END_SYS

#endif
