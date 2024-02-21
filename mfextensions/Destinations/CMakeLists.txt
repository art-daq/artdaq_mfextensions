
include(mfPlugin)
find_package(CURL 7.20.0)

if(CURL_FOUND)
  SET(CMAKE_C_FLAGS "-std=gnu11 -fPIC -Wno-implicit-fallthrough" CACHE STRING "" FORCE)
cet_make_library(LIBRARY_NAME curl_send_message 
                 SOURCE detail/curl_send_message.c
                 LIBRARIES 
				 CURL::libcurl
                           WITH_STATIC_LIBRARY)

mfPlugin( SMTP LIBRARIES REG curl_send_message Qt5::Core Boost::thread)

endif()

mfPlugin( UDP LIBRARIES REG TRACE::TRACE )
mfPlugin( OTS LIBRARIES REG TRACE::TRACE )
mfPlugin( TRACE  LIBRARIES REG TRACE::TRACE)
mfPlugin( Friendly )
mfPlugin( MultiFile )
mfPlugin( ANSI )

install_fhicl(SUBDIRS fcl)
install_headers()
install_source()
