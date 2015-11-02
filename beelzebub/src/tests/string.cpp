#ifdef __BEELZEBUB__TEST_STR

#include <tests/string.hpp>
#include <debug.hpp>

using namespace Beelzebub;

char const * const tStrA = "";
char const * const tStrB = "a";
char const * const tStrC = "ab";
char const * const tStrD = "abc";
char const * const tStrE = "A";
char const * const tStrF = "AB";
char const * const tStrG = "ABC";
char const * const tStrH = "0000000000000000000000000000";
char const * const tStrI = "ABC";
char const * const tStrJ = "";

size_t const tStrAlen = 0;
size_t const tStrBlen = 1;
size_t const tStrClen = 2;
size_t const tStrDlen = 3;
size_t const tStrElen = 1;
size_t const tStrFlen = 2;
size_t const tStrGlen = 3;
size_t const tStrHlen = 28;
size_t const tStrIlen = 3;
size_t const tStrJlen = 0;

Handle TestStringLibrary()
{
#define testlen(name)                                        \
size_t MCATS(str, name, len) = strlen(MCATS(tStr, name));    \
ASSERT(MCATS(str, name, len) == MCATS(tStr, name, len)       \
    , "Failed string length test for string %s (\"%s\"): "   \
      "got %us, expected %us!"                               \
    , #name, MCATS(tStr, name), MCATS(str, name, len)        \
    , MCATS(tStr, name, len));

#define testNlen(tseq, name, nl, expctd)                                \
size_t MCATS(str, name, Nlen, tseq) = strnlen(MCATS(tStr, name), (nl)); \
ASSERT(MCATS(str, name, Nlen, tseq) == (size_t)(expctd)                 \
    , "Failed string capped length test for string %s (\"%s\"): "       \
      "got %us, expected %us (cap: %us; test sequence %us)!"            \
    , #name, MCATS(tStr, name), MCATS(str, name, Nlen, tseq)            \
    , (size_t)(expctd), (size_t)(nl), (size_t)(tseq));

    testlen(A);
    testlen(B);
    testlen(C);
    testlen(D);
    testlen(E);
    testlen(F);
    testlen(G);
    testlen(H);
    testlen(I);
    testlen(J);

    testNlen(1, A, 0, 0);
    testNlen(1, B, 0, 0);
    testNlen(1, C, 0, 0);
    testNlen(1, D, 0, 0);
    testNlen(1, E, 0, 0);
    testNlen(1, F, 0, 0);
    testNlen(1, G, 0, 0);
    testNlen(1, H, 0, 0);
    testNlen(1, I, 0, 0);
    testNlen(1, J, 0, 0);

    testNlen(2, A, 1, 0);
    testNlen(2, B, 1, 1);
    testNlen(2, C, 1, 1);
    testNlen(2, D, 1, 1);
    testNlen(2, E, 1, 1);
    testNlen(2, F, 1, 1);
    testNlen(2, G, 1, 1);
    testNlen(2, H, 1, 1);
    testNlen(2, I, 1, 1);
    testNlen(2, J, 1, 0);

    testNlen(3, A, 2, 0);
    testNlen(3, B, 2, 1);
    testNlen(3, C, 2, 2);
    testNlen(3, D, 2, 2);
    testNlen(3, E, 2, 1);
    testNlen(3, F, 2, 2);
    testNlen(3, G, 2, 2);
    testNlen(3, H, 2, 2);
    testNlen(3, I, 2, 2);
    testNlen(3, J, 2, 0);

    testNlen(4, A, 3, 0);
    testNlen(4, B, 3, 1);
    testNlen(4, C, 3, 2);
    testNlen(4, D, 3, 3);
    testNlen(4, E, 3, 1);
    testNlen(4, F, 3, 2);
    testNlen(4, G, 3, 3);
    testNlen(4, H, 3, 3);
    testNlen(4, I, 3, 3);
    testNlen(4, J, 3, 0);

    testNlen(5, A, 100, tStrAlen);
    testNlen(5, B, 100, tStrBlen);
    testNlen(5, C, 100, tStrClen);
    testNlen(5, D, 100, tStrDlen);
    testNlen(5, E, 100, tStrElen);
    testNlen(5, F, 100, tStrFlen);
    testNlen(5, G, 100, tStrGlen);
    testNlen(5, H, 100, tStrHlen);
    testNlen(5, I, 100, tStrIlen);
    testNlen(5, J, 100, tStrJlen);

    return HandleResult::Okay;
}

#endif
