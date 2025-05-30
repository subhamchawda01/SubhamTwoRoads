project(execs)

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

set(STATIC_LIBS
  protobuf
  boost_date_time
  boost_filesystem
  boost_iostreams
  boost_system
  Threads::Threads
  z
)

set(DYNAMIC_LIBS
  ${CMAKE_DL_LIBS}
)

add_executable(mid_term_data_server midterm_data_server.cpp)
# target_link_libraries(mid_term_data_server CDef Utils TradingInfo CommonTradeUtils ExternalData MarketAdapter OrderRouting Profiler MidTerm ModelMath CommonDataStructures Indicators BaseUtils OptionsUtils FpgaHeader)
target_link_libraries(mid_term_data_server CDef Utils TradingInfo CommonTradeUtils ExternalData MarketAdapter OrderRouting Profiler MidTerm ModelMath CommonDataStructures Indicators BaseUtils OptionsUtils)
target_link_libraries(mid_term_data_server -Wl,--start-group -Wl,-Bstatic ${STATIC_LIBS} -Wl,-Bdynamic ${DYNAMIC_LIBS} -Wl,--end-group)

INSTALL(TARGETS
  mid_term_data_server
  DESTINATION ${CMAKE_SOURCE_DIR}/../qplum_install/bin
)
