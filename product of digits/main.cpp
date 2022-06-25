/*
	Recursive product of digits

	Numberphile video: https://youtu.be/Wim9WJeDTHQ

	Steps | Smallest number
	------+---------------------
	0     | 0
	1     | 10
	2     | 25
	3     | 39
	4     | 77
	5     | 679
	6     | 6788
	7     | 68889
	8     | 2677889
	9     | 26888999
	10    | 377888899
	11    | 277777788888899
	12    | Should be imposible

	Should be largest 11 step: 77777733332222222222222222222
	Note that's 277777788888899 but expandened and reordered


	I ran this with suffixes of 0 to 1024 digits (ie MIN_DIGITS = 0, MAX_DIGITS = 1024):
	Prefix | Suffix regex
	-------+--------------
	2      | 7*8*9*
	26     | 7*8*9*
	3      | 7*8*9*
	4      | 7*8*9*
	6      | 7*8*9*
	None   | 7*8*9*
	3      | 5+7*9*
	None   | 5+7*9*

	Note regex repeaters: * means 0 or more, + means 1 or more


	The output summary for MIN_DIGITS = 0 and MAX_DIGITS = 1024:
	Steps | Count
	------+------------
	0     |          9
	1     |          0
	2     | 1438995148
	3     |      11994   Largest is MAX_DIGITS = 155
	4     |        387   Largest is MAX_DIGITS =  47
	5     |        142   Largest is MAX_DIGITS =  24
	6     |         41   Largest is MAX_DIGITS =  18
	7     |         12   Largest is MAX_DIGITS =  17
	8     |          8   Largest is MAX_DIGITS =  17
	9     |          5   Largest is MAX_DIGITS =  13
	10    |          2   Largest is MAX_DIGITS =  12
	11    |          2   Largest is MAX_DIGITS =  16
	Took: 5 hr, 17 min (i5-6500 3.2 GHz)
*/

#define MIN_DIGITS          0
#define MAX_DIGITS          1024
#define PROGRESS_FREQUENCY  400  // Output progress every "0.25%". Note progress is nonlinear.
#define MIN_STEPS_TO_PRINT  10

#include <stdio.h>
#include <inttypes.h>

#ifdef _WIN32
	#include <intrin.h>
	#include <windows.h>
	typedef LARGE_INTEGER TIMER_TYPE;
	#define TIMER_FUNC(t)             QueryPerformanceCounter(&t)
	inline double TIMER_DIFF(TIMER_TYPE s, TIMER_TYPE e)
	{
		TIMER_TYPE f;
		QueryPerformanceFrequency(&f);
		return ((double) (e.QuadPart - s.QuadPart)) / f.QuadPart;
	}
#else
	#include <sys/time.h>
	typedef timeval TIMER_TYPE;
	#define TIMER_FUNC(t)             gettimeofday(&t, NULL)
	#define TIMER_DIFF(s,e)           ((e.tv_sec - s.tv_sec) + (e.tv_usec - s.tv_usec) / (double)1000000.0)

	typedef unsigned int uint128_t __attribute__((mode(TI)));

	inline uint32_t _udiv64(uint64_t num, uint32_t div, uint32_t *rem)
	{
		*rem = (uint32_t) (num % div);
		return (uint32_t) (num / div);
	}

	inline uint64_t _udiv128(uint64_t hi, uint64_t lo, uint64_t div, uint64_t *rem)
	{
		uint128_t num = ((uint128_t) hi << 64) | lo;

		*rem = (uint64_t) (num % div);
		return (uint64_t) (num / div);
	}

	inline uint64_t _umul128(uint64_t a, uint64_t b, uint64_t *hi)
	{
		uint128_t num = (uint128_t) a * b;

		*hi = (uint64_t) (num >> 64);
		return (uint64_t) num;
	}
#endif

