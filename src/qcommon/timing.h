
class timing_c
{
private:
	long long	start;
	long long	end;

	int		reset;
public:
	timing_c(void)
	{
	}
	void Start()
	{
		const long long *s = &start;

		__asm
		{
			push eax
			push ebx
			push edx

			rdtsc
			mov ebx, s
			mov	[ebx], eax
			mov [ebx + 4], edx

			pop edx
			pop ebx
			pop eax
		}
	}
	int End()
	{
		const long long *e = &end;
		long long	time;

		__asm
		{
			push eax
			push ebx
			push edx

			rdtsc
			mov ebx, e
			mov	[ebx], eax
			mov [ebx + 4], edx

			pop edx
			pop ebx
			pop eax
		}
		time = end - start;
		if (time < 0)
		{
			time = 0;
		}
		return((int)time);
	}
};

// end
