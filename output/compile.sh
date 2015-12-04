gcc -g -o ../bo ../backend.c
gcc -g -o ../go ../gateway.c -lpthread
gcc -g -o ../mo ../motion.c -lpthread
gcc -g -o ../ko ../keychain.c -lpthread
gcc -g -o ../do ../door.c -lpthread
gcc -g -o ../so ../security.c -lpthread

#clean output files

>GatewayOutput.log
>DoorOutput.log
>KeychainOutput.log
>MotionOutput.log
>PersistentStorage.txt
>SecurityDeviceOutput.log
