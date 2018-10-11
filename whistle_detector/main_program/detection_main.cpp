//--------------------------------------------------------------------------
//Author:   Yen-Hsiang Huang (YHHuang)
//Origin:   National Taiwan University
//Date:     Aug 17th, 2018
//--------------------------------------------------------------------------

// This code can analysis the sound data to check if there is whistle or not 
// STFT -> median filter -> edge detector -> moving time and frequency square

#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <time.h>

#include "readfile.h"
#include "STFT_header/detection_algorithm.h"
// Remember to compile STFT.cpp as well

using namespace std;

void saveData(vector<vector<float> > ,int , int , float);

int main(int argc, char* argv[]){

// declare the constant 
    const float pi = 3.1415926;
//---------------------------------------------------------
//Read Data to form x (using file only contain voltage data)
//User can input the data by command line, default is 9k_20k_3s
//---------------------------------------------------------
    vector<float>   input;
                    input.clear();
    
//----------------------------------------------------------
//  setup the parameters: 
//
//  fs:                 sample rate 
//  N:  window length:  must be 8/375 seconds
//  do_detect:          do detection for whistle or not 
//  window_type:        0 is rectangular and 1 is Hanning window
//  channel:            mono or stereo
//  SNR_threshold:      SNR threshold for edge detector 
//  frq_low:            lower frequency for band-pass filter 
//  frq_high:           higher frequency fpr band-pass filter 
//  overlap:            overlap for STFT, must be 0.9
//----------------------------------------------------------
   
        int     fs              = 96000;     // sample rate
        int     N               = round((float)fs/46.875);
        bool    do_detect       = true;      
        int     window_type     = 1;         
        int     channel         = 1;
        float   SNR_threshold   = 10;
        float   frq_low         = 3000;
        float   frq_high        = 10000;
        float   overlap         = 0.9;
//----------------------------------------------------------

    if(argc!=4){
      cout<<"Usage: ./GaborSTFT_using_function filename do_detect channel_number"<<endl;
      cout<<"Calculate chirp with noise and do detect for default"<<endl;  
      
      input = readfile("chirp_with_noise.txt");
    }
    else{
      cout<<"Calculate for "<<argv[1]<<endl;
      input = readfile(argv[1]);
      do_detect = atof(argv[2]);
      cout<<"Do detect="<<argv[2]<<endl;
      channel = atof(argv[3]);
      cout<<"Channel number = "<<argv[3]<<endl;
    }
    

    int input_size;

    if(channel == 1)
        input_size = input.size();
    else 
        input_size = round(input.size()/2);

    vector<float> input_x(input_size,0);

    
//mono
    if(channel==1){
        for(int i=0;i<input.size();i++)
            input_x[i]= input[i];
    }
//stereo    
    else{
        int j=0;
        for(int i=0;i<input.size();i+=2){
            input_x[j] = input[i];
            j++;
        }
    }

//time start
clock_t t_one;
t_one =clock();

clock_t t_stft;
t_stft = clock();

        vector<whistle> whistle_list;
        vector<vector<float> > P   = spectrogram_yhh(input_x,fs,N,overlap,window_type);   
// P: row is frequency and column is time: x time and y frequency

//time for STFT
t_stft = (clock()-t_stft);
float STFT_time = (float)t_stft/CLOCKS_PER_SEC;
cout<<"\nSTFT time:"<<STFT_time<<" seconds"<<endl;


//time start for detection 
clock_t t_two;
t_two =clock();


//put detection algorithm in
      if(do_detect){
        detect_whistle(P,fs,N,overlap,SNR_threshold,frq_low,frq_high); 
        whistle_list = check_result(P,fs,N,overlap);        
      }

// detection time end
t_two = (clock()-t_two);
float detection_time = (float)t_two/CLOCKS_PER_SEC;
cout<<"\nDetection time:"<<detection_time<<" seconds"<<endl;


//total time end
t_one = (clock()-t_one);
float total_time = (float)t_one/CLOCKS_PER_SEC;
cout<<"\nTotal time:"<<total_time<<" seconds"<<endl;

        if(!whistle_list.empty()){
            cout<<"Whistle exist!"<<endl;
            for(int i=0;i<whistle_list.size();i++){
//                cout<<"whistle_"<<i<<" start frequency="<<whistle_list[i].start_frq<<endl;
//                cout<<"whistle_"<<i<<" end frequency="<<whistle_list[i].end_frq<<endl;
//                cout<<"whistle_"<<i<<" duration="<<whistle_list[i].duration<<endl;
                
            
            }
        }
        else 
            cout<<"No whistle!"<<endl;

    
//Save data in matrix format
        saveData(P,fs,N,overlap);


}

void saveData(vector<vector<float> > P,int fs, int N, float overlap ){
    FILE *fp;    
    float power_value;

            fp = fopen("/home/yhhuang/thesis/Matlab/data_for_plot.dat3","w");

            for(unsigned int i=0;i<P.size();i++){
                for(unsigned int j=0;j<P[0].size();j++){
                    power_value = P[i][j];
                    fprintf(fp, "%f %s",power_value,"  ");
                }
                fprintf(fp,"%s","\n");
            }
            
            fclose(fp);

            float dt = time_mapping(1,fs,N,overlap)-time_mapping(0,fs,N,overlap);
            
            cout<<"df="<<fs/N<<endl;
            cout<<"dt="<<dt<<endl;
}

