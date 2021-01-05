#ifndef ERROR_HANDLER_MA_FREQUENCY_H
#define ERROR_HANDLER_MA_FREQUENCY_H

#include "ErrorHandler/MessageAnalyzer/ma_utils.h"

#include <boost/multi_array.hpp>
#include <boost/regex.hpp>

namespace novadaq {
namespace errorhandler {

class ma_cell
{
public:
	ma_cell();
	~ma_cell();

	// reset to ground state
	void
	reset();

	// call hit method when a message passes filtering and match tests
	// returns true if the status has changed (off->on or on->off), or
	// false if not
	//
	// if persistent (persistent=true), the status never turns off.
	// otherwise, the status can change to off when it slides out of
	// the time window
	bool
	hit(qt_mf_msg const& msg, boost::smatch const& w, ma_condition& cond, size_t s_idx, size_t t_idx);

	bool event(time_t t, ma_condition& cond);

	// get status
	bool
	is_on() const { return on; }

	// get number of messages
	size_t
	get_message_count() const { return msgs.size(); }

	// get messages
	const msgs_t&
	get_messages() const { return msgs; }

	// get latest message
	std::string
	get_latest_message() const
	{
		assert(!msgs.empty());
		return msgs.back().text(false).toStdString();
	}

	// get group
	std::string
	get_message_group(size_t i) const
	{
		if (i > what_.size()) throw std::runtime_error("group does not exist");
		return std::string(what_[i].first, what_[i].second);
	}

private:
	msgs_t msgs;
	bool on;

	// groups from last hit
	boost::smatch what_;

	// time of next event
	time_t t_event;
};

typedef boost::multi_array<ma_cell, 2> hitmap_t;
typedef hitmap_t::index index_t;

typedef hitmap_t::const_array_view<2>::type hitmap_view_t;
typedef hitmap_view_t ma_cond_domain_view;
typedef ma_cond_domain_view::const_iterator ma_cond_domain_view_iter;
typedef std::vector<ma_cond_domain_view_iter> ma_cond_domain_view_iters;
typedef std::vector<ma_cond_domain_view> ma_domain_view;
typedef ma_domain_view::const_iterator ma_domain_view_iter;
typedef std::list<ma_domain_view> ma_domain_views;

typedef boost::multi_array_types::index_range range;

}  // end of namespace errorhandler
}  // end of namespace novadaq

#endif
