#ifndef ALGO_LAMBDA_H
#define ALGO_LAMBDA_H

#include <algo/bits/common.h>
#include <algo/bits/impl_lambda.h>

namespace algo {

ALGO_OP_BINARY( add,     + )
ALGO_MK_BINARY( add,     + )
ALGO_OP_BINARY( sub,     - )
ALGO_MK_BINARY( sub,     - )
ALGO_OP_BINARY( mult,    * )
ALGO_MK_BINARY( mult,    * )
ALGO_OP_BINARY( div,     / )
ALGO_MK_BINARY( div,     / )
ALGO_OP_BINARY( mod,     % )
ALGO_MK_BINARY( mod,     % )
ALGO_OP_BINARY( lshift, << )
ALGO_MK_BINARY( lshift, << )
ALGO_OP_BINARY( rshift, >> )
ALGO_MK_BINARY( rshift, >> )
ALGO_OP_BINARY( and,     & )
ALGO_MK_BINARY( and,     & )
ALGO_OP_BINARY( or,      | )
ALGO_MK_BINARY( or,      | )
ALGO_OP_BINARY( xor,     ^ )
ALGO_MK_BINARY( xor,     ^ )

ALGO_OP_BINARY( assign,  = )
ALGO_OP_BINARY( aadd,   += )
ALGO_MK_BINARY( aadd,   += )
ALGO_OP_BINARY( asub,   -= )
ALGO_MK_BINARY( asub,   -= )
ALGO_OP_BINARY( amult,  *= )
ALGO_MK_BINARY( amult,  *= )
ALGO_OP_BINARY( adiv,   /= )
ALGO_MK_BINARY( adiv,   /= )
ALGO_OP_BINARY( amod,   %= )
ALGO_MK_BINARY( amod,   %= )

ALGO_OP_BINARY( band,   && )
ALGO_MK_BINARY( band,   && )
ALGO_OP_BINARY( bor,    || )
ALGO_MK_BINARY( bor,    || )
ALGO_OP_BINARY( beq,    == )
ALGO_MK_BINARY( beq,    == )
ALGO_OP_BINARY( bneq,   != )
ALGO_MK_BINARY( bneq,   != )
ALGO_OP_BINARY( bless,  <  )
ALGO_MK_BINARY( bless,  <  )
ALGO_OP_BINARY( bleq,   <= )
ALGO_MK_BINARY( bleq,   <= )
ALGO_OP_BINARY( bgrt,   >  )
ALGO_MK_BINARY( bgrt,   >  )
ALGO_OP_BINARY( bgeq,   >= )
ALGO_MK_BINARY( bgeq,   >= )

ALGO_OP_UNARY( not,  ! )
ALGO_OP_UNARY( inv,  ~ )
ALGO_OP_UNARY( addr, & )
ALGO_OP_UNARY( pntr, * )

template <typename Type, typename Ftor>
inline _wrapper<op_cast<Ftor, Type> >
cast(_wrapper<Ftor> w1) {
  return make_wrapper(op_cast<Ftor, Type>(w1.m_ftor));
}

}

#endif
