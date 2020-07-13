#include "ErrorHandler/MessageAnalyzer/ma_condition.h"
#include "ErrorHandler/MessageAnalyzer/ma_parse.h"

using namespace novadaq::errorhandler;

ma_condition::ma_condition( std::string  const & desc
                          , std::string  const & sev
                          , std::vector<std::string> const & sources
                          , std::vector<std::string> const & categories
                          , std::string  const & regex
                          , std::string  const & test 
                          , bool              persistent_cond
                          , int               occur 
                          , bool              at_least
                          , int               timespan
                          , bool              per_source
                          , bool              per_target
                          , int               target_group
                          , ma_timing_events & events
                          )
: description_  ( desc )
    , severity_(get_sev_from_string(sev))
    , srcs_str      ( )
, e_srcs        ( )
, any_src       ( false )
, cats_str      ( )
, e_cats        ( )
, any_cat       ( false )
, match_type    ( MATCH_REGEX )
, regex_str     ( regex )
, e             ( regex )
, test_expr     ( )
, tc            ( at_least ? occur : occur+1 )
, at_least_     ( at_least )
, ts            ( timespan )
, ps            ( per_source )
, pt            ( per_target )
, t_group       ( target_group )
, persistent_   ( persistent_cond )
, hitmap        ( )
, events        ( events )
, sev_             ( )
, src_             ( )
, tgt_             ( )
, cat_             ( )
, bdy_             ( )
, what_            ( )
, last_sev_        ( )
, last_src_        ( )
, last_tgt_        ( )
, last_cat_        ( )
, last_bdy_        ( )
, last_what_       ( )
, notify_on_source ( )
, notify_on_target ( )
, notify_on_status ( )
, catched_messages ( 0 )
{
  // parse sources
  std::vector<std::string>::const_iterator it = sources.begin();
  while(it!=sources.end())
  {
    if (*it == "*")
    {
      any_src = true;
      e_srcs.clear();
      srcs_str = "any, ";
      break;
    }
    e_srcs.push_back(regex_t(*it));
    srcs_str.append(*it).append(", ");
    ++it;
  }

  // remove the last ", "
  srcs_str.resize(srcs_str.size()-2);

  // parse categories
  it = categories.begin();
  while(it!=categories.end())
  {
    if (*it == "*")
    {
      any_cat = true;
      e_cats.clear();
      cats_str = "any, ";
      break;
    }
    e_cats.push_back(regex_t(*it));
    cats_str.append(*it).append(", ");
    ++it;
  }

  // remove the last ", "
  cats_str.resize(cats_str.size()-2);

  // regex or contains?
  if (regex.empty() || (regex.compare("*")==0)) 
    match_type = MATCH_ANY;
  else               
    match_type = MATCH_REGEX;

  // test functions
  if( !parse_condition_test( test, test_expr) )
    throw std::runtime_error("condition test function parse failed");
}

void
  ma_condition::reset( )
{
  hitmap.reset();
  catched_messages = 0;
}

void ma_condition::init( )
{
  hitmap.set_parent(this);
}

bool 
  ma_condition::match( qt_mf_msg const & msg
                     , conds_t & status
                     , conds_t & source 
                     , conds_t & target )
{
  extract_fields(msg);

  // filtering conditions
  if (sev_<severity_) return false;
  if (!match_srcs())  return false;
  if (!match_cats())  return false;

  // match condition
  if (!match_body())  return false;

  // update fields after passing the match condition
  update_fields();

  // test condition
  if (!match_test())  return false;

  // register to hitmap
  unsigned int result = hitmap.capture(msg, src_, tgt_, what_);

  TLOG(TLVL_DEBUG) << "cond::match() result = " << result << " src = " << src_;

  // update reaction_start list
  if (result & STATUS_CHANGE)  status.push_back(this);
  if (result & SOURCE_CHANGE)  source.push_back(this);
  if (result & TARGET_CHANGE)  target.push_back(this);

  //if (result & STATUS_CHANGE)  update_fields();

  ++catched_messages;

  return true;
}

bool ma_condition::event( size_t src, size_t tgt, time_t t, conds_t & status )
{
  if ( hitmap.event(src, tgt, t) )
  {
    // we have a status flip
    status.push_back(this);
    return true;
  }

  return false;
}

void ma_condition::extract_fields (qt_mf_msg const & msg)
{
  sev_ = msg.sev();
         get_source_from_msg(src_, msg);
  cat_ = msg.cat().toStdString();
  bdy_ = msg.text(false).toStdString();
}

void ma_condition::update_fields ( )
{
  last_sev_ = sev_;
  last_src_ = src_;
  last_tgt_ = tgt_;
  last_cat_ = cat_;
  last_bdy_ = bdy_;
  last_what_ = what_;
}

bool ma_condition::match_srcs ( )
{
  if (any_src)  return true;

  size_t imax = e_srcs.size();

  for (size_t i=0; i<imax; ++i)
  {
    if (boost::regex_match(src_, what_, e_srcs[i])) return true;
  }

  return false;
}

bool ma_condition::match_cats ( )
{
  if (any_cat)  return true;

  size_t imax = e_cats.size();

  for (size_t i=0; i<imax; ++i)
  {
    if (boost::regex_match(cat_, what_, e_cats[i])) return true;
  }

  return false;
}

bool ma_condition::match_body ( )
{
  if (match_type == MATCH_ANY)  
    return true;

  if (boost::regex_search(bdy_, what_, e)) 
  {
    if( pt ) // per_target, now extract the target string
    {
      if( t_group > what_.size() )
        throw std::runtime_error("match_body: target group does not exit");

      tgt_ = std::string(what_[t_group].first, what_[t_group].second);
    }
    
    return true;
  }

  return false;
}

bool ma_condition::match_test ( )
{
  return test_expr.evaluate( this );
}
