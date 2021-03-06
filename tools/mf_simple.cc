// This file (mf_test.cc) was created by Ron Rechenmacher <ron@fnal.gov> on
// Apr 27, 2017. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: mf_test.cc,v $
// rev="$Revision: 1.5 $$Date: 2017/10/10 14:31:02 $";

// link with libMF_MessageLogger
#if 0
unsetup_all
setup messagefacility v1_18_03 -q e10:debug # will setup deps (cetlib, boost, cetlib_except, fhiclcpp)
: optional:; setup artdaq_mfextensions v1_01_10 -qe10:debug:s47

OR

unsetup_all
: setup messagefacility v2_01_00 -q e14:debug
setup messagefacility v2_00_02 -q e14:debug
: optional:; setup artdaq_mfextensions v1_01_12 -qe14:debug:s50

setup messagefacility     v2_02_03 -qe15:prof
setup artdaq_mfextensions v1_03_04 -qprof:e15:s67

setup TRACE v3_09_01
g++ -g -Wall -I$MESSAGEFACILITY_INC -I$CETLIB_INC -I$CETLIB_EXCEPT_INC -I$FHICLCPP_INC \
 -I$BOOST_INC \
 -I$TRACE_INC \
 -std=c++14 -o mf_cout_vs_TRACE_printf{,.cc} -L$MESSAGEFACILITY_LIB -L$FHICLCPP_LIB -L$CETLIB_LIB\
 -lMF_MessageLogger -lfhiclcpp -lcetlib && \
./mf_cout_vs_TRACE_printf test
#endif

#include <cstdlib>  // setenv
#include <string>
#include "TRACE/tracemf.h"  // TRACE
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
//#include "TRACE/trace.h"				// TRACE

#define TRACE_NAME "mftest"

const char *mf_test_config =
    "\
debugModules : [\"*\"]\n\
suppressInfo : []\n\
#    threshold : DEBUG\n\
destinations : {\n\
    threshold : DEBUG\n\
  #LogToConsole : {\n\
  #  type : Friendly     # this is the important \"label\" -- value can be cout, cerr, or file (more with mf_extensions)\n\
  #  threshold : DEBUG\n\
  #  #noLineBreaks : true\n\
  #  #lineLength : 200\n\
  #  #noTimeStamps : false\n\
  #  #useMilliseconds : true   # this will short circuit format:timestamp:\n\
  #  #outputStatistics : true  # this will cause exception if use by other than cout, cerr, or file\n\
  #  resetStatistics : false\n\
  #  categories : {\n\
  #    unimportant : { limit : 100 }\n\
  #    serious_matter : { limit : 1000 timespan : 60 }\n\
  #    default : { limit : 1000 }\n\
  #  }\n\
  #  format: { wantFullContext: true  timestamp: \"%FT%T%z\" } #wantSomeContext: false  }\n\
  #}\n\
  xxx: {\n\
    type: cout\n\
    threshold : DEBUG\n\
    format: { timestamp: \"%FT%T%z\" }\n\
  }\n\
}\n\
";

const char *mf_friendly_config =
    "\
debugModules : [\"*\"]\n\
suppressInfo : []\n\
#    threshold : DEBUG\n\
destinations : {\n\
    threshold : DEBUG\n\
  LogToConsole : {\n\
    type : Friendly     # this is the important \"label\" -- value can be cout, cerr, or file (more with mf_extensions)\n\
    threshold : DEBUG\n\
    #noLineBreaks : true\n\
    #lineLength : 200\n\
    #noTimeStamps : false\n\
    #useMilliseconds : true   # this will short circuit format:timestamp:\n\
    #outputStatistics : true  # this will cause exception if use by other than cout, cerr, or file\n\
    resetStatistics : false\n\
    categories : {\n\
      unimportant : { limit : 100 }\n\
      serious_matter : { limit : 1000 timespan : 60 }\n\
      default : { limit : 1000 }\n\
    }\n\
    format: { wantFullContext: true  timestamp: \"%FT%T%z\" noLineBreaks: true} #wantSomeContext: false  }\n\
  }\n\
  xxx: {\n\
    type: cout\n\
    threshold : DEBUG\n\
    format: { timestamp: \"%FT%T%z\" }\n\
  }\n\
}\n\
";

