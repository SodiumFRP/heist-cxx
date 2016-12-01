// $Id$

#ifndef _ACTIONS_H_
#define _ACTIONS_H_

#include <functional>

/*!
 * Chain the two effectful functions (actions) together so they get executed in sequence.
 * This doesn't really belong
 */
std::function<void()> chainActions(std::function<void()>, std::function<void()>);

#endif