// BigInt is optimized for base 10
class BigInt
{
public:
	BigInt()
	{
		m_num = new uint64_t[1];
		m_num[0] = 0;
		m_limbs = 1;
	}
	BigInt(uint64_t n)
	{
		if (n >= BIGINT_MOD)
		{
			m_num = new uint64_t[2];
			m_num[0] = n % BIGINT_MOD;
			m_num[1] = n / BIGINT_MOD;
			m_limbs = 2;
		}
		else
		{
			m_num = new uint64_t[1];
			m_num[0] = n;
			m_limbs = 1;
		}
	}
	~BigInt()
	{
		delete [] m_num;
	}

	void operator=(const BigInt &n)
	{
		uint64_t       *a;
		const uint64_t *b     = n.m_num;
		size_t          limbs = n.m_limbs;
		
		delete [] m_num;
		m_num = a = new uint64_t[limbs];
		for (size_t i = 0; i < limbs; i++)
		{
			a[i] = b[i];
		}
		m_limbs = limbs;
	}

	void set(uint64_t n)
	{
		delete [] m_num;
		if (n >= BIGINT_MOD)
		{
			m_num = new uint64_t[2];
			m_num[0] = n % BIGINT_MOD;
			m_num[1] = n / BIGINT_MOD;
			m_limbs = 2;
		}
		else
		{
			m_num = new uint64_t[1];
			m_num[0] = n;
			m_limbs = 1;
		}
	}

	void mul(const BigInt &n)
	{
		const uint64_t *a      = n.m_num;
		uint64_t       *b      = m_num;
		uint64_t       *c;
		size_t          limbsA = n.m_limbs;
		size_t          limbsB = m_limbs;
		size_t          limbsC;

		if (limbsA == 0 || limbsB == 0)
		{
			m_limbs = 0;
			return;
		}

		limbsC = limbsA + limbsB;
		if (limbsC == 2)
		{
			uint64_t lo, hi;

			lo = _umul128(a[0], b[0], &hi);
			if (hi != 0 || lo >= BIGINT_MOD)
			{
				c = new uint64_t[2];
				hi = _udiv128(hi, lo, BIGINT_MOD, &lo);
				c[0] = lo;
				c[1] = hi;
			}
			else
			{
				c = new uint64_t[1];
				c[0] = lo;
				limbsC = 1;
			}
		}
		else
		{
			c = new uint64_t[limbsC];
			for (size_t i = 0; i < limbsC; i++)
			{
				c[i] = 0;
			}
			for (size_t i = 0; i < limbsA; i++)
			{
				for (size_t j = 0; j < limbsB; j++)
				{
					uint64_t lo, hi;

					lo = _umul128(a[i], b[j], &hi);
					hi = _udiv128(hi, lo, BIGINT_MOD, &lo);

					lo += c[i + j];
					hi += c[i + j + 1];
					if (lo >= BIGINT_MOD)
					{
						lo -= BIGINT_MOD;
						hi++;
					}
					if (hi >= BIGINT_MOD)
					{
						hi -= BIGINT_MOD;
						c[i + j + 2]++;
					}
					c[i + j    ] = lo;
					c[i + j + 1] = hi;
				}
			}
		}
		uint64_t carry = 0;
		for (size_t i = 0; i < limbsC; i++)
		{
			uint64_t x = c[i] + carry;
			carry = 0;
			while (x >= BIGINT_MOD)
			{
				x -= BIGINT_MOD;
				carry++;
			}
			c[i] = x;
		}
		while (limbsC != 0)
		{
			if (c[limbsC - 1] != 0)
			{
				break;
			}
			limbsC--;
		}
		delete [] m_num;
		m_num = c;
		m_limbs = limbsC;
	}

