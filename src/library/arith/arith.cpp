/*
Copyright (c) 2013 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#include "library/arith/arith.h"

namespace lean {
void import_arith(environment const & env, io_state const & ios) {
    import_nat(env, ios);
    import_int(env, ios);
    import_real(env, ios);
}
}
