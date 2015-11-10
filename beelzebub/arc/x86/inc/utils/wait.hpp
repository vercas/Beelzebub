#pragma once

#include <metaprogramming.h>

namespace Beelzebub { namespace Utils
{
    /**
     *  <summary>Waits for the specified time to pass.</summary>
     */
    __bland void Wait(uint64_t const microseconds);

    /**
     *  <summary>
     *  Waits for the specified time to pass or the given predicate to succeed.
     *  </summary>
     *  <return>True if the predicate succeeded; otherwise false.</return>
     */
    __bland bool Wait(uint64_t const microseconds, PredicateFunction0 const pred);
}}
