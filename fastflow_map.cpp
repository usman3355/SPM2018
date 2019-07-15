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
#include <map>

/*

 */
using namespace ff;
using namespace cimg_library;

struct Image {
    std::string Pho_title;
    int total_parts;
    int starting;
    CImg<unsigned char> *P_Pointer;
    Image(CImg<unsigned char> *P_, int p_no, int S_point, std::string title) : P_Pointer(P_), total_parts(p_no), starting(S_point), Pho_title(title){};
};

CImg<unsigned char> *watermark;
int inserted_delay; //miliseconds
int number_of_workers;
int number_of_photos;
std::string input_directory;
long completion_time=0;
long inter_arival_time=0;
auto completion_time_start = std::chrono::high_resolution_clock::now();
std::vector<Image *> photos;
std::vector<Image *> photos_completed;
std::map<std::string, std::vector<Image *>> results;
long counter_worker=0;
long ideal_service_worker;

/********************************************* Worker  *********************************/
struct Worker: ff_node_t<Image> {
        Image *svc(Image *ii){
        auto start_marking = std::chrono::high_resolution_clock::now();
        if(ii != EOS){
        for(int y = 0; y<ii->total_parts; y++){
        cimg_forX(*(ii->P_Pointer), i){
        int j = y + ii->starting;
        if((*watermark)(i,j) != 255){
        unsigned char red_ = (*(ii->P_Pointer))(i,j,0,0);
        unsigned char green_ = (*(ii->P_Pointer))(i,j,0,1);
        unsigned char blue_ = (*(ii->P_Pointer))(i,j,0,2);
        unsigned char average_color_value = ((0.21*red_) + (0.72*green_) + (0.07*blue_))/3;
        (*(ii->P_Pointer))(i,j,0,0) = (*(ii->P_Pointer))(i,j,0,1) = (*(ii->P_Pointer))(i,j,0,2) = average_color_value;
        }
        }
        }
        if(counter_worker==0)
        {  /*********************************** Ideal Service Time ********************************************/
           auto ideal_worker= std::chrono::high_resolution_clock::now() - start_marking;
           ideal_service_worker=std::chrono::duration_cast<std::chrono::milliseconds>(ideal_worker).count();
           /***************************************************************************************************/
           counter_worker++;
        }
        return ii;
        }
        }
        };
/***************************************************************************************/
/*
 
 */
/********************************************* Scatter  *********************************/
 struct firstStage: ff_node_t<int>{
    int *svc(int *) {
        int counterr=0;
        Image *app;
        auto start_emitting = std::chrono::high_resolution_clock::now();
        completion_time_start=std::chrono::high_resolution_clock::now();
        for(Image *ph : photos){
            int chunk =(ph->P_Pointer->height()/number_of_workers)+1;
            int rest_chunk =(ph->P_Pointer->height()%number_of_workers);
            int chunk_assigned = 0;
            for(int i = 0; i<number_of_workers; i++){
                if(i == rest_chunk)
                    chunk --;
                app = new Image(ph->P_Pointer, chunk, chunk_assigned, ph->Pho_title);
                chunk_assigned += chunk;
                active_delay(inserted_delay);
                ff_send_out(app);
                if(counterr==0)
                {   /********************************************* Inter Arival Time *********************************/
                    auto end_ = std::chrono::high_resolution_clock::now();
                    inter_arival_time+= std::chrono::duration_cast<std::chrono::milliseconds>(end_-start_emitting).count();
                    /*************************************************************************************************/
                    counterr++;

                }
        
            }
        }
        
        return EOS;
    }
} Emitter;

/***************************************************************************************/
/*
 
 */

/********************************************* Collecter  ******************************/
struct lastStage: ff_node_t<Image>{
    Image *svc(Image *ph) {
    results[ph->Pho_title].push_back(ph);
    if(results[ph->Pho_title].size() == number_of_workers){
        photos_completed.push_back(ph);
        }
        return ph;
      }
      void svc_end(){
        /********************************************* Completion Time*********************************/
        auto end_completion = std::chrono::high_resolution_clock::now()-completion_time_start;
        completion_time=std::chrono::duration_cast<std::chrono::milliseconds>(end_completion).count();
        /**********************************************************************************************/
        
        }
    
}Collector;
/***************************************************************************************/


