#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include "CImg.h"
#include <fstream>
#include <atomic>
#include <chrono>
#include "util.cpp"
#include <ctime>
#include <thread>
using namespace cimg_library;

struct Image
{
    std::string Photo_title;
    CImg<unsigned char> *P_pointer;
    Image(CImg<unsigned char> *image, std::string title) : P_pointer(image), Photo_title(title){};
};
std::string watermark_file_name;
std::string input_directory;
int number_of_Photos;
int number_of_workers;
long completion_time = 0;
int counter_emitter = 0;
long total_parallel;
long start_completion;
auto tCompletionStart = std::chrono::high_resolution_clock::now();
auto tparallel_start = std::chrono::high_resolution_clock::now();
auto tparallel_end= std::chrono::high_resolution_clock::now();
/************************** Atomic Variables *********************************/
int counter_worker = 0;
long ideal_service_time_worker;
/*****************************************************************************/
int inserted_delay;          // time to slow down the emitter// delay inserted to remove the bottle neck
std::vector<Image *> photos; //vector to store pointers of loaded images
std::vector<Image *> Photos_Done;
std::vector<long> stamp_times; //vector containing stamp time of every photo// inserted by worker working on it
std::vector<long> computation_times;
queue<Image *> original_photos; //contains photos to be taken by the workers
queue<Image *> marked_photos;   //contains photos marked by worker
CImg<unsigned char> *watermark;


/********************************************* Worker *********************************/
int Worker_Farm()
{
     
    while (true)
    {Image *q = original_photos.pop();
     if (q != EOS)
        {   counter_worker++;
            auto start = std::chrono::high_resolution_clock::now();
         cimg_forXY(*(q->P_pointer), x, y)
            {
                if ((*watermark)(x, y) != 255)
                {
                    unsigned char valR = (*(q->P_pointer))(x, y, 0, 0);
                    unsigned char valG = (*(q->P_pointer))(x, y, 0, 1);
                    unsigned char valB = (*(q->P_pointer))(x, y, 0, 2);
                    unsigned char avgerage_color_value = ((0.21 * valR) + (0.72 * valG) + (0.07 * valB)) / 3;
                    (*(q->P_pointer))(x, y, 0, 0) = (*(q->P_pointer))(x, y, 0, 1) = (*(q->P_pointer))(x, y, 0, 2) = avgerage_color_value;
                }
            }
            marked_photos.push(q);
            if (counter_worker == 1)
            {
                /*************************************** Ideal Service Time ************************************************/
                auto end_stamp = std::chrono::high_resolution_clock::now() - start;
                ideal_service_time_worker = std::chrono::duration_cast<std::chrono::milliseconds>(end_stamp).count();
                counter_worker++;
                /**********************************************************************************************************/
            }
            
        }
        
        else
        {
            marked_photos.push(EOS);
            return 0;
        }
    }
}
/***************************************************************************************/

/********************************************* Emitter *********************************/
int Emitter_Farm()
{       auto start_Emitter=std::chrono::high_resolution_clock::now();
    for (Image *point : photos)
    {
        if (counter_emitter == 0)
        {
            tCompletionStart  = std::chrono::high_resolution_clock::now();
            tparallel_start = std::chrono::high_resolution_clock::now();
            counter_emitter++;
        }
        active_delay(inserted_delay); //this inserted delay is considered as interarival time for a task to worker
        original_photos.push(point);  // pushing photos to serve to workers
     }
    for (int i = 0; i < number_of_workers; i++) // pushing as number of EOS as the numbers of workers 1 for every worker
        {//active_delay(inserted_delay);
        original_photos.push(EOS);
        }


    return 0;
}
/***************************************************************************************/

