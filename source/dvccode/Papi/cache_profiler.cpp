
#include "dvccode/Papi/cache_profiler.hpp"

CacheProfiler::CacheProfiler(const ProfilerTypes::HwEvent& profiler_type)
    : _event_set(PAPI_NULL), _profiler_type(profiler_type) {
  if (!PapiInit()) {
    // throws exception;
    throw "CacheProfiler Object cannot be created.";
  }

  switch (profiler_type) {
    case ProfilerTypes::HwEvent::L1I_L1D_CACHE_PROFILE: {
      AddL1InstructionAndDataEvents();
      printf("ProfilerTypes::HwEvent::L1I_L1D_CACHE_PROFILE\n");
    } break;

    case ProfilerTypes::HwEvent::L2_L3_CACHE_PROFILE:
    default: {
      AddL2L3Events();
      printf("ProfilerTypes::HwEvent::L2_L3_CACHE_PROFILE\n");
    }
  }
}

CacheProfiler::~CacheProfiler() {
  int retval = PAPI_cleanup_eventset(_event_set);
  if (retval != PAPI_OK) {
    printf("File:%s Line:%d Msg:%s RetVal:%d\n", __FILE__, __LINE__, "PAPI_destroy", retval);
  }

  retval = PAPI_destroy_eventset(&_event_set);
  if (retval != PAPI_OK) {
    printf("File:%s Line:%d Msg:%s RetVal:%d\n", __FILE__, __LINE__, "PAPI_destroy_eventset", retval);
  }
}

bool CacheProfiler::PapiInit() {
  //! checking the Performance API version.
  int retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT) {
    printf("File:%s Line:%d Msg:%s RetVal:%d\n", __FILE__, __LINE__, "PAPI_library_init", retval);
    return false;
  }

  //! creating the eventset, all the PAPI events will be added to this event.
  // retval = PAPI_create_eventset(eventset);
  retval = PAPI_create_eventset(&_event_set);
  if (retval != PAPI_OK) {
    printf("File:%s Line:%d Msg:%s RetVal:%d\n", __FILE__, __LINE__, "PAPI_create_eventset", retval);
    return false;
  }
  std::cout << "CacheProfiler::PapiInit success" << std::endl;
  return true;
}

/*
 *  AddPapiEvent: adds a PAPI event to the event set.
 *  returns true if added successfully, otherwise false.
 */
bool CacheProfiler::AddPapiEvent(int event) {
  int retval = PAPI_add_event(_event_set, event);
  if (retval) {
    printf("%s %d %s %d\n", __FILE__, __LINE__, "PAPI_add_event", retval);
    return false;
  }

  return true;
}

/*
 *  RemovePapiEvent: removes the evnet from the eventset
 *  returns true if removed successfully, otherwise false.
 */
bool CacheProfiler::RemovePapiEvent(int event) {
  int retval = PAPI_remove_event(_event_set, event);
  if (retval != PAPI_OK) {
    printf("%s %d %s %d\n", __FILE__, __LINE__, "PAPI_remove_event", retval);
    return false;
  }
  return true;
}

int CacheProfiler::GetNumEvents() {
  int retval = PAPI_num_events(_event_set);
  return retval;
}

void CacheProfiler::StartProfiler() {
  int retval = PAPI_start(_event_set);
  if (retval) {
    printf("%s %d %s %d\n", __FILE__, __LINE__, "PAPI_start", retval);
  }
}

//! Resets the event_set counters to 0
void CacheProfiler::ResetProfilerStats() {
  int retval = PAPI_reset(_event_set);
  if (retval) {
    printf("%s %d %s %d\n", __FILE__, __LINE__, "PAPI_reset", retval);
  }
}

//! copies the event_set counters to refvalue,
//! also resets the event_set counter to 0.
void CacheProfiler::ReadProfilerStats() {
  int retval = PAPI_read(_event_set, _refvalue);
  if (retval) {
    printf("%s %d %s %d\n", __FILE__, __LINE__, "PAPI_read", retval);
  }
}

void CacheProfiler::EndProfiler() {
  int retval = PAPI_stop(_event_set, _refvalue);
  if (retval) {
    printf("%s %d %s %d\n", __FILE__, __LINE__, "PAPI_stop", retval);
  }
}

std::string CacheProfiler::GetProfilerStats() {
  std::stringstream ss;
  for (int i = 0; i < GetNumEvents(); i++) {
    ss << _refvalue[i] << ",";
  }
  std::string prof_stat_str = ss.str();
  return prof_stat_str;
}

std::string CacheProfiler::GetCacheMissRatio() {
  if (GetNumEvents() == 4 && _profiler_type == ProfilerTypes::HwEvent::L1I_L1D_CACHE_PROFILE) {
    std::stringstream ss;
    ss << "L1I_Cache_Miss_Ratio:" << ((double)_refvalue[0] / (double)_refvalue[2]) * 100.0 << ",";
    ss << "L1D_Cache_Miss_Ratio:" << ((double)_refvalue[1] / (double)_refvalue[3]) * 100.0;
    std::string prof_stat_str = ss.str();
    return prof_stat_str;
  } else if (GetNumEvents() == 4 && _profiler_type == ProfilerTypes::HwEvent::L2_L3_CACHE_PROFILE) {
    std::stringstream ss;
    ss << "L2_Cache_Miss_Ratio:" << ((double)_refvalue[0] / (double)_refvalue[2]) * 100.0 << ",";
    ss << "L3_Cache_Miss_Ratio:" << ((double)_refvalue[1] / (double)_refvalue[3]) * 100.0;
    std::string prof_stat_str = ss.str();
    return prof_stat_str;
  } else
    return "Insufficient Data";
}

void CacheProfiler::AddL1InstructionAndDataEvents() {
  if (AddPapiEvent(PAPI_L1_ICM)) {
    std::cout << "Added PAPI_L1_ICM event." << std::endl;
  }
  if (AddPapiEvent(PAPI_L1_DCM)) {
    std::cout << "Added PAPI_L1_DCM event." << std::endl;
  }

  if (AddPapiEvent(PAPI_L1_ICA)) {
    std::cout << "Added PAPI_L1_ICA event." << std::endl;
  }
  if (AddPapiEvent(PAPI_L1_DCA)) {
    std::cout << "Added PAPI_L1_DCA event." << std::endl;
  }
  std::cout << "Num of Events Added:" << GetNumEvents() << std::endl;
}

void CacheProfiler::AddL2L3Events() {
  if (AddPapiEvent(PAPI_L2_TCM)) {
    std::cout << "Added PAPI_L2_TCM event." << std::endl;
  }
  if (AddPapiEvent(PAPI_L3_TCM)) {
    std::cout << "Added PAPI_L3_TCM event." << std::endl;
  }

  if (AddPapiEvent(PAPI_L2_TCA)) {
    std::cout << "Added PAPI_L2_TCA event." << std::endl;
  }
  if (AddPapiEvent(PAPI_L3_TCA)) {
    std::cout << "Added PAPI_L3_TCA event." << std::endl;
  }
  std::cout << "Num of Events Added:" << GetNumEvents() << std::endl;
}
