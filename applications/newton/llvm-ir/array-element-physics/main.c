typedef double	time;
typedef double	distance;
typedef double	speed;


int
main(int argc, char const *argv[])
{
	double state[2];

	time		dt = 0.1;	/* seconds */
	distance	x = 5;		/* meters  */
	speed		v = 1;		/* m/s     */

	state[0] = x;
	state[1] = v;


	/* Next state */
	for (int i = 0; i < 10; i++)
	{
		state[0] = state[i] + v*dt;
	}

	return 0;
}

