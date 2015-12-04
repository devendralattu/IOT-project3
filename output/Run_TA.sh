gnome-terminal -e "bash -c \"./<database_executable_file> <Backend_Config_file> PersistentStorage.txt; exec bash\""
sleep 4
gnome-terminal -e "bash -c \"./<gateway_executable_file> <Gateway_Config_file> GatewayOutput.log; exec bash\""
sleep 4
gnome-terminal -e "bash -c \"./<door_executable_file> <Door_Config_file> DoorInput.txt DoorOutput.log; exec bash\""
sleep 4
gnome-terminal -e "bash -c \"./<keychain_executable_file> <Keychain_Config_file> KeychainInput.txt KeychainOutput.log; exec bash\""
sleep 4
gnome-terminal -e "bash -c \"./<motion_executable_file> <Motion_Config_file> MotionInput.txt MotionOutput.log; exec bash\""
sleep 4
gnome-terminal -e "bash -c \"./<securitysystem_executable_file> <SecurityDevice_Config_file> SecurityDeviceOutput.log; exec bash\""
