./ns3 run scratch/udp_application/example-wifi.cc 

for i in $(seq 0 29); do
    gnuplot $i.plt
done


notify-send "test done"