	const void toDigits(uint32_t digitCount[10])
	{
		const uint64_t *num   = m_num;
		size_t          limbs = m_limbs;

		for (size_t i = 0; i < 10; i++)
		{
			digitCount[i] = 0;
		}
		if (limbs == 0)
		{
			digitCount[0] = 1;
			return;
		}

		for (size_t i = 0; i < limbs - 1; i++)
		{
			uint32_t hi, lo, x;

			hi = _udiv64(num[i], 1000000000, &lo);
			x = digits[lo % 1000]; lo /= 1000;
			digitCount[ x       & 0xf]++;
			digitCount[(x >> 4) & 0xf]++;
			digitCount[ x >> 8       ]++;
			x = digits[lo % 1000]; lo /= 1000;
			digitCount[ x       & 0xf]++;
			digitCount[(x >> 4) & 0xf]++;
			digitCount[ x >> 8       ]++;
			x = digits[lo];
			digitCount[ x       & 0xf]++;
			digitCount[(x >> 4) & 0xf]++;
			digitCount[ x >> 8       ]++;
			x = digits[hi % 1000]; hi /= 1000;
			digitCount[ x       & 0xf]++;
			digitCount[(x >> 4) & 0xf]++;
			digitCount[ x >> 8       ]++;
			x = digits[hi % 1000]; hi /= 1000;
			digitCount[ x       & 0xf]++;
			digitCount[(x >> 4) & 0xf]++;
			digitCount[ x >> 8       ]++;
			x = digits[hi];
			digitCount[ x       & 0xf]++;
			digitCount[(x >> 4) & 0xf]++;
			digitCount[ x >> 8       ]++;
		}

		uint64_t n = num[limbs - 1];
		uint32_t hi, lo, x;

		lo = (uint32_t) n;
		if (n >= 1000000000)
		{
			hi = _udiv64(n, 1000000000, &lo);
		}
		x = digits[lo % 1000]; lo /= 1000;
		if (n < 1000)
		{
			digitCount[ x       & 0xf]++; if (x >> 4 == 0) { return; }
			digitCount[(x >> 4) & 0xf]++; if (x >> 8 == 0) { return; }
			digitCount[ x >> 8       ]++;
			return;
		}
		else
		{
			digitCount[ x       & 0xf]++;
			digitCount[(x >> 4) & 0xf]++;
			digitCount[ x >> 8       ]++;
		}
		x = digits[lo % 1000]; lo /= 1000;
		if (n < 1000000)
		{
			digitCount[ x       & 0xf]++; if (x >> 4 == 0) { return; }
			digitCount[(x >> 4) & 0xf]++; if (x >> 8 == 0) { return; }
			digitCount[ x >> 8       ]++;
			return;
		}
		else
		{
			digitCount[ x       & 0xf]++;
			digitCount[(x >> 4) & 0xf]++;
			digitCount[ x >> 8       ]++;
		}
		x = digits[lo];
		if (n < 1000000000)
		{
			digitCount[ x       & 0xf]++; if (x >> 4 == 0) { return; }
			digitCount[(x >> 4) & 0xf]++; if (x >> 8 == 0) { return; }
			digitCount[ x >> 8       ]++;
			return;
		}
		else
		{
			digitCount[ x       & 0xf]++;
			digitCount[(x >> 4) & 0xf]++;
			digitCount[ x >> 8       ]++;
		}
		x = digits[hi % 1000]; hi /= 1000;
		if (hi == 0)
		{
			digitCount[ x       & 0xf]++; if (x >> 4 == 0) { return; }
			digitCount[(x >> 4) & 0xf]++; if (x >> 8 == 0) { return; }
			digitCount[ x >> 8       ]++;
			return;
		}
		else
		{
			digitCount[ x       & 0xf]++;
			digitCount[(x >> 4) & 0xf]++;
			digitCount[ x >> 8       ]++;
		}
		x = digits[hi % 1000]; hi /= 1000;
		if (hi == 0)
		{
			digitCount[ x       & 0xf]++; if (x >> 4 == 0) { return; }
			digitCount[(x >> 4) & 0xf]++; if (x >> 8 == 0) { return; }
			digitCount[ x >> 8       ]++;
			return;
		}
		else
		{
			digitCount[ x       & 0xf]++;
			digitCount[(x >> 4) & 0xf]++;
			digitCount[ x >> 8       ]++;
		}
		x = digits[hi];
		digitCount[ x       & 0xf]++; if (x >> 4 == 0) { return; }
		digitCount[(x >> 4) & 0xf]++; if (x >> 8 == 0) { return; }
		digitCount[ x >> 8       ]++;
	}

private:
	uint64_t *m_num;
	size_t    m_limbs;
	static uint32_t *digits;
	static const uint64_t BIGINT_MOD = UINT64_C(1000000000000000000);

