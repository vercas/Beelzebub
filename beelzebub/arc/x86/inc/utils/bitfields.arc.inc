//  This file is for inclusion as text.

/*  Bit properties!!!1!  */

#ifdef __GCC_ASM_FLAG_OUTPUTS__
    #ifndef BITPROPRO
    //  Creates a getter for bit-based properties.
    #define BITPROPRO(name, value, decorB, decorGetter)                 \
    decorB inline int MCATS2(Get, name)() decorGetter                   \
    {                                                                   \
        int res;                                                        \
                                                                        \
        asm ( "bt %[bit], %[src] \n\t"                                  \
            : "=@ccc"(res) /* carry flag  */                            \
            : [src]"rm"(value)                                          \
            , [bit]"rN"(MCATS2(name, BitIndex))                         \
            : "cc" );                                                   \
                                                                        \
        return res;                                                     \
    }
    #endif

    #ifndef BITPROPRW
    //  Creates a getter and setter for bit-based properties.
    #define BITPROPRW(name, value, decorB, decorGetter)                 \
    decorB inline int MCATS(Get, name)() decorGetter                    \
    {                                                                   \
        int res;                                                        \
                                                                        \
        asm ( "bt %[bit], %[src] \n\t"                                  \
            : "=@ccc"(res) /* carry flag  */                            \
            : [src]"rm"(value)                                          \
            , [bit]"rN"(MCATS2(name, BitIndex))                         \
            : "cc" );                                                   \
                                                                        \
        return res;                                                     \
    }                                                                   \
    decorB inline int MCATS(Set, name)(bool const val)                  \
    {                                                                   \
        if (val)                                                        \
            asm ( "bts %[bit], %[dst] \n\t"                             \
                : [dst]"+rm"(value)                                     \
                : [bit]"rN"(MCATS2(name, BitIndex))                     \
                : "cc" );                                               \
        else                                                            \
            asm ( "btr %[bit], %[dst] \n\t"                             \
                : [dst]"+rm"(value)                                     \
                : [bit]"rN"(MCATS2(name, BitIndex))                     \
                : "cc" );                                               \
                                                                        \
        return val;                                                     \
    }                                                                   \
    decorB inline int MCATS(FetchSet, name)()                           \
    {                                                                   \
        int res;                                                        \
                                                                        \
        asm ( "bts %[bit], %[dst] \n\t"                                 \
            : [dst]"+rm"(value), "=@ccc"(res)                           \
            : [bit]"rN"(MCATS2(name, BitIndex))                         \
            : "cc" );                                                   \
                                                                        \
        return res;                                                     \
    }                                                                   \
    decorB inline int MCATS(FetchClear, name)()                         \
    {                                                                   \
        int res;                                                        \
                                                                        \
        asm ( "btr %[bit], %[dst] \n\t"                                 \
            : [dst]"+rm"(value), "=@ccc"(res)                           \
            : [bit]"rN"(MCATS2(name, BitIndex))                         \
            : "cc" );                                                   \
                                                                        \
        return res;                                                     \
    }                                                                   \
    decorB inline int MCATS(FetchFlip, name)()                          \
    {                                                                   \
        int res;                                                        \
                                                                        \
        asm ( "btc %[bit], %[dst] \n\t"                                 \
            : [dst]"+rm"(value), "=@ccc"(res)                           \
            : [bit]"rN"(MCATS2(name, BitIndex))                         \
            : "cc" );                                                   \
                                                                        \
        return res;                                                     \
    }
    #endif
#else
    #ifndef BITPROPRO
    //  Creates a getter for bit-based properties.
    #define BITPROPRO(name, value, decorB, decorGetter)                 \
    decorB inline bool MCATS2(Get, name)() decorGetter                  \
    {                                                                   \
        bool res;                                                       \
                                                                        \
        asm("bt    %[bit], %[src] \n\t"                                 \
            "setcb %[res]         \n\t"                                 \
            : [res]"=qm"(res) /* carry flag  */                         \
            : [src]"rm"(value)                                          \
            , [bit]"rN"(MCATS2(name, BitIndex))                         \
            : "cc" );                                                   \
                                                                        \
        return res;                                                     \
    }
    #endif

    #ifndef BITPROPRW
    //  Creates a getter and setter for bit-based properties.
    #define BITPROPRW(name, value, decorB, decorGetter)                 \
    decorB inline bool MCATS(Get, name)() decorGetter                   \
    {                                                                   \
        bool res;                                                       \
                                                                        \
        asm("bt    %[bit], %[src] \n\t"                                 \
            "setcb %[res]         \n\t"                                 \
            : [res]"=qm"(res)                                           \
            : [src]"rm"(value)                                          \
            , [bit]"rN"(MCATS2(name, BitIndex))                         \
            : "cc" );                                                   \
                                                                        \
        return res;                                                     \
    }                                                                   \
    decorB inline bool MCATS(Set, name)(bool const val)                 \
    {                                                                   \
        if (val)                                                        \
            asm("bts   %[bit], %[dst] \n\t"                             \
                : [dst]"+rm"(value)                                     \
                : [bit]"rN"(MCATS2(name, BitIndex))                     \
                : "cc" );                                               \
        else                                                            \
            asm("btr   %[bit], %[dst] \n\t"                             \
                : [dst]"+rm"(value)                                     \
                : [bit]"rN"(MCATS2(name, BitIndex))                     \
                : "cc" );                                               \
                                                                        \
        return val;                                                     \
    }                                                                   \
    decorB inline bool MCATS(FetchSet, name)()                          \
    {                                                                   \
        bool res;                                                       \
                                                                        \
        asm("bts   %[bit], %[dst] \n\t"                                 \
            "setcb %[res]         \n\t"                                 \
            : [dst]"+rm"(value)                                         \
            , [res]"=qm"(res)                                           \
            : [bit]"rN"(MCATS2(name, BitIndex))                         \
            : "cc" );                                                   \
                                                                        \
        return res;                                                     \
    }                                                                   \
    decorB inline bool MCATS(FetchClear, name)()                        \
    {                                                                   \
        bool res;                                                       \
                                                                        \
        asm("btr   %[bit], %[dst] \n\t"                                 \
            "setcb %[res]         \n\t"                                 \
            : [dst]"+rm"(value)                                         \
            , [res]"=qm"(res)                                           \
            : [bit]"rN"(MCATS2(name, BitIndex))                         \
            : "cc" );                                                   \
                                                                        \
        return res;                                                     \
    }                                                                   \
    decorB inline bool MCATS(FetchFlip, name)()                         \
    {                                                                   \
        bool res;                                                       \
                                                                        \
        asm("btc   %[bit], %[dst] \n\t"                                 \
            "setcb %[res]         \n\t"                                 \
            : [dst]"+rm"(value)                                         \
            , [res]"=qm"(res)                                           \
            : [bit]"rN"(MCATS2(name, BitIndex))                         \
            : "cc" );                                                   \
                                                                        \
        return res;                                                     \
    }
    #endif
#endif
