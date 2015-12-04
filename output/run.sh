gnome-terminal -e "bash -c \".././bo ../BackendConfiguration.txt PersistentStorage.txt; exec bash\""
sleep 1
gnome-terminal -e "bash -c \".././go ../GatewayConfiguration.txt GatewayOutput.log; exec bash\""
sleep 1
gnome-terminal -e "bash -c \".././do ../DoorConfiguration.txt DoorInput.txt DoorOutput.log; exec bash\""
sleep 1
gnome-terminal -e "bash -c \".././ko ../KeychainConfiguration.txt KeychainInput.txt KeychainOutput.log; exec bash\""
sleep 1
gnome-terminal -e "bash -c \".././mo ../MotionSensorConfiguration.txt MotionInput.txt MotionOutput.log; exec bash\""
sleep 1
gnome-terminal -e "bash -c \".././so ../SecurityConfiguration.txt SecurityDeviceOutput.log; exec bash\""
