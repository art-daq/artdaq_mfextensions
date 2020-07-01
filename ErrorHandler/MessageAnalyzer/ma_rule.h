#ifndef ERROR_HANDLER_MA_RULE_H
#define ERROR_HANDLER_MA_RULE_H

// from novadaq
#include "ErrorHandler/MessageAnalyzer/ma_utils.h"
#include "ErrorHandler/MessageAnalyzer/ma_condition.h"
#include "ErrorHandler/MessageAnalyzer/ma_domain_expr.h"
#include "ErrorHandler/MessageAnalyzer/ma_boolean_expr.h"
#include "ErrorHandler/MessageAnalyzer/ma_richmsg.h"
#include "ErrorHandler/MessageAnalyzer/ma_action.h"

// from ups
#include <boost/shared_ptr.hpp>
#include <fhiclcpp/ParameterSet.h>

// sys headers
#include <map>
#include <sys/time.h>

namespace novadaq {
namespace errorhandler {

// MsgAnalyzer Rule
class ma_rule
{
public:

  // c'tor
  ma_rule( std::string const & name
         , std::string const & desc
         , bool repeat
         , int holdoff_time = 0 );

  // ----------------------------------------------------------------
  //

  // public method, call to initialze the rule
  void
    parse( std::string const & cond_expr
         , std::string const & alarm_message
         , fhicl::ParameterSet const & actions
         , cond_map_t * cond_map_ptr );

  // public method, call to evaluate the domain expression
  void
    evaluate_domain( );

  // public method, call to evaluate the boolean expression 
  bool
    evaluate( );

  // carry out actions
  int
    act( );

  // public method, get the alarm
  ma_domain const & 
    get_alarm( ) const;

  // public method, get the alarm message
  std::string
    get_alarm_message( );

  // number of alarms. if the repeatable alarm flag is true, it is the
  // number of total alarms; otherwise it is the number of distinguishable
  // alarms (one domain only alarms once)
  int
    get_alarm_count()              const { return alarm_count; }

  // get fields
  const std::string & name()          const { return name_;}
  const std::string & description()   const { return description_; }
  const std::string & cond_expr()     const { return condition_expr; }
  const std::string & alarm_message() const { return alarm_msg.plain_message(); }

  const std::vector<std::string> & cond_names()   const { return cond_names_; }

  // enable/disable the rule
  void
    enable( bool flag ) { enabled = flag; }

  // reset the rule to its ground state ( reset alarms and domains ) 
  void 
    reset( );

  // ----------------------------------------------------------------
  //

  // called by the parser to set the boolean expression
  void set_boolean_expr( ma_boolean_expr const & expr )
    { boolean_expr = expr; }

  // called by the parser to set the domain expression
  void set_domain_expr( ma_domain_expr const & expr )
    { domain_expr = expr; }

  // called by the parser to push a cond_ptr to the container
  cond_idx_t 
    insert_condition_ptr( std::string const & name, bool primitive );

  // ----------------------------------------------------------------
  //
  // get condition index and pointer given a name
  cond_idx_t 
    get_cond_idx( std::string const & name ) const;

  // get pointer to the condition 
  ma_condition *
    get_cond( std::string const & name ) const;

  // get index to the condition 
  size_t 
    get_idx( std::string const & name ) const;

  // get the size of condition container
  size_t
    get_cond_size() const;

  // update the "notify_on_source" or "notify_on_target" list
  // for corresponding conditions
  void
    update_notify_list( std::string const & name, arg_t arg );

public:

  cond_vec_t         conditions;
  idx_t              conditions_idx;
  std::vector<bool>  primitive_cond;

private:

  // recursive evaluation function
  //   value:  specific value set in the given domain
  //   domain: the input domain where values are allowed
  //   n:      depth of the recursion
  //   return: true if new alarm found
  bool 
    recursive_evaluate ( ma_domain & value  
                       , ma_domain & alarm
                       , ma_domain const & domain 
                       , size_t n );

  // evaluate the boolean expression with a given set of inputs
  //   value:  the input values for each condition
  bool 
    boolean_evaluate ( ma_domain & value
                     , ma_domain & alarm
                     , ma_domain const & domain );

  bool 
    parse_alarm_message ( std::string const & s );

  bool
    parse_alarm_ref( std::string const & s );

private:

  // a pointer to the condition container containing all conditions in the app
  // the original container is hold in the ma_rule_engine class
  cond_map_t    * cond_map;

  std::string        name_;
  std::string        description_;
  std::string        condition_expr;
  int             alarm_count;

  std::vector<std::string>       cond_names_; // vector of strings holding the cond name list

  ma_richmsg      alarm_msg;

  ma_boolean_expr boolean_expr;
  ma_domain_expr  domain_expr;

  ma_domains      domains;

  std::map<ma_domain, timeval>                 alarms;
  std::map<ma_domain, timeval>::const_iterator itor_last_alarm;

  bool            repeat_alarm;
  int             holdoff;

  bool            initialized;
  bool            enabled;

  ma_actions      actions;
};

typedef boost::shared_ptr<ma_rule>   rule_sp;
typedef std::map<std::string, ma_rule>  rule_map_t;

} // end of namespace errorhandler
} // end of namespace novadaq

#endif
