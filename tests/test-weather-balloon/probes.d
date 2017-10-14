/*
 *      To fix the 'dtrace: 5860 dynamic variable drops with non-empty dirty list'.
 *
 *      See dtrace_tips.pdf and the internet
 */
#pragma D option dynvarsize=64m

provider newton {
   probe newton__start();
   probe newton__done();
};
