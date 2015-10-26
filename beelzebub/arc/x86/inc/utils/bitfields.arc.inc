//  This file is for inclusion as text.

/*  Bit properties!!!1!  */

#ifdef __GCC_ASM_FLAG_OUTPUTS__
    #ifndef BITFIELD_FLAG_RO
    //  Creates a getter for bit-based properties.
    #define BITFIELD_FLAG_RO(bitInd, name, varT, var, decB, decG)              \
    decB inline int MCATS2(Get, name)() decG                                   \
    {                                                                          \
        int res;                                                               \
                                                                               \
        asm ( "bt %[bit], %[src] \n\t"                                         \
            : "=@ccc"(res) /* carry flag  */                                   \
            : [src]"rm"(var)                                                   \
            , [bit]"rN"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }
    #endif

    #ifndef BITFIELD_FLAG_RW
    //  Creates a getter and setter for bit-based properties.
    #define BITFIELD_FLAG_RW(bitInd, name, varT, var, decB, decG)              \
    decB inline int MCATS(Get, name)() decG                                    \
    {                                                                          \
        int res;                                                               \
                                                                               \
        asm ( "bt %[bit], %[src] \n\t"                                         \
            : "=@ccc"(res) /* carry flag  */                                   \
            : [src]"rm"(var)                                                   \
            , [bit]"rN"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }                                                                          \
    decB inline int MCATS(Set, name)(bool const val)                           \
    {                                                                          \
        if (val)                                                               \
            asm ( "bts %[bit], %[dst] \n\t"                                    \
                : [dst]"+rm"(var)                                              \
                : [bit]"rN"(bitInd)                                            \
                : "cc" );                                                      \
        else                                                                   \
            asm ( "btr %[bit], %[dst] \n\t"                                    \
                : [dst]"+rm"(var)                                              \
                : [bit]"rN"(bitInd)                                            \
                : "cc" );                                                      \
                                                                               \
        return val;                                                            \
    }                                                                          \
    decB inline int MCATS(FetchSet, name)()                                    \
    {                                                                          \
        int res;                                                               \
                                                                               \
        asm ( "bts %[bit], %[dst] \n\t"                                        \
            : [dst]"+rm"(var), "=@ccc"(res)                                    \
            : [bit]"rN"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }                                                                          \
    decB inline int MCATS(FetchClear, name)()                                  \
    {                                                                          \
        int res;                                                               \
                                                                               \
        asm ( "btr %[bit], %[dst] \n\t"                                        \
            : [dst]"+rm"(var), "=@ccc"(res)                                    \
            : [bit]"rN"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }                                                                          \
    decB inline int MCATS(FetchFlip, name)()                                   \
    {                                                                          \
        int res;                                                               \
                                                                               \
        asm ( "btc %[bit], %[dst] \n\t"                                        \
            : [dst]"+rm"(var), "=@ccc"(res)                                    \
            : [bit]"rN"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }
    #endif
#else
    #ifndef BITFIELD_FLAG_RO
    //  Creates a getter for bit-based properties.
    #define BITFIELD_FLAG_RO(bitInd, name, varT, var, decB, decG)              \
    decB inline bool MCATS2(Get, name)() decG                                  \
    {                                                                          \
        bool res;                                                              \
                                                                               \
        asm("bt    %[bit], %[src] \n\t"                                        \
            "setcb %[res]         \n\t"                                        \
            : [res]"=qm"(res) /* carry flag  */                                \
            : [src]"rm"(var)                                                   \
            , [bit]"rN"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }
    #endif

    #ifndef BITFIELD_FLAG_RW
    //  Creates a getter and setter for bit-based properties.
    #define BITFIELD_FLAG_RW(bitInd, name, varT, var, decB, decG)              \
    decB inline bool MCATS(Get, name)() decG                                   \
    {                                                                          \
        bool res;                                                              \
                                                                               \
        asm("bt    %[bit], %[src] \n\t"                                        \
            "setcb %[res]         \n\t"                                        \
            : [res]"=qm"(res)                                                  \
            : [src]"rm"(var)                                                   \
            , [bit]"rN"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }                                                                          \
    decB inline bool MCATS(Set, name)(bool const val)                          \
    {                                                                          \
        if (val)                                                               \
            asm("bts   %[bit], %[dst] \n\t"                                    \
                : [dst]"+rm"(var)                                              \
                : [bit]"rN"(bitInd)                                            \
                : "cc" );                                                      \
        else                                                                   \
            asm("btr   %[bit], %[dst] \n\t"                                    \
                : [dst]"+rm"(var)                                              \
                : [bit]"rN"(bitInd)                                            \
                : "cc" );                                                      \
                                                                               \
        return val;                                                            \
    }                                                                          \
    decB inline bool MCATS(FetchSet, name)()                                   \
    {                                                                          \
        bool res;                                                              \
                                                                               \
        asm("bts   %[bit], %[dst] \n\t"                                        \
            "setcb %[res]         \n\t"                                        \
            : [dst]"+rm"(var)                                                  \
            , [res]"=qm"(res)                                                  \
            : [bit]"rN"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }                                                                          \
    decB inline bool MCATS(FetchClear, name)()                                 \
    {                                                                          \
        bool res;                                                              \
                                                                               \
        asm("btr   %[bit], %[dst] \n\t"                                        \
            "setcb %[res]         \n\t"                                        \
            : [dst]"+rm"(var)                                                  \
            , [res]"=qm"(res)                                                  \
            : [bit]"rN"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }                                                                          \
    decB inline bool MCATS(FetchFlip, name)()                                  \
    {                                                                          \
        bool res;                                                              \
                                                                               \
        asm("btc   %[bit], %[dst] \n\t"                                        \
            "setcb %[res]         \n\t"                                        \
            : [dst]"+rm"(var)                                                  \
            , [res]"=qm"(res)                                                  \
            : [bit]"rN"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }
    #endif
#endif
