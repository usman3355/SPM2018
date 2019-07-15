#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include "CImg.h"
#include <fstream>
#include <atomic>
#include <chrono>
#include "util.cpp"
#include <thread>
#include <map>


using namespace cimg_library;
struct Image {
    std::string Pho_title;
    int total_parts;
    int starting;
    CImg<unsigned char> *P_Pointer;
    Image(CImg<unsigned char> *P_, int p_no, int S_point, std::string title) :  P_Pointer(P_), total_parts(p_no), starting(S_point), Pho_title(title){};
};
int number_of_photos;
long total_worker_time=0;
std::string watermark_file_name;
std::string input_directory;
CImg<unsigned char> *watermark;
int inserted_delay;
int number_workers;
long completion_time=0;
long inter_arival_time=0;
auto completion_time_start = std::chrono::high_resolution_clock::now();
long counter_worker=0;
long ideal_service_worker;

std::vector<queue<Image *>> Scatter_to_worker;
std::vector<queue<Image *>> worker_to_collecter;
std::vector<Image *> photos;
std::vector<Image *> images_done;

/********************************************* Scatter  *********************************/
int scatter(){
    int counter=0;
    auto start_scatter = std::chrono::high_resolution_clock::now();
    Image *point;
    completion_time_start=std::chrono::high_resolution_clock::now();
    for(Image *ii : photos){
        int partitions =(ii->P_Pointer->height()/number_workers)+1;
        int portion_left =(ii->P_Pointer->height()%number_workers);
        int allocation = 0;
        for(int i = 0; i<number_workers; i++){
            if(i == portion_left)
                partitions --;
            point = new Image(ii->P_Pointer, partitions, allocation, ii->Pho_title);
            allocation += partitions;
            active_delay(inserted_delay);
            Scatter_to_worker[i].push(point);
            if(counter==0)
            {   /******************************************* Interarival Time*********************************************/
                auto end_sca = std::chrono::high_resolution_clock::now();
                inter_arival_time+= std::chrono::duration_cast<std::chrono::milliseconds>(end_sca - start_scatter).count();
                counter++;
                /*********************************************************************************************************/
            }


        }
    }
    
    for(int i = 0; i< number_workers; i++)
        {active_delay(inserted_delay);
        Scatter_to_worker[i].push(EOS);}
    

    return 0;
}
/***************************************************************************************/

/********************************************* Worker  *********************************/
int worker(int id){
    
    while(true){
        auto start_stamp = std::chrono::high_resolution_clock::now();
        Image *ii = Scatter_to_worker[id].pop();
        if(ii != EOS){
            auto start = std::chrono::high_resolution_clock::now();
            for(int y = 0; y<ii->total_parts; y++){ //here every value of y denotes number of colum of the matrix
                cimg_forX(*(ii->P_Pointer), i){ //every value of i represents row
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
            
            worker_to_collecter[id].push(ii);
            if(counter_worker==0)
            {   /*********************************** Ideal Service Time ***************************************************/
                auto end_stamp = std::chrono::high_resolution_clock::now() - start_stamp;
                ideal_service_worker= std::chrono::duration_cast<std::chrono::milliseconds>(end_stamp).count();
                counter_worker++;
                /*********************************************************************************************************/
            }
        }
        else{
            worker_to_collecter[id].push(EOS);
            return 0;
        }
        
    }
}
/***************************************************************************************/

/********************************************* Collecter  ******************************/
int collector(){
    auto start_collector = std::chrono::high_resolution_clock::now();
    int counter = 0;
    std::map<std::string, std::vector<Image *>> results;     //since we are using pointers to identify to work on images we dont care about the order and syncing 
    while(counter < number_workers){
        for(int i = 0; i<worker_to_collecter.size(); i++){
            if(!worker_to_collecter[i].isEmpty()){
                Image * ii = worker_to_collecter[i].pop();
                if(ii!= EOS){
                    results[ii->Pho_title].push_back(ii);
                    if(results[ii->Pho_title].size() == number_workers)
                        images_done.push_back(ii);
                }
                else
                    counter ++;
            }
        }
    }
    /********************************************* Completion Time ********************************************/
    auto end_collector = std::chrono::high_resolution_clock::now();
    completion_time= std::chrono::duration_cast<std::chrono::milliseconds>(end_collector - completion_time_start).count();
    /**********************************************************************************************************/
    
    return 0;
}
/******************************************** Main *******************************************/
int main(int argc, char * argv[]) {
    
    if (argc != 6){
        std::cout<< "Input folder path "<< std::endl;
        std::cout<< "Water Mark name"<< std::endl;
        std::cout<< "Number of workers"<< std::endl;
        std::cout<< "Provide some delay"<< std::endl;
        std::cout<< "Enter Number of photos to be Processed"<< std::endl;
        

        return 0;
    }
    /***************************** Input Arguments************************************/
    input_directory = argv[1];
    watermark_file_name = argv[2];
    number_workers = atoi(argv[3]);
    inserted_delay = atoi(argv[4]);
    number_of_photos = atoi(argv[5]);
    /*********************************************************************************/
    
    if(input_directory.back() != '/')
        input_directory = input_directory+"/";
    
    Scatter_to_worker = std::vector<queue<Image *>>(number_workers);
    worker_to_collecter  = std::vector<queue<Image *>>(number_workers);
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
        CImg<unsigned char> copy = first_image;
        CImg<unsigned char> *copy_pointer = new CImg<unsigned char>(copy);
        Image *ii = new Image(copy_pointer,0,0, std::to_string(i)+".jpg");
        photos.push_back(ii);
    }

    auto end_loading = std::chrono::high_resolution_clock::now();
    auto total_loading = std::chrono::duration_cast<std::chrono::milliseconds>(end_loading - start_loading).count();

    /*********************************************************************************/

    /***************************** Parallel Execution ********************************/
    auto start_parallel = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> threads;
    threads.push_back(std::thread(scatter));
    
    for(int i = 0; i < number_workers; i++){
        threads.push_back(std::thread(worker, i));
    }
    
    threads.push_back(std::thread(collector));
    for (int i=0; i < threads.size(); i++)
        threads[i].join();
    auto end_parallel = std::chrono::high_resolution_clock::now();
    auto total_parallel = std::chrono::duration_cast<std::chrono::milliseconds>(end_parallel - start_parallel).count();
    /***************************** Parallel Execution ********************************/
    /********************************************* Disk Writing***********************/
    /*
    auto start_disk_writing = std::chrono::high_resolution_clock::now();
    for(Image *ii : images_done)
      {(*(ii->P_Pointer)).save_jpeg((std::string("Photos_marked/marked_")+ (ii->Pho_title)).c_str());}
    auto end_disk_writing = std::chrono::high_resolution_clock::now();
    auto total_disk_writing = std::chrono::duration_cast<std::chrono::milliseconds>(end_disk_writing - start_disk_writing).count();
    /*********************************************************************************/
    /********************************************* outputs ***************************/
     std::wcerr <<  "Number of workers: " << number_workers << "\n";
     std::wcerr <<  "Load Time: " << total_loading << "\n";
     std::wcerr <<  "Completion Time: " << completion_time << "\n";
     std::wcerr <<  "Ideal Service Time: " << ideal_service_worker << "\n";
     //std::wcerr <<  "Completion Time: " << completion_time<< "\n";
     std::wcerr <<  "Photos Processed: " << number_of_photos << "\n";
     std::wcerr <<  "Images dimensions: " << watermark->width() <<" : "<< watermark->height() << "\n";
    // std::wcerr <<  "Total disk writing time: " << total_disk_writing << "\n";
    /*********************************************************************************/
      return 0;
}


