#include <iostream>
#include <vector>
#include <ff/pipeline.hpp>
#include <ff/farm.hpp>
#include <string>
#include "CImg.h"
#include <fstream>
#include <atomic>
#include <ctime>
#include <algorithm>
#include "util.cpp"


using namespace ff;
using namespace cimg_library;


/*
 */
struct Image {
    CImg<unsigned char> *P_pointer;
    std::string title;
    Image(CImg<unsigned char> *P_, std::string title) : P_pointer(P_), title(title){};
};

CImg<unsigned char> *watermark;
int print_out = 0;
long inter_arival_time=0;
long total_emitter_time = 0;
long total_collector_time = 0;
long ideal_service_time_worker;
long counter_worker=0;
long Total_parallel;
std::string input_directory;
std::string watermark_file_name;
int number_of_Photos;
int inerted_delay;
int number_of_workers;
int counter=0;
long completion_time;
long emitter_overhead=0;
auto Completion_time_start = std::chrono::high_resolution_clock::now();
std::vector<Image *> photos;
std::vector<Image *> images_done;
/******************************************** Worker *******************************************/
struct Worker: ff_node_t<Image> {
    Image *svc(Image *q){
        auto start_marking = std::chrono::high_resolution_clock::now();
        cimg_forXY(*(q->P_pointer), x, y){
            if((*watermark)(x,y) != 255){
                unsigned char valR = (*(q->P_pointer))(x,y,0,0);
                unsigned char valG = (*(q->P_pointer))(x,y,0,1);
                unsigned char valB = (*(q->P_pointer))(x,y,0,2);
                unsigned char average_color_value = ((0.21*valR) + (0.72*valG) + (0.07*valB))/3;
                (*(q->P_pointer))(x,y,0,0) = (*(q->P_pointer))(x,y,0,1) = (*(q->P_pointer))(x,y,0,2) = average_color_value;
            }
        }
        auto end_marking = std::chrono::high_resolution_clock::now();
        if(counter_worker==0)
        {   /********************************************* Ideal Service Time********************************************/
            ideal_service_time_worker+=std::chrono::duration_cast<std::chrono::milliseconds>(end_marking - start_marking).count();
            counter_worker++;
            /************************************************************************************************************/
        }
        

        return q;
    }
    };

/*
 The emitter takes the elements from the "photos" vector and sends them to the workers.
 */
/******************************************** Emitter *******************************************/
struct firstStage: ff_node_t<int> {
    int *svc(int *) {
        int counterr=0;
        Completion_time_start = std::chrono::high_resolution_clock::now();
        auto start_emitting = std::chrono::high_resolution_clock::now();
        for(int i = 0; i<photos.size(); i++){
            active_delay(inerted_delay);
            ff_send_out(photos[i]);
                if(counterr==0)
                {   /********************************************* Inter arival Time ********************************************/
                    auto end_ = std::chrono::high_resolution_clock::now();
                    inter_arival_time= std::chrono::duration_cast<std::chrono::milliseconds>(end_-start_emitting).count();
                    counterr++;
                    /************************************************************************************************************/

                }
            
        }
        
        return EOS;
    }
} Emitter;


/*
 The collector tries receives the elements from the workers and puts them inside the "photo_completed" vector.
 */

/******************************************** Collector *******************************************/
struct lastStage: ff_node_t<Image>{
    Image *svc(Image *ph) {
    auto start_collecting = std::chrono::high_resolution_clock::now();
    images_done.push_back(ph);
    auto end_collecting = std::chrono::high_resolution_clock::now();
    total_collector_time += std::chrono::duration_cast<std::chrono::milliseconds>(end_collecting-start_collecting).count();
    
        return ph;
    }
    void svc_end(){
        auto end_completion = std::chrono::high_resolution_clock::now()-Completion_time_start;
        completion_time=std::chrono::duration_cast<std::chrono::milliseconds>(end_completion).count();
        
        }
    }Collector;


