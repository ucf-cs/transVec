#ifndef HELPSCHEME_H
#define HELPSCHEME_H

#include <stack>
#include "transaction.hpp"

template <class T>
// TODO: Implement this in some clean way.
class HelpScheme
{
	thread_local std::stack<Operation> helpstack;
};

#endif