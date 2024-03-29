#ifndef ERROR_HANDLER_MA_TYPES_H
#define ERROR_HANDLER_MA_TYPES_H

/*
 *
 * ma_types.h has all typedefs and enums needed in the
 * message analyzer package
 *
 */

// includes from ups
#include <messagefacility/MessageLogger/MessageLogger.h>
#include "mfextensions/Receivers/qt_mf_msg.hh"  // sev_code_t

#include <boost/multi_array.hpp>

// system includes
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <TRACE/trace.h>

namespace novadaq {
namespace errorhandler {

// forward declaration
class ma_condition;
class ma_rule;

// typdefs used in errorhandler
typedef std::list<qt_mf_msg> msgs_t;
typedef std::shared_ptr<msgs_t> msgs_sp_t;

typedef boost::multi_array_types::index_range range;
typedef std::map<std::string, size_t> idx_t;

// domain of a condition
// <source, target>, -1 means all, -2 means null

const int D_ANY = -1;
const int D_NIL = -2;

typedef std::pair<int, int> ma_cond_range;
typedef std::pair<int, int> ma_cond_domain;
typedef std::list<ma_cond_domain> ma_cond_domains;

// one set of domain for all conditions
typedef std::vector<ma_cond_domain> ma_domain;

// alternative domain sets
typedef std::list<ma_domain> ma_domains;
typedef std::vector<ma_domain> ma_domain_vec;

// enums
enum rule_type_t
{
	RULE_SIMPLE,
	RULE_COMPLEX
};
enum match_type_t
{
	MATCH_ANY,
	MATCH_REGEX,
	MATCH_CONTAINS
};
enum action_type_t
{
	ACTION_ALERT,
	ACTION_POP
};
enum node_type_t
{
	Framework,
	UserCode,
	External
};

// type for elemental domain/boolean cond
enum cond_type_t
{
	COND,
	EXPR,
	FUNCTION,
	FUNCTION_BOOL,
	FUNCTION_INT,
	FUNCTION_DOUBLE,
	FUNCTION_STRING
};

// first element is the pointer to the condition stored in the
// ma_rule class. Second element indicating it looks for a source
// string or a target string
enum arg_t
{
	NONE,
	SOURCE,
	TARGET,
	MESSAGE,
	GROUP1,
	GROUP2,
	GROUP3,
	GROUP4,
	GROUP5,
	GROUP6,
	GROUP7,
	GROUP8,
	GROUP9
};

typedef std::pair<ma_condition *, size_t> cond_idx_t;
typedef std::pair<cond_idx_t, arg_t> cond_arg_t;
typedef std::list<cond_arg_t> cond_arg_list_t;

// compare operators, <, <=, =, >=, >
enum compare_op_t
{
	CO_L,
	CO_LE,
	CO_E,
	CO_NE,
	CO_GE,
	CO_G
};

// notification list
enum notify_t
{
	STATUS_NOTIFY,
	SOURCE_NOTIFY,
	TARGET_NOTIFY
};
typedef std::list<ma_rule *> notify_list_t;
typedef std::list<ma_condition *> conds_t;

// reaction starter: conditions stored in the list will need to notify
// rules for changes in status, domain source, or domain target
typedef std::list<ma_condition *> reaction_starters_t;

// bit pattern indicating if theres
// 1. status change
// 2. source list change
// 3. target list change
const unsigned int STATUS_CHANGE = 0x01;
const unsigned int SOURCE_CHANGE = 0x02;
const unsigned int TARGET_CHANGE = 0x04;

// alarm callback funtion type
typedef std::function<void(std::string const &, std::string const &)> alarm_fn_t;

// condition match callback function type
typedef std::function<void(std::string const &)> cond_match_fn_t;

// alert message type
enum message_type_t
{
	MSG_SYSTEM,
	MSG_ERROR,
	MSG_WARNING,
	MSG_INFO,
	MSG_DEBUG
};

}  // end of namespace errorhandler
}  // end of namespace novadaq

#endif
