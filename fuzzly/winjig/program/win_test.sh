# Usage: %s [-mode <mode>] [-rate <rate>] "
#		"[-startgap <startgap>]\n"
#		"       [-port <port>] [-direction <dir> [-seed <seed>]\n",
#		progname);
#
# <mode>      = messages, garbling, events, kmevents\n");
# <rate>      = # unaltered messages between operations\n");
# <startgap>  = # unaltered messages before first operation\n");
# <port>      = port on which Winjig listens\n");
# <direction> = s2c, c2s, both\n");
# <seed>      = seed for random number generator\n");


echo Running Winjig
./winjig -mode kmevents -startgap 50 -port 6002 2> ./data/last_run.txt