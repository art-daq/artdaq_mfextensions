file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/moc_dummy.cpp "TEST")
file(GLOB MOCFILES ${CMAKE_CURRENT_BINARY_DIR}/moc_*.cpp)
message(STATUS "MOCFILES=${MOCFILES}")
file(REMOVE ${MOCFILES})

cet_make_library(LIBRARY_NAME MessageAnalyzer
  SOURCE
  ma_action.cpp
  ma_boolean_cond.cpp
  ma_cond_test_andexpr.cpp
  ma_domain_cond.cpp
  ma_function.cpp
  ma_participants.cpp
  ma_test_function.cpp
  ma_action_mail.cpp
  ma_boolean_expr.cpp
  ma_cond_test_expr.cpp
  ma_domain_expr.cpp
  ma_function_is_syncd.cpp
  ma_richmsg.cpp
  ma_tf_grp_to_number.cpp
  ma_action_script.cpp
  ma_cell.cpp
  ma_cond_test_primary.cpp
  ma_function_count.cpp
  ma_hitmap.cpp
  ma_rule.cpp
  ma_utils.cpp
  ma_boolean_andexpr.cpp
  ma_condition.cpp
  ma_domain_andexpr.cpp
  ma_function_countpercent.cpp
  ma_parse.cpp
  ma_rule_engine.cpp
  LIBRARIES
	messagefacility::MF_MessageLogger
	Qt5::Core
	Qt5::Gui
	TRACE::TRACE
	Boost::system
	Boost::thread
	Boost::regex
)

install_headers()
install_source()
