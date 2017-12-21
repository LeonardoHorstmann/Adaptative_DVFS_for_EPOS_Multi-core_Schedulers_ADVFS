// EPOS (Litte-endian) Big Numbers Utility Implementation

#include <utility/bignum.h>

__BEGIN_UTIL

// Class attributes
template<>
const Bignum<16>::_Word Bignum<16>::_mod = {{0xff, 0xff, 0xff, 0xff,
                                             0xff, 0xff, 0xff, 0xff,
                                             0xff, 0xff, 0xff, 0xff,
                                             0xfd, 0xff, 0xff, 0xff }};

template<>
const Bignum<16>::_Barrett Bignum<16>::_barrett_u = {{ 17, 0, 0, 0,
                                                        8, 0, 0, 0,
                                                        4, 0, 0, 0,
                                                        2, 0, 0, 0,
                                                        1, 0, 0, 0}};

__END_UTIL
