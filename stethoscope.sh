$password = odroid
echo $password | sudo apt-get update
echo $password y| sudo apt-get install -y jackd2 libjack-jackd2-0 libjack-jackd2-dev libsndfile1-dev
gcc -o simple simple_client.c `pkg-config --cflags --libs jack` -lsndfile -lpthrea
sleep 2 
(echo $PASSWORD | sudo -S jackd -d alsa -d hw:1 -S true -r 44100 &)  
sleep 2
(echo $PASSWORD | sudo ./uploader &)
(echo $PASSWORD | sudo ./isr &) 