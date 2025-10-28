#include "Logger.h"
#include <iostream>

namespace KitchenCAD {
namespace Utils {

// Static member definitions
std::unique_ptr<Logger> Logger::instance_ = nullptr;
std::mutex Logger::instanceMutex_;

} // namespace Utils
} // namespace KitchenCAD