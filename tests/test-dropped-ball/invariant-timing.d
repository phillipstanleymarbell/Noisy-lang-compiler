#!/usr/sbin/dtrace -s
 
#pragma D option quiet
 
newton*:::newton-start
{
  self->start = timestamp;
}
 
newton*:::newton-done
{
        @times["invariants"] = sum(timestamp - self->start);
}

END
{
        printa(@times);
}
