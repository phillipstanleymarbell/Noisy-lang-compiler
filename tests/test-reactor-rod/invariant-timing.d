#!/usr/sbin/dtrace -s
 
#pragma D option quiet
 
invariant*:::invariant-start
{
  self->start = timestamp;
}
 
invariant*:::invariant-done
{
        @times["invariants"] = sum(timestamp - self->start);
}

END
{
        printa(@times);
}
