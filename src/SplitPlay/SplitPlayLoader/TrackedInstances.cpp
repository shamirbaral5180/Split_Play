#include "TrackedInstances.h"

std::unordered_map<SplitPlayInstanceHandle, SplitPlay::SplitPlayInstance> SplitPlay::instances;
SplitPlayInstanceHandle SplitPlay::instanceCounter = 1;