/******************************************** Main *******************************************/
int main(int argc, char *argv[]) {
    
    if (argc != 6){
        std::cout<< "input directory name" << std::endl;
        std::cout<< "watermark file name" << std::endl;
        std::cout<< "number of workers" << std::endl;
        std::cout<< "number of photos to be processed" << std::endl;
        std::cout<< "Active Delay" << std::endl;
    return 0;
    }
    /***************************** Input Arguments************************************/
    input_directory = argv[1];
    watermark_file_name = argv[2];
    number_of_workers = atoi(argv[3]);
    inerted_delay = atoi(argv[4]);
    number_of_Photos = atoi(argv[5]);
    /*********************************************************************************/
    
    if(input_directory.back() != '/')
        input_directory = input_directory+"/";
    
    watermark = new CImg<unsigned char>((watermark_file_name).c_str());
    std::string target_image;
    /************** Loading from Disk/ Creating coppies in Memmory *******************/
    auto start_loading = std::chrono::high_resolution_clock::now();
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (input_directory.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            std::string file_found = ent->d_name;
            if (file_found.find(".jpg")!= std::string::npos){
               if (counter==0) {
                  target_image=input_directory + file_found;
                  counter++;
               }
               else
               {
                   break;
               }
            }
        }
        
        closedir (dir);
    } else {
        perror ("");
        return EXIT_FAILURE;
    }
    
    
    CImg<unsigned char> first_image (target_image.c_str());
    for(int i = 0 ; i<number_of_Photos; i++){
        CImg<unsigned char> app_ph = first_image;
        CImg<unsigned char> *app = new CImg<unsigned char>(app_ph);
        Image *ph = new Image(app, std::to_string(i)+".jpg");
        photos.push_back(ph);
    }
    auto end_loading = std::chrono::high_resolution_clock::now();
    auto total_loading = std::chrono::duration_cast<std::chrono::milliseconds>(end_loading - start_loading).count();
    /****************************************************************************************/
    /********************************************* Parallel Execution ***********************/
    auto start_parallel = std::chrono::high_resolution_clock::now();
    
    std::vector<std::unique_ptr<ff_node>> workers;
    for(int i = 0; i<number_of_workers; ++i)
        workers.push_back(make_unique<Worker>());
    
    ff_Farm<long> farm(std::move(workers), Emitter, Collector);
    farm.set_scheduling_ondemand();
    if (farm.run_and_wait_end()<0)
        error("running farm");
    
    auto end_parallel = std::chrono::high_resolution_clock::now();
    auto Time_elapsed = end_parallel - start_parallel;
    Total_parallel = std::chrono::duration_cast<std::chrono::milliseconds>(Time_elapsed).count();

    /********************************************* Disk Writing***********************/
    /*auto start_disk_writing = std::chrono::high_resolution_clock::now();
    for(Image *ii : images_done)
      {(*(ii->P_pointer)).save_jpeg((std::string("Photos_marked/marked_")+ (ii->title)).c_str());}
    auto end_disk_writing = std::chrono::high_resolution_clock::now();
    auto total_disk_writing = std::chrono::duration_cast<std::chrono::milliseconds>(end_disk_writing - start_disk_writing).count();
    /*********************************************************************************/
    /********************************************* outputs ***************************/
    std::wcerr <<  "All the measured time values are in Milliseconds"<< "\n";
    std::wcerr <<  "Number of Workers: " <<number_of_workers<< "\n";
    std::wcerr <<  "Total load time/loading input image + creating copies in memmory: " << total_loading << "\n";
    std::wcerr <<  "completion time: " << Total_parallel << "\n";
    std::wcerr <<  "Ideal Serive Time: "<<ideal_service_time_worker<<"\n";
    //std::wcerr <<  "Completion Time: " <<completion_time<<"\n";
    //std::wcerr <<  "Total disk writing time: " << total_disk_writing << "\n";
    std::wcerr <<  "Number of images: " <<number_of_Photos<< "\n";
    std::wcerr <<  "Image Dimensions: " << watermark->width() << " : " << watermark->height() << "\n";
    /*********************************************************************************/
    return 0;
}