./ns3 run scratch/udp_application/example-wifi.cc 

uav_num=$(grep -Po "(?<=(UAV_NUM) = )([0-9])+" ./scratch/udp_application/config.h)
echo UAV_NUM : $uav_num

for i in $(seq 0 $uav_num); do
    gnuplot $i.plt
done

notify-send "test done" -u critical
