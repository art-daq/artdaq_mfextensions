
#include <ErrorHandler/ma_rule.h>
#include <ErrorHandler/ma_parse.h>

#include <iostream>

using fhicl::ParameterSet;
using namespace novadaq::errorhandler;

ma_rule::ma_rule( string_t const & rule_name
                , string_t const & rule_desc
                , bool repeat
                , int  holdoff_time )
: conditions( )
, conditions_idx( )
, primitive_cond( )
, cond_map( NULL )
, name_( rule_name )
, description_( rule_desc )
, condition_expr( )
, alarm_count( 0 )
, cond_names_( )
, alarm_msg( )
, boolean_expr( )
, domain_expr( ) 
, domains( )
, alarms( )
, itor_last_alarm( alarms.end() )
, repeat_alarm( repeat )
, holdoff( holdoff_time )
, initialized( false )
, enabled( true )
, actions( )
{

}

void
  ma_rule::parse( string_t const & cond_expr
                , string_t const & alarm_message
                , ParameterSet const & act_pset
                , cond_map_t     * cond_map_ptr )
{
  cond_map = cond_map_ptr;
  condition_expr = cond_expr;

  // condition expression
  if( !parse_condition_expr( cond_expr, this ) )
    throw std::runtime_error("rule parsing failed");

  // alarm message
  alarm_msg.init(this, alarm_message);

  // actions
  std::vector<string_t> keys = act_pset.get_pset_keys();
  for(size_t i=0; i<keys.size(); ++i)
  {
    ParameterSet param = act_pset.get<ParameterSet>(keys[i]);
    actions.push_back( ma_action_factory::create_instance( keys[i], this, param ) );
  }

  // init
  if( domain_expr.empty() )
    domains.push_back(ma_domain_ctor_any(conditions.size()));

  initialized = true;
}


cond_idx_t 
  ma_rule::insert_condition_ptr( string_t const & name, bool primitive )
{ 
  // cond_map must not be empty
  assert (cond_map != NULL);

  LOG_DEBUG("") << "insert_cond_ptr: name = " << name << "  "
                << "primitive = " << primitive;

  // the condition has already been added
  {
    idx_t::const_iterator it = conditions_idx.find(name);
    if( it != conditions_idx.end() )  
    {
      // primitive cond overrides non-primitive cond
      primitive_cond[it->second] = primitive_cond[it->second] | primitive;
      return cond_idx_t(conditions[it->second], it->second);;
    }
  }

  // look for the cond in the rule_engine container
  cond_map_t::iterator it = cond_map->find(name);

  if( it == cond_map->end() )  // name not found
    throw std::runtime_error("insert_cond_ptr: condition " + name + " not found");

  // put this rule to the status notification list of the condition
  it->second.push_notify_status(this);

  // register the condition in the rule
  cond_names_.push_back(name);
  conditions.push_back(&it->second);
  primitive_cond.push_back(primitive);

  assert( conditions.size() == primitive_cond.size() );
  size_t idx = conditions.size()-1;

  conditions_idx.insert(std::make_pair(name, idx));

  return cond_idx_t(&it->second, idx);
}


void ma_rule::evaluate_domain( )
{
  assert( initialized );

  // re-evaluate domains
  domains.clear();
  domain_expr.evaluate(domains);

  LOG_DEBUG("") << description_
                << ": domain evaluated, size = " << domains.size();
}

bool ma_rule::recursive_evaluate ( ma_domain & value  
                                 , ma_domain & alarm
                                 , ma_domain const & domain
                                 , size_t n )
{
  // pre-conditions
  assert( !domain_is_null(domain[n]) );

  // get range
  ma_cond_range src(D_NIL, D_NIL);
  ma_cond_range target(D_NIL, D_NIL);
    
  // a primitive condition (Cn) or non-primitive ( COUNT(Cn.$s|t) )
  // we only loop through possible values for primitive conditions
  if (primitive_cond[n])
    conditions[n]->get_cond_range(domain[n], src, target);

  LOG_DEBUG("") << "depth: " << n << "  "
                << "primitive_cond[n]: " << primitive_cond[n];

  for(int s = src.first; s<=src.second; ++s)
  {
    for(int t = target.first; t<=target.second; ++t)
    {
      value[n].first = s;
      value[n].second = t;

      LOG_DEBUG("") << "depth: " << n << "  "
                    << "src: "   << s << "  "
                    << "tgt: "   << t;

      if( n != domain.size()-1 )
      {
        if( recursive_evaluate(value, alarm, domain, n+1) )
          return true;
      }
      else
      { 
        // evaluate and, if found new alarm, no need to continue
        if( boolean_evaluate(value, alarm, domain) )  
          return true;
      }
    }
  }

  return false;
}