/********************************************* Collector ********************************/
int Collector_Farm()
{   int first_end_of_stream=0;
    int count = 0;
    while (count < number_of_workers)
    {
        Image *point = marked_photos.pop();
        if (point == EOS) //check to ensure that
            {   first_end_of_stream++;
                count++;
                if (first_end_of_stream==1)
                    {   /************************************* Parallel Time************************************************/
                        tparallel_end = std::chrono::high_resolution_clock::now();
                        total_parallel=std::chrono::duration_cast<std::chrono::milliseconds>(tparallel_end - tparallel_start).count();
                        /**************************************************************************************************/
                    }
            }
        else
            Photos_Done.push_back(point);
    }
    /****************************************** Comletion Time********************************************/
    auto end_collection = std::chrono::high_resolution_clock::now();
    completion_time=std::chrono::duration_cast<std::chrono::milliseconds>(end_collection - tCompletionStart).count();
    /*****************************************************************************************************/
    return 0;
}
/***************************************************************************************/
int main(int argc, char *argv[])
{

    if (argc != 6)
    {
        std::cout << "Input folder path " << std::endl;
        std::cout << "Water Mark name" << std::endl;
        std::cout << "Number of workers" << std::endl;
        std::cout << "Provide some delay" << std::endl;
        std::cout << "Enter Number of photos to be Processed" << std::endl;

        return 0;
    }
    /***************************** Input Arguments************************************/
    input_directory = argv[1];
    watermark_file_name = argv[2];
    number_of_workers = atoi(argv[3]);
    inserted_delay = atoi(argv[4]);
    number_of_Photos = atoi(argv[5]);
    /*********************************************************************************/
    if (input_directory.back() != '/')
        input_directory = input_directory + "/";

    watermark = new CImg<unsigned char>((watermark_file_name).c_str());
    std::string target_image;

    /********************************************* Loading from Disk/ Creating coppies in Memmory *********************************/
    auto start_loading = std::chrono::high_resolution_clock::now();
    DIR *dir;
    int counter = 0;
    struct dirent *ent;
    if ((dir = opendir(input_directory.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            std::string file_name = ent->d_name;
            if (file_name.find(".jpg") != std::string::npos)
            {
                if (counter == 0)
                {
                    target_image = input_directory + file_name;
                    counter++;
                }
                else
                {
                    break;
                }
            }
        }

        closedir(dir);
    }
    else
    {
        perror("");
        return EXIT_FAILURE;
    }

    CImg<unsigned char> sample_photo(target_image.c_str());

    for (int i = 0; i < number_of_Photos; i++)
    {
        CImg<unsigned char> app_ph = sample_photo;
        CImg<unsigned char> *app = new CImg<unsigned char>(app_ph);
        Image *ii = new Image(app, std::to_string(i) + ".jpg");
        photos.push_back(ii);
    }
    auto end_loading = std::chrono::high_resolution_clock::now();
    auto total_loading_coppies = std::chrono::duration_cast<std::chrono::milliseconds>(end_loading - start_loading).count();
    /*******************************************************************************************************************************/
    auto start_total_completion = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> threads;
    threads.push_back(std::thread(Emitter_Farm));

    for (int i = 0; i < number_of_workers; i++)
    {
        threads.push_back(std::thread(Worker_Farm));
    }

    threads.push_back(std::thread(Collector_Farm));
    for (int i = 0; i < threads.size(); i++)
        {threads[i].join();
                    }
    auto end_completion = std::chrono::high_resolution_clock::now();
    auto complete = std::chrono::duration_cast<std::chrono::milliseconds>(end_completion - start_total_completion).count();
    /********************************************* Disk Writing*********************************/
    /*auto start_writing = std::chrono::high_resolution_clock::now();
    for(Image *Point : Photos_Done)
       { (*(Point->P_pointer)).save_jpeg((std::string("Photos_marked/marked_")+ (Point->Photo_title)).c_str());}
    auto end_writing = std::chrono::high_resolution_clock::now();
    auto total_disk_writing = std::chrono::duration_cast<std::chrono::milliseconds>(end_writing - start_writing).count();
    /******************************************************************************************/
/******************************************Outputs ****************************************/
    std::wcerr << "All the measured time values are in Milliseconds"<< "\n";
    //std::wcerr << "sample time: " << complete << "\n";
    std::wcerr << "Number of workers: " << number_of_workers << "\n";
    std::wcerr << "Total loading/coppies/Time: " << total_loading_coppies << "\n";
    std::wcerr << "Parallel Time: " << total_parallel << "\n";
    std::wcerr << "Interarival Time: " << inserted_delay << "\n";
    std::wcerr << "Ideal Serivce Time: " << ideal_service_time_worker << "\n";
    std::wcerr << "Completion Time: " << completion_time << "\n";
    //std::wcerr <<  "Total disk writing time: " << total_disk_writing << "\n";
    std::wcerr << "Number of images Processed: " << number_of_Photos << "\n";
    std::wcerr << "Images dimensions: " << watermark->width() << " : " << watermark->height() << "\n";
    /******************************************************************************************/
    
    return 0;
}
