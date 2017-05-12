# ../sim -l1000
# ../sim -l1000
Numswitches 1
Switch 0
	Numinputs    8
	Numoutputs   8
	InputAction  defaultInputAction  
	OutputAction defaultOutputAction
	Fabric       crossbar
	Algorithm	slip_prime -n 2
	0	bursty  -m 0.1 -u 0.92
	1	bursty  -m 0.1 -u 0.92
	2	bursty  -m 0.1 -u 0.92
	3	bursty  -m 0.1 -u 0.92
	4	bursty  -m 0.1 -u 0.92
	5	bursty  -m 0.1 -u 0.92
	6	bursty  -m 0.1 -u 0.92
	7	bursty  -m 0.1 -u 0.99
	Stats
		Arrivals    (0,0)
		Departures
		Latency     (*,1)
		Occupancy   (1,*)
	Histograms
		Arrivals 
		Departures  (*,m)
		Latency 
		Occupancy   