const char *mf_OTS_config =
    "\
debugModules : [\"*\"]\n\
suppressInfo : []\n\
#    threshold : DEBUG\n\
destinations : {\n\
    threshold : DEBUG\n\
  LogToConsole : {\n\
    type : OTS     # this is the important \"label\" -- value can be cout, cerr, or file (more with mf_extensions)\n\
    threshold : DEBUG\n\
    #noLineBreaks : true\n\
    #lineLength : 200\n\
    #noTimeStamps : false\n\
    #useMilliseconds : true   # this will short circuit format:timestamp:\n\
    #outputStatistics : true  # this will cause exception if use by other than cout, cerr, or file\n\
    resetStatistics : false\n\
    categories : {\n\
      unimportant : { limit : 100 }\n\
      serious_matter : { limit : 1000 timespan : 60 }\n\
      default : { limit : 1000 }\n\
    }\n\
    format: { wantFullContext: true  timestamp: \"%FT%T%z\" noLineBreaks: true} #wantSomeContext: false  }\n\
  }\n\
  xxx: {\n\
    type: cout\n\
    threshold : DEBUG\n\
    format: { timestamp: \"%FT%T%z\" }\n\
  }\n\
}\n\
";

const char *mf_TRACE_config =
    "\
debugModules : [\"*\"]\n\
suppressInfo : []\n\
#    threshold : DEBUG\n\
destinations : {\n\
    threshold : DEBUG\n\
  xxx: {\n\
    type: TRACE\n\
    threshold : DEBUG\n\
    format: { timestamp: \"%FT%T%z\" noLineBreaks: true}\n\
  }\n\
}\n\
";

int main(int argc, char *argv[])
{
	setenv("TRACE_MSG_MAX", "0", 0);
	setenv("TRACE_LIMIT_MS", "5,50,500", 0);  // equiv to TRACE_CNTL( "limit_ms", 5L, 50L, 500L )
	TRACE_CNTL("reset");
	fhicl::ParameterSet pset;
	if (argc == 2 && strcmp(argv[1], "test") == 0)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	{
		pset = fhicl::ParameterSet::make(std::string(mf_test_config));
		// ref. https://cdcvs.fnal.gov/redmine/projects/messagefacility/wiki/Build_and_start_messagefacility
	}
	else if (argc == 2 && strcmp(argv[1], "TRACE") == 0)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	{
		pset = fhicl::ParameterSet::make(std::string(mf_TRACE_config));
		// ref. https://cdcvs.fnal.gov/redmine/projects/messagefacility/wiki/Build_and_start_messagefacility
	}
	else if (argc == 2 && strcmp(argv[1], "friendly") == 0)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	{
		pset = fhicl::ParameterSet::make(std::string(mf_friendly_config));
		// ref. https://cdcvs.fnal.gov/redmine/projects/messagefacility/wiki/Build_and_start_messagefacility
	}
	else if (argc == 2 && strcmp(argv[1], "OTS") == 0)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	{
		pset = fhicl::ParameterSet::make(std::string(mf_OTS_config));
		// ref. https://cdcvs.fnal.gov/redmine/projects/messagefacility/wiki/Build_and_start_messagefacility
	}
	else if (argc == 2)
	{
		// i.e ./MessageFacility.cfg
		setenv("FHICL_FILE_PATH", ".", 0);
		cet::filepath_maker fpm;
		pset = fhicl::ParameterSet::make(argv[1], fpm);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	}
#if defined(__cplusplus) && (__cplusplus == 201300L)
	mf::StartMessageFacility(mf::MessageFacilityService::MultiThread, pset);
	mf::SetApplicationName(std::string("myAppName"));
#else
	mf::StartMessageFacility(pset);
	// mf::setEnabledState("not used");
	mf::SetApplicationName(std::string("myAppName"));
#endif

	// else total mf default

	TRACE(1, "\nHello\n");  // NOLINT
	TLOG_ERROR("mf_test_category") << "hello - this is an mf::LogError(\"mf_test_category\")\n";
	mf::LogAbsolute("abs_category/id") << "hello - this is an mf::LogAbsolute(\"abs_category/id\")";
	mf::LogAbsolute("abs_category/id", __FILE__) << "hello - this is an mf::LogAbsolute(\"abs_category/id\")";
	mf::LogAbsolute("abs_category/id", __FILE__, __LINE__) << "hello - this is an mf::LogAbsolute(\"abs_category/id\")";

	TRACE(1, "start 1000 LOG_DEBUG");  // NOLINT
	for (auto ii = 0; ii < 1000; ++ii)
	{
		TLOG_DEBUG("mf_test_category") << "this is a LOG_DEBUG " << ii;
	}

	TRACE(1, "end LOG_DEBUG, start 1000 TRACE");  // NOLINT

	for (auto ii = 0; ii < 1000; ++ii)
	{
		TRACEN_(TRACE_NAME, 1, "this is a TRACE_ " << ii);  // NOLINT
	}
	TRACE(1, "end TRACE");  // NOLINT

	for (auto ii = 0; ii < 2; ++ii)
	{
		::mf::LogTrace{"simply", __FILE__, __LINE__} << "this is a test";
	}
	return (0);
}  // main
