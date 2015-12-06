gcc -g -o ../bo1 ../backend1.c -lpthread
gcc -g -o ../bo2 ../backend2.c -lpthread
gcc -g -o ../go1 ../gateway1.c -lpthread
gcc -g -o ../go2 ../gateway2.c -lpthread
gcc -g -o ../mo ../motion.c -lpthread
gcc -g -o ../ko ../keychain.c -lpthread
gcc -g -o ../do ../door.c -lpthread
gcc -g -o ../so ../security.c -lpthread

#clean output files

>GatewayOutput1.log
>GatewayOutput2.log
>DoorOutput.log
>KeychainOutput.log
>MotionOutput.log
>PersistentStorage1.txt
>PersistentStorage2.txt
>SecurityDeviceOutput.log
