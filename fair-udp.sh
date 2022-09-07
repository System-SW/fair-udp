./ns3 run scratch/udp_application/example-wifi.cc 

for i in $(seq 30); do
    gnuplot node$i.plt
done


notify-send "test done"
