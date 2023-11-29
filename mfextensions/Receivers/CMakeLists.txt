file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/moc_dummy.cpp "TEST")
file(GLOB MOCFILES ${CMAKE_CURRENT_BINARY_DIR}/moc_*.cpp)
message(STATUS "MOCFILES=${MOCFILES}")
file(REMOVE ${MOCFILES})

cet_register_export_set(SET_NAME PluginTypes NAMESPACE artdaq_plugin_types)

cet_make_library(LIBRARY_NAME MFReceivers
  EXCLUDE LogReader_receiver.cc UDP_receiver.cc
  LIBRARIES
  TRACE::TRACE
  Boost::regex
  fhiclcpp::fhiclcpp
  messagefacility::MF_MessageLogger
  Qt5::Core
  Qt5::Gui
)


cet_make_library(LIBRARY_NAME receiver INTERFACE
				 EXPORT_SET PluginTypes
				 SOURCE MVReceiver.hh
				 LIBRARIES INTERFACE
				 art_plugin_support::plugin_config_macros
				 art_plugin_support::support_macros
				 artdaq_mfextensions::MFReceivers)

include(BasicPlugin)
foreach(type IN ITEMS receiver)
  cet_make_plugin_builder(${type} ${type} Modules
    LIBRARIES REG artdaq_plugin_types::${type})
endforeach()

cet_collect_plugin_builders(Modules ReceiverPlugins)
include(ReceiverPlugins)

cet_build_plugin(LogReader receiver)
cet_build_plugin(UDP receiver)

install_headers(SUBDIRS detail)
install_source(SUBDIRS detail)
