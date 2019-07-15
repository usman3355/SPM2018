# Final Project: Distributed systems: paradigms and models


Context The problem is to apply a "marker" of the same dimension onto the images provided by the user.To achieve this, I use the CIMG library to process the images.

`Aims` 
The project is to provide both a sequential and parallel implementation of the problem using the pthread version and the fastflow library (https://github.com/fastflow) and to compare the cost and performance models.

`Compile`

g++ sequential.cpp -o seq -std=c++11 -O3 -lm -pg -pthread -L/usr/X11R6/lib -ljpeg -lX11
g++ -o test farm.cpp -lpthread -lX11 -I/mnt/c/Users/usman/Desktop/imgspm

`RUN` 
./test input-folder-name water_mark_file_name_.png numbr_workers Delay number_images