	static uint32_t *makeDigits()
	{
		digits = new uint32_t[1000];
		for (uint32_t a = 0, i = 0; a < 10; a++)
		{
			for (uint32_t b = 0; b < 10; b++)
			{
				for (uint32_t c = 0; c < 10; c++, i++)
				{
					digits[i] = (a << 8) | (b << 4) | c;
				}
			}
		}
		return digits;
	}
};

uint32_t *BigInt::digits = BigInt::makeDigits();

void doSteps(const uint32_t digitCount_[10], BigInt **pows, uint64_t stepCounts[32])
{
	uint32_t digitCount[10];
	uint32_t steps = 0;

	for (int i = 0; i < 10; i++)
	{
		digitCount[i] = digitCount_[i];
	}
	while (1)
	{
		uint32_t digits = 0;
		for (int i = 0; i < 10; i++)
		{
			digits += digitCount[i];
		}
		if (digits <= 1)
		{
			break;
		}
		steps++;
		if (digitCount[0])
		{
			break;
		}
		if (digitCount[5])
		{
			if (digitCount[2] || digitCount[4] || digitCount[6] || digitCount[8])
			{
				steps++;
				break;
			}
		}

		BigInt num;
		int isSet = 0;

		for (int i = 2; i < 10; i++)
		{
			uint32_t count = digitCount[i];
			if (count != 0)
			{
				if (count > MAX_DIGITS)
				{
					printf("FUCK!?: ");
					for (int i = 0; i < 10; i++)
					{
						for (uint32_t j = 0; j < digitCount_[i]; j++)
						{
							printf("%u", i);
						}
					}
					printf("\n");
					return;
				}
				if (isSet)
				{
					num.mul(pows[i][count - 1]);
				}
				else
				{
					num = pows[i][count - 1];
					isSet = 1;
				}
			}
		}

		num.toDigits(digitCount);
	}

	uint32_t digits = 0;
	for (int i = 0; i < 10; i++)
	{
		digits += digitCount_[i];
	}
	if (steps >= MIN_STEPS_TO_PRINT)
	{
		printf("Steps %u: ", steps);
		for (int i = 0; i < 10; i++)
		{
			for (uint32_t j = 0; j < digitCount_[i]; j++)
			{
				printf("%u", i);
			}
		}
		printf("\n");
	}
	if (steps < 32)
	{
		stepCounts[steps]++;
	}
}

