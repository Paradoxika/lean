assert(nVal(10):data() == mpz(10))
assert(not pcall(function() nVal(-10) end))
assert(iVal(10):data() == mpz(10))
assert(iVal(-10):data() == mpz(-10))
assert(rVal(10):data() == mpq(10))
assert(rVal(10):data() ~= mpz(10))
assert(rVal("10/3"):data() == mpq(10)/3)
assert(rVal("-10/3"):data() == mpq(-10)/3)
assert(Const("a"):data() == name("a"))
