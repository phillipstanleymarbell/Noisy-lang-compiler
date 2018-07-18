#!/usr/sbin/dtrace -s
 
 
newton*:::newton-start
{
        self->start = timestamp;
}
 
newton*:::newton-done
{
        @times["newton"] = sum(timestamp - self->start);
}
 
END
{
        printa(@times);
}