void productOfDigits()
{
	uint64_t stepCounts[32] = {0};
	BigInt **pows = new BigInt*[10];

	for (int i = 2; i < 10; i++)
	{
		pows[i] = new BigInt[MAX_DIGITS];
		BigInt *pow = pows[i];
		BigInt curPow(i);
		BigInt num(i);

		pow[0].set(i);
		for (int j = 1; j < MAX_DIGITS; j++)
		{
			curPow.mul(num);
			pow[j] = curPow;
		}
	}

	uint32_t digitCount[10];
	uint64_t total = 0;
	uint64_t totalProgress = 0;

	for (int sevens = 0; sevens <= MAX_DIGITS; sevens++)
	{
		for (int eights = 0, eightsEnd = MAX_DIGITS - sevens; eights <= eightsEnd; eights++)
		{
			int ninesStart = 0;
			if (sevens + eights < MIN_DIGITS)
			{
				ninesStart = MIN_DIGITS - sevens - eights;
			}
			int ninesEnd = MAX_DIGITS - sevens - eights;
			if (ninesEnd >= ninesStart)
			{
				totalProgress += ninesEnd - ninesStart + 1;
			}
		}
	}

	uint64_t nextTotal = (totalProgress + (PROGRESS_FREQUENCY / 2)) / PROGRESS_FREQUENCY;
	uint64_t nextTotalCount = 1;
	TIMER_TYPE s, e;

	printf("Status: progress%% time total_time time_left\n");
	TIMER_FUNC(s);

	// 2 7*8*9*
	// 267*8*9*
	// 3 7*8*9*
	// 4 7*8*9*
	// 6 7*8*9*
	//   7*8*9*
	// 3 5+7*9*
	//   5+7*9*
	for (int sevens = 0; sevens <= MAX_DIGITS; sevens++)
	{
		for (int eights = 0, eightsEnd = MAX_DIGITS - sevens; eights <= eightsEnd; eights++)
		{
			int ninesStart = 0;
			if (sevens + eights < MIN_DIGITS)
			{
				ninesStart = MIN_DIGITS - sevens - eights;
			}
			for (int nines = ninesStart, ninesEnd = MAX_DIGITS - sevens - eights; nines <= ninesEnd; nines++, total++)
			{
				for (int i = 0; i < 7; i++)
				{
					digitCount[i] = 0;
				}
				digitCount[7] = sevens;
				digitCount[8] = eights;
				digitCount[9] = nines;

				// 2 7*8*9*
				digitCount[2] = 1;
				doSteps(digitCount, pows, stepCounts);

				// 267*8*9*
				digitCount[6] = 1;
				doSteps(digitCount, pows, stepCounts);

				// 6 7*8*9*
				digitCount[2] = 0;
				doSteps(digitCount, pows, stepCounts);

				// 3 7*8*9*
				digitCount[3] = 1;
				digitCount[6] = 0;
				doSteps(digitCount, pows, stepCounts);

				// 4 7*8*9*
				digitCount[3] = 0;
				digitCount[4] = 1;
				doSteps(digitCount, pows, stepCounts);

				//   7*8*9*
				digitCount[4] = 0;
				doSteps(digitCount, pows, stepCounts);

				if (eights > 0)
				{
					// 3 5+7*9*
					digitCount[3] = 1;
					digitCount[5] = eights; // use eights as fives
					digitCount[8] = 0;
					doSteps(digitCount, pows, stepCounts);

					//   5+7*9*
					digitCount[3] = 0;
					doSteps(digitCount, pows, stepCounts);
				}
			}
			if (total >= nextTotal)
			{
				TIMER_FUNC(e);
				double t = TIMER_DIFF(s, e);
				printf("Status: %0.2f%% %0.2f %0.0f %0.0f\n", total * 100.0 / totalProgress, t, t * totalProgress / total, t * totalProgress / total - t);
				do
				{
					nextTotalCount++;
					nextTotal = (nextTotalCount * totalProgress + (PROGRESS_FREQUENCY / 2)) / PROGRESS_FREQUENCY;
				} while (nextTotal <= total);
			}
		}
	}

	// Print summary
	printf("\nSteps: count\n");
	int max = 0;
	for (int i = 0; i < 32; i++)
	{
		if (stepCounts[i])
		{
			max = i;
		}
	}
	for (int i = 0; i <= max; i++)
	{
		printf("%u: %" PRIu64 "\n", i, stepCounts[i]);
	}

	// Clean up
	for (int i = 2; i < 10; i++)
	{
		delete [] pows[i];
	}
	delete [] pows;
}

int main()
{
	TIMER_TYPE s, e;

	TIMER_FUNC(s);
	productOfDigits();
	TIMER_FUNC(e);

	printf("Total time: %f\n", TIMER_DIFF(s, e));

	return 0;
}