bool ma_rule::evaluate( )
{
  assert( initialized );

  // if disabled, always returns false
  if( !enabled ) return false;

  LOG_DEBUG("") << description_ << ": evaluate boolean expr...";

  // loop through domain alternatives
  for ( ma_domains::const_iterator ait = domains.begin()
      ; ait != domains.end(); ++ait )
  {
    // each domain consists a set of possible values for each condition
    ma_domain const & domain = *ait;

    // pre-condition
    assert( !domain_is_null(domain) );

    // holds the one possible set of value
    ma_domain value = ma_domain_ctor_null(domain.size());
    ma_domain alarm = ma_domain_ctor_null(domain.size());

    // recursively build and evaluate possible values from domain
    if( recursive_evaluate(value, alarm, domain, 0) )
      return true;
  }

  return false;
}

bool ma_rule::boolean_evaluate( ma_domain & value
                              , ma_domain & alarm
                              , ma_domain const & domain )
{
  LOG_DEBUG("") << "now evaluate boolean_expr with given value";

  // evaluate as true with given set of values
  if (boolean_expr.evaluate(value, alarm, domain))
  {
    LOG_DEBUG("") << "alarm (" << alarm[0].first << ", "
                               << alarm[0].second << ")";

    std::map<ma_domain, timeval>::iterator it = alarms.find(alarm);
    if ( it==alarms.end() )
    {
      // this is a new alarm. push the current domain to alarm list
      timeval tv; gettimeofday(&tv, 0);
      itor_last_alarm = alarms.insert(std::make_pair(alarm, tv)).first;

      alarm.clear();
      alarm = ma_domain_ctor_null( domain.size() );

      // trigger new alarm
      // ...

      ++alarm_count;

      return true;
    }

    else if ( repeat_alarm )
    {
      // not a new alarm, but the repeat_alarm flag has been set
      timeval tv; gettimeofday(&tv, 0);
      timeval lt = it->second;
      if( tv.tv_sec - lt.tv_sec > holdoff )
      {
        // passed the holdoff time, can refire a new alarm
        itor_last_alarm = alarms.insert(it, std::make_pair(alarm, tv));

        alarm.clear();
        alarm = ma_domain_ctor_null( domain.size() );

        // trigger new alarm
        // ...

        ++alarm_count;

        return true;
      }
    }

    // otherwise, the alarm has already been triggered, or hasn't passed
    // the holdoff time
    LOG_DEBUG("") << "this alarm has already been triggered";
  }

  // reset alarm
  alarm.clear();
  alarm = ma_domain_ctor_null( domain.size() );

  return false;
}

int ma_rule::act( )
{

  int mask = 0;
  for(ma_actions::const_iterator it = actions.begin()
	, e  = actions.end(); it != e; ++it )
  {
    if((*it)->exec())++mask;
  }
  return mask;
}

void ma_rule::reset( )
{ 
  // clear user function state
  boolean_expr.reset();

  // reset all related conds
  cond_vec_t::iterator it=conditions.begin();
  for( ; it!=conditions.end(); ++it) (*it)->reset();

  //domains.clear(); 
  alarms.clear(); 
  itor_last_alarm = alarms.end();
  alarm_count=0; 
}

ma_domain const & ma_rule::get_alarm() const
{
  if( alarms.empty() )
    throw std::runtime_error("get_alarm_message(): no alarm has been triggerd");

  // make sure that the itor_last_alarm points to something
  assert( itor_last_alarm!=alarms.end() );

  return itor_last_alarm->first;
}

string_t ma_rule::get_alarm_message( )
{
  return alarm_msg.message();
}



// ----------------------------------------------------------------
//
// get condition index and pointer given a name
cond_idx_t
  ma_rule::get_cond_idx( string_t const & name ) const
{
  idx_t::const_iterator it = conditions_idx.find(name);
  if( it == conditions_idx.end() )
    throw std::runtime_error("get_cond_idx: name '" + name + "' not found");
  return cond_idx_t(conditions[it->second], it->second);
}

// get pointer to the condition 
ma_condition *
  ma_rule::get_cond( string_t const & name ) const
{
  idx_t::const_iterator it = conditions_idx.find(name);
  if( it == conditions_idx.end() )
    throw std::runtime_error("get_cond: name '" + name + "' not found");
  return conditions[it->second];
}

// get index to the condition 
size_t
  ma_rule::get_idx( string_t const & name ) const
{
  idx_t::const_iterator it = conditions_idx.find(name);
  if( it == conditions_idx.end() )
    throw std::runtime_error("get_cond: name '" + name + "' not found");
  return it->second;
}

// get the size of condition container
size_t
  ma_rule::get_cond_size() const
{
  assert( conditions.size() == conditions_idx.size() );
  return conditions.size();
}

// update the "notify_on_source" or "notify_on_target" list
// for corresponding conditions
void
  ma_rule::update_notify_list( string_t const & name, arg_t arg )
{
  idx_t::const_iterator it = conditions_idx.find(name);

  if( it == conditions_idx.end() )
    throw std::runtime_error("update_notify_list: name '" + name + "' not found");

  (arg==SOURCE) ? conditions[it->second]->push_notify_source(this)
    : conditions[it->second]->push_notify_target(this);
}











