/*
	Authored 2021, Orestis Kaparounakis.

	All rights reserved.

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

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define FRAC_Q			10
#define K   (1 << (FRAC_Q - 1))
#define FRAC_BASE		(1<<FRAC_Q)
#define DEC2FRAC(_d)		((uint8_t)(_d*FRAC_BASE))
#define DISPLAY_INT(_x)		((_x>>FRAC_Q) + (((uint32_t)_x)>>(sizeof(int32_t)*FRAC_Q-1)))
#define DISPLAY_FRAC(_y)	(((FRAC_BASE-1)&_y) * (10000/FRAC_BASE))
#define FORMAT_FIXP		"%s%d.%04d"

void	MadgwickAHRSupdate(int32_t gx, int32_t gy, int32_t gz, int32_t ax, int32_t ay, int32_t az, int32_t mx, int32_t my, int32_t mz,
                           int32_t* q0_ptr, int32_t* q1_ptr, int32_t* q2_ptr, int32_t* q3_ptr);
void	MadgwickAHRSupdateIMU(int32_t gx, int32_t gy, int32_t gz, int32_t ax, int32_t ay, int32_t az,
                              int32_t* q0_ptr, int32_t* q1_ptr, int32_t* q2_ptr, int32_t* q3_ptr);

int32_t	sqrt_rsqrt(int32_t x, int recip);