/******************************************** Main *******************************************/
int main(int argc, char *argv[]) {
    if (argc != 6){
        std::cout<< "Input folder path "<< std::endl;
        std::cout<< "Water Mark name"<< std::endl;
        std::cout<< "Number of workers"<< std::endl;
        std::cout<< "Provide some delay"<< std::endl;
        std::cout<< "Enter Number of photos to be Processed"<< std::endl;
        return 0;
    }
/***************************** Input Arguments************************************/    
    std::string input_directory = argv[1];
    std::string watermark_file_name = argv[2];
    number_of_workers = atoi(argv[3]);
    inserted_delay = atoi(argv[4]);
    number_of_photos= atoi(argv[5]);
/*********************************************************************************/   
    if(input_directory.back() != '/')
        input_directory = input_directory+"/";
    watermark = new CImg<unsigned char>((watermark_file_name).c_str());
    int counter=0;
    std::string target_image;
    
/************** Loading from Disk/ Creating coppies in Memmory *******************/
    auto start_loading = std::chrono::high_resolution_clock::now();
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (input_directory.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            std::string file_name = ent->d_name;
            if (file_name.find(".jpg")!= std::string::npos){
                if (counter==0) {
                  target_image=input_directory + file_name;
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
    for(int i = 0 ; i<number_of_photos; i++){
        CImg<unsigned char> app_ph = first_image;
        CImg<unsigned char> *app = new CImg<unsigned char>(app_ph);
        Image *ph = new Image(app, 0,0,std::to_string(i)+".jpg");
        photos.push_back(ph);
    }
    auto end_loading = std::chrono::high_resolution_clock::now();
    auto total_loading = std::chrono::duration_cast<std::chrono::milliseconds>(end_loading - start_loading).count();
    /*********************************************************************************/
    
    /***************************** Parallel Execution ********************************/
    auto start_parallel = std::chrono::high_resolution_clock::now();
    std::vector<std::unique_ptr<ff_node>> Workers;
    for(int i = 0; i<number_of_workers; ++i)
        Workers.push_back(make_unique<Worker>());
    
    ff_Farm<long> farm(std::move(Workers), Emitter, Collector);
    farm.set_scheduling_ondemand();
    if (farm.run_and_wait_end()<0)
        error("running farm");
    
    auto end_parallel = std::chrono::high_resolution_clock::now();
    auto total_parallel = std::chrono::duration_cast<std::chrono::milliseconds>(end_parallel - start_parallel).count();
    /*********************************************************************************/

    /********************************************* Disk Writing***********************/
    /*auto start_disk_writing = std::chrono::high_resolution_clock::now();
    for(Image *q : photos_completed)
        {(*(q->P_Pointer)).save_jpeg((std::string("Photos_marked/marked_")+ (q->Pho_title)).c_str());}
    auto end_disk_writing = std::chrono::high_resolution_clock::now();
    auto total_disk_writing = std::chrono::duration_cast<std::chrono::milliseconds>(end_disk_writing - start_disk_writing).count();
    /*********************************************************************************/
    /********************************************* outputs ***************************/
    std::wcerr <<  "All the measured time values are in Milliseconds"<< "\n";
    std::wcerr <<  "Load Time: " << total_loading << "\n";
    std::wcerr <<  "Completion time: " << total_parallel << "\n";
    //std::wcerr <<  "Interarival Time: " <<inter_arival_time << "\n"; //
    std::wcerr <<  "Ideal service Time: "<<ideal_service_worker<< "\n";
    //std::wcerr <<  "Completion Time: "<<completion_time<< "\n";
    std::wcerr <<  "Photos Processed: " << number_of_photos << "\n";
    std::wcerr <<  "Image dimension: " << watermark->width() << ":" << watermark->height() << "\n";
    /*********************************************************************************/
    return 0;
}

