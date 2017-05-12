# ../sim -l1000
# ../sim -l1000
Numswitches 1
Switch 0
	Numinputs    8
	Numoutputs   8
	PriorityLevels	4
	InputAction  defaultInputAction  
	OutputAction strictPriorityOutputAction
	Fabric       outputQueued
	Algorithm	null
	0	bernoulli_iid_uniform  -p 4 -m 0.1 -f 2 -u 0.8 -r 0.20:0.20:0.20:0.20
	1	bernoulli_iid_uniform  -p 4 -m 0.1 -f 2 -u 0.8 -r 0.20:0.20:0.20:0.20
	2	bernoulli_iid_uniform  -p 4 -m 0.1 -f 2 -u 0.8 -r 0.20:0.20:0.20:0.20
	3	bernoulli_iid_uniform  -p 4 -m 0.1 -f 2 -u 0.8 -r 0.20:0.20:0.20:0.20
	4	bernoulli_iid_uniform  -p 4 -m 0.1 -f 2 -u 0.8 -r 0.20:0.20:0.20:0.20
	5	bernoulli_iid_uniform  -p 4 -m 0.1 -f 2 -u 0.8 -r 0.20:0.20:0.20:0.20
	6	bernoulli_iid_uniform  -p 4 -m 0.1 -f 2 -u 0.8 -r 0.20:0.20:0.20:0.20
	7	bernoulli_iid_uniform  -p 4 -m 0.1 -f 2 -u 0.8 -r 0.20:0.20:0.20:0.20
	Stats
		Arrivals
		Departures
		Latency   
		Occupancy (0,0) 0
	Histograms
		Arrivals 
		Departures
		Latency (0,0)
		Occupancy 
