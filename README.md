# Parallel Image Processing 
`Aims` 
Context ofThe problem is to apply a "marker/watermark" images provided by the user by using parallel Programing paradigms.
For image processing CIMG image processing library is used. the goal is to produce optimized and scalable code to address the given problem. 
Implemented Processing Models:
        1) Sequential Implementation
        2) Farm based Parallel implentation using STD Pthread
        3) Map Data Prallel Implementation using STD Pthread
        4) Farm based Parallel implentation using fastflow
        5) Map Data Prallel Implementation using STD fastflow
        
`Observations`
the implemented code is deployed and tested on 164 core intel server and good scalability and speedup is observed.

`fastflow`
 :(https://github.com/fastflow) 

`Compile`

g++ sequential.cpp -o seq -std=c++11 -O3 -lm -pg -pthread -L/usr/X11R6/lib -ljpeg -lX11
g++ -o test farm.cpp -lpthread -lX11 -I/mnt/c/Users/usman/Desktop/imgspm

`RUN` 
./test input-folder-name water_mark_file_name_.png numbr_workers Delay number_images
