
#include "ErrorHandler/MessageAnalyzer/ma_cell.h"
#include "ErrorHandler/MessageAnalyzer/ma_condition.h"

#include <time.h>

using namespace novadaq::errorhandler;

ma_cell::ma_cell()
    : msgs()
    , on(false)
    , what_()
    , t_event(0)
{
}

ma_cell::~ma_cell()
{
}

bool ma_cell::hit(qt_mf_msg const& msg, boost::smatch const& w, ma_condition& cond, size_t s_idx, size_t t_idx)
{
	// regex groups
	what_ = w;

	// push new message
	time_t latest = msg.time().tv_sec;
	msgs.push_back(msg);

	if (on && cond.persistent())
	{
		msgs.pop_front();
		return false;
	}

	// pop expired messages ( >timespan )
	while (latest - msgs.front().time().tv_sec > cond.timespan())
		msgs.pop_front();

	// pop excessive messages ( >count )
	while (msgs.size() > (size_t)cond.trigger_count())
		msgs.pop_front();

	// determin the new state (on or off)
	bool new_state = false;

	if (msgs.size() == (size_t)cond.trigger_count())
	{
		new_state = cond.at_least() ? true : false;

		// lock
		boost::mutex::scoped_lock lock(cond.timing_events().lock);

		// schedule event
		// t0 = events.front();
		// schedule(t0 + ts + 1);

		time_t t0 = msgs.front().time().tv_sec;
		t_event = t0 + cond.timespan() + 1;
		cond.timing_events().event_queue().push(ma_timing_event(t_event, cond, s_idx, t_idx));
	}
	else if (cond.at_most())
	{
		// lock
		boost::mutex::scoped_lock lock(cond.timing_events().lock);

		// not reached the critical size
		// for occur_at_least, do nothing.
		// for occur_at_most, schedule an event to turn on the cell
		// t0 = events.front();
		// schedule(t0 + ts + 1);

		time_t t0 = msgs.front().time().tv_sec;
		t_event = t0 + cond.timespan() + 1;
		cond.timing_events().event_queue().push(ma_timing_event(t_event, cond, s_idx, t_idx));
	}

	// no change in status
	if (new_state == on)
		return false;

	// changed
	on = new_state;
	return true;
}

bool ma_cell::event(time_t t, ma_condition& cond)
{
	// not reached the event time, no flip
	if (t != t_event)
		return false;

	bool new_status = cond.at_most() ? true : (cond.persistent() ? true : false);

	// not flipped
	if (new_status == on)
		return false;

	// flipped
	on = new_status;
	return true;
}

void ma_cell::reset()
{
	on = false;
	t_event = 0;
	msgs.clear();
}
