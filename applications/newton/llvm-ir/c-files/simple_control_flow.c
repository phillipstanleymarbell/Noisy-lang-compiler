//
// Created by blackgeorge on 3/2/22.
//

#include <stdio.h>

int main()
{
	int i = 0;
	int s;

	while (i <= 10)
	{
		if (i == 1)
		{
			s = 0;
		}
		s = s + i;
		i++;
	}
	printf("%d\n", s);
}