#include "auxLogger.h"

std::unique_ptr<aux::Logger> aux::Logger::m_Logger;
std::mutex aux::Logger::m_Mutex;
std::ostream* aux::Logger::m_pLogStream = &std::cout;