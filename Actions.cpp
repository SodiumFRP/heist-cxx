// $Id$

#include "Actions.h"

using namespace std;


// Chain the two functions (actions) together so they get executed in sequence.
std::function<void()> chainActions(std::function<void()> a, std::function<void()> b)
{
    return [=] () {a(); b();};
}

