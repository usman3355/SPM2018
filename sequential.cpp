#include <iostream>
#include <vector>
#include <ctime>
#include "CImg.h"
#include <fstream>
#include <chrono>
#include <string>
#include <atomic>
#include <chrono>
#include "util.cpp"

using namespace cimg_library;

struct Image {
    CImg<unsigned char> *P_pointer;
    std::string title;
    Image(CImg<unsigned char> *P_, std::string title) : P_pointer(P_), title(title){};
};
std::string input_directory;
std::string watermark_file_name;
int number_of_photos;
std::vector<Image *> Images_Processed;
long ideal_service_time=0;
long inter_arival_time=0;
long completion_time=0;
/********************************************* Main ***********************************************/
int main(int argc, char * argv[]) {
    
    if (argc != 4){
        std::cout<< "Input folder path "<< std::endl;
        std::cout<< "Water Mark name"<< std::endl;
        std::cout<< "Enter Number of photos to be Processed"<< std::endl;
        return 0;
    }
    std::string target_image;
    int counter=0;
    /********************************************* Input Arguments *********************************/
    input_directory = argv[1]; 
    watermark_file_name = argv[2];
    number_of_photos= atoi(argv[3]);
    
    if(input_directory.back() != '/')
        input_directory = input_directory+"/";
    
    /**************** Loading watermark from Disk to Memmory****************************************/
    CImg<unsigned char> *watermark = new CImg<unsigned char>((watermark_file_name).c_str());
    /****************************************************************************************/
    
/************************************ Sequential Execution ******************************************/
auto overall_start = std::chrono::high_resolution_clock::now();
auto load_start = std::chrono::high_resolution_clock::now();
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (input_directory.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            std::string file_name = ent->d_name;
            
            if (file_name.find(".jpg")!= std::string::npos){
                std::string file_name =  ent->d_name;
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
    /***************** Reading single Image from Disk and creating N coppies******************/
    CImg<unsigned char> first_image (target_image.c_str());
    for(int i = 0 ; i<number_of_photos; i++){
        CImg<unsigned char> *first_pointer = new CImg<unsigned char>(first_image);
        Image *point = new Image(first_pointer, std::to_string(i)+".jpg");
        Images_Processed.push_back(point);
    }
    auto load_end = std::chrono::high_resolution_clock::now();
    auto loading_time = std::chrono::duration_cast<std::chrono::milliseconds>(load_end - load_start).count();
    /****************************************************************************************/
    
    /********************************************* Marking *********************************/
    auto start_stamp = std::chrono::high_resolution_clock::now();
    int count = 0;
    int counterr=0;
    for(Image *p : Images_Processed){   
        if(counterr==0)
        {auto end_interarival = std::chrono::high_resolution_clock::now()-start_stamp;
        inter_arival_time+= std::chrono::duration_cast<std::chrono::milliseconds>(end_interarival).count();
        counterr++;
        }   
        cimg_forXY(*(p->P_pointer), x, y){
            if((*watermark)(x,y) != 255 ){
                unsigned char valR = (*(p->P_pointer))(x,y,0,0);
                unsigned char valG = (*(p->P_pointer))(x,y,0,1);
                unsigned char valB = (*(p->P_pointer))(x,y,0,2);
                unsigned char Average_color_value = ((0.21*valR) + (0.72*valG) + (0.07*valB))/3;
                (*(p->P_pointer))(x,y,0,0) = (*(p->P_pointer))(x,y,0,1) = (*(p->P_pointer))(x,y,0,2) = Average_color_value;
            }
        }
         if (count==0) 
         {
           auto ideal_end= std::chrono::high_resolution_clock::now()-start_stamp;
           ideal_service_time+= std::chrono::duration_cast<std::chrono::milliseconds>(ideal_end).count();
           count++;
         }
        
    }
    auto End_stamp = std::chrono::high_resolution_clock::now();
    completion_time+= std::chrono::duration_cast<std::chrono::milliseconds>(End_stamp - start_stamp).count();
    
    
    /********************************************* Disk Writing*********************************/
    /*auto Start_write = std::chrono::high_resolution_clock::now();
    for(int i = 0; i<Images_Processed.size(); i++)
        (*(Images_Processed[i]->P_pointer)).save_jpeg((std::string("Photos_marked/marked_")+  (Images_Processed[i]->title)).c_str());
    auto End_write = std::chrono::high_resolution_clock::now();
    auto total_write_time = std::chrono::duration_cast<std::chrono::milliseconds>(End_write - Start_write).count();
    /******************************************************************************************/
    
    
    auto overall_end = std::chrono::high_resolution_clock::now();
    auto toal_time_consumed = std::chrono::duration_cast<std::chrono::milliseconds>(overall_end - overall_start).count();
    
    
    /********************************************* outputs *********************************/
    std::wcerr <<  "Loading Time: " << loading_time << "\n"; //
    std::wcerr <<  "Inter arival time: " << inter_arival_time << "\n";
    std::wcerr <<  "service Time: " << ideal_service_time << "\n";//
    std::wcerr <<  "Sequential Time: " <<ideal_service_time*number_of_photos << "\n";//
    std::wcerr <<  "Completion Time: " << completion_time << "\n";//
    //std::wcerr <<  "Disk saving Time: " << total_write_time << "\n";
    std::wcerr <<  "Total images Processed: " <<number_of_photos << "\n";
    std::wcerr <<  "Images dimensions: " << watermark->width() << " x " << watermark->height() << "\n";
    /**************************************************************************************/
    return 0;
}

