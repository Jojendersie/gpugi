
#include "../Time.h"

namespace
{
  struct TimeInit
  {
    TimeInit()
    {
      ezTime::Initialize();
    }
  } timeInit;
};

#include <windows.h>

static LARGE_INTEGER g_qpcFrequency = {0};

void ezTime::Initialize()
{
	QueryPerformanceFrequency(&g_qpcFrequency);
}

ezTime ezTime::Now()
{
	LARGE_INTEGER temp;
	QueryPerformanceCounter(&temp);

	return ezTime::Seconds((double(temp.QuadPart) / double(g_qpcFrequency.QuadPart)));
}
