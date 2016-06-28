// The InteractionTuple structure details what machine the interaction took place on (context),
// the initiator of the interaction (from), and the target of the interaction (to).

#ifndef INTERACTIONTUPLE_H_
#define INTERACTIONTUPLE_H_

#include <string>

struct InteractionTuple
{
	InteractionTuple()
	{}

	InteractionTuple(const std::string& f, const std::string& t, const std::string& c)
		: from(f), to(t), context(c)
	{}

	std::string from;
	std::string to;
	std::string context;
};

#endif // INTERACTIONTUPLE_H_
