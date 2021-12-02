# Usage: %s [-mode <mode>] [-rate <rate>] "
#		"[-startgap <startgap>]\n"
#		"       [-port <port>] [-direction <dir> [-seed <seed>]\n",
#		progname);
#
#%s -h[elp]\n", progname);
#    <mod_mode> = garble, kill_seq, kill_length, passthrough\n");
#    <mod_rate>      = # unaltered messages between modifier\n");
#    <inj_mode>    = noise, events, kmevents, replay, mixed, passthrough\n");
#    <inj_rate>  = # milliseconds between injections\n");
#    <startgap>  = # unaltered messages before first operation\n");
#    <port>      = port on which Winjig listens\n");


# garble_mode
#   0 - passthrough
#   1 - garble        // Valid Test
#   2 - kill_seq      // Kills XBC
#   3 - kill_length   // Kills XBC

# inj_mode
#   0 - passthrough
#   1 - noise         // Kills XBC
#   2 - events
#   3 - kmevents
#   4 - replay
#   5 - mixed (replay & events) // Fine, but maybe stick to one or other

# Note:  There are still some issues with Fuzz21 falling behind the current
#         sequence number when large packets are sent.  So keep inj_rate
#         a little higher or alter code to only inject after 32 byte msgs

echo Running fuzz21

####################
# Passthrough
####################
# ./fuzz21 -port 6002 2> ./data/last_run.txt

####################
# Standard
####################


# Garble mode
# ./fuzz21 -mod_mode garble -mod_rate 20 -startgap 350 -port 6002 2> ./data/last_run.txt
# Injection mode - events
./fuzz21 -inj_mode kmevents -inj_rate 500 -startgap 350 -port 6002 2> ./data/last_run.txt
# Injection mode - replay (legal events)
#./fuzz21 -inj_mode replay -inj_rate 100 -startgap 350 -port 6002 2> ./data/last_run.txt


####################
# QuickKill
####################
#./fuzz21 -mod_mode kill_seq -mod_rate 20 -startgap 300 -port 6002 2> ./data/last_run.txt


####################
# Other
####################
#injcetion mode - kmevents

# Full attack mode
#./fuzz21 -inj_mode replay -inj_rate 250 -mod_mode garble -mod_rate 5 -startgap 350 -port 6002 2> ./data/last_run.txt