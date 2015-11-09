/*
	Authored 2015. Phillip Stanley-Marbell.

	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	*	Redistributions of source code must retain the above
		copyright notice, this list of conditions and the following
		disclaimer.

	*	Redistributions in binary form must reproduce the above
		copyright notice, this list of conditions and the following
		disclaimer in the documentation and/or other materials
		provided with the distribution.

	*	Neither the name of the author nor the names of its
		contributors may be used to endorse or promote products
		derived from this software without specific prior written
		permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/



/*
 *	NOTE:   Add entries here for new locations we want to time stamp; all
 *	functions in the implementation should ideally have entries here.
 */
typedef enum
{
	/*
	 *	Code depends on this being last.
	 */
	kNoisyTimeStampKeyMax = 1,
} NoisyTimeStampKey;

typedef struct
{
	uint64_t		nanoseconds;
	NoisyTimeStampKey	key;
} NoisyTimeStamp;


#ifdef NoisyOsMacOS
#include <mach/mach.h>
#include <mach/mach_time.h>

/*
 *	NOTE: The final N->callAggregateTotal and N->timestampCount won't match
 *	because we roll timestampCount modulo number of slots. 
 *
 *	Need a cleaner solution...
 */
#define NoisyTimeStampTraceMacro(routineKey)		if (N->mode & kNoisyModeCallStatistics)\
							{\
								uint64_t	now = mach_absolute_time();\
								\
								N->timestamps[N->timestampCount].nanoseconds = now;\
								N->timestamps[N->timestampCount].key = (routineKey);\
								\
								if (N->callAggregateTotal > 0)\
								{\
									N->timeAggregates[N->timeAggregatesLastKey] += (now - N->timeAggregatesLastTimestamp);\
									N->timeAggregateTotal += (now - N->timeAggregatesLastTimestamp);\
								}\
								\
								N->timeAggregatesLastKey = (routineKey);\
								N->timeAggregatesLastTimestamp = now;\
								N->timestampCount = (N->timestampCount + 1) % N->timestampSlots;\
								\
								N->callAggregates[routineKey]++;\
								N->callAggregateTotal++;\
							}\

#else
#define NoisyTimeStampTraceMacro(routineKey) 
#endif /* NoisyOsMacOS */

extern const char *     NoisyTimeStampKeyStrings[kNoisyTimeStampKeyMax];
