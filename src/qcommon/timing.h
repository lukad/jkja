
#include <chrono>

// Measures an interval, in nanoseconds.
class timing_c
{
private:
	std::chrono::steady_clock::time_point	start;
public:
	timing_c(void)
	{
	}
	void Start()
	{
		start = std::chrono::steady_clock::now();
	}
	int End()
	{
		const std::chrono::nanoseconds time = std::chrono::steady_clock::now() - start;
		return((int)time.count());
	}
};

// end
