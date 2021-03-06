//***********************************************************************
//Author: Yen-Hsiang Huang(yhhuang)
//Origin: National Taiwan University
//Date:   Aug 17th, 2018

//Library: fftw3 library
//***********************************************************************
//-------------------------------------------------------------------------
#include "detection_algorithm.h"
#include <stdlib.h>
#include <fftw3.h>
using namespace std;
//-------------------------------------------------------------------------

void save_data(std::string filename, FILE *fpp, vector<vector<float> > P, string filepath="./Output_Data/"){

    stringstream ss;
    string filepath_name;
    ss<<filepath<<filename;
    ss>>filepath_name;
    
    
    fpp = fopen(filepath_name.c_str(),"w");
    float value;
    for(unsigned int i=0;i<P.size();i++){
        for(unsigned int j=0;j<P[0].size();j++){
            value = P[i][j];
        fprintf(fpp,"%f %s",value,"  ");    
        }
        fprintf(fpp,"%s","\n");
    }
    fclose(fpp);
}


vector<vector<float> > spectrogram_yhh(spectrogram_input &sp_in)
{
//STEP_0 set up all variable;

    vector<float>   x       = sp_in.voltage_in;
    int             fs      = sp_in.fs;
    unsigned int    N       = sp_in.N;
    int             win     = sp_in.win;
    float           sen     = sp_in.sensitivity;
    float           gain    = sp_in.gain;

    float           overlap_percent = sp_in.overlap;

//STEP_1 set up window function 
    float   W;
    int     overlap = round(overlap_percent*N);
    int     no_overlap = N-overlap; 
    float   *window_func;

    if(no_overlap ==0)
        no_overlap =1;

    window_func = new float[N];

// 0 is Rectangular 
// 1 is Hanning 

    int i;
    switch (win) {
    
    case 0:  // rectangular
      for( i=0; i<N; ++i)
	    window_func[i] = 1.0F;
    break;
    
    case 1:  // Hanning window 
      W = N/2.0F;
      for( i=0; i<N; ++i)
	    window_func[i] = (1.0F + cos(pi*(i-W)/W))/2;
    break;
    
    default:  // others?
      fprintf(stderr, "unknown windowtype!\n");
    }
//STEP_2 set up fftw plan 
// only use date from 0 Hz ~ fs/2 Hz (Nyquist frequency)
    double*  in;
    fftw_complex *after_fft;
    vector<float>  power(N/2+1,0);
    unsigned int loop_num = 0;
    int     time_index = 0;
 
    in = new double[N];
    after_fft = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*(N/2+1));
    fftw_plan plan;
    plan = fftw_plan_dft_r2c_1d(N, in,after_fft,FFTW_ESTIMATE);

//Get the loop number to set up the size of the output matrice
    for(int start_index=0;start_index<=x.size()-N;start_index+=no_overlap)
        loop_num +=1;

//vector<vector<float> > output spectrogram matrix
    vector<vector<float> > spectrogram_mat((N/2)+1,vector<float>(loop_num));
    vector<vector<float> > PSD_mat((N/2)+1,vector<float>(loop_num));
//STEP_3 doing STFT
    for(int start_index=0;start_index<=x.size()-N;start_index+=no_overlap){
    //I. get data by window function 
        for(int i=start_index,j=0;i<start_index+N;i++,j++){
            in[j] = x[i]*window_func[j];    
        }
    //II. fft 
        fftw_execute(plan);

    //III. Calculate power
        //fftw_complex[0] is real part and fftw_complex[1] is imaginary part
        
        for(int k=0 ;k<=N/2;k++){
            power[k] = sqrt(after_fft[k][0]*after_fft[k][0]+after_fft[k][1]*after_fft[k][1]);
            spectrogram_mat[k][time_index] = power[k];

    // calculate exact power 
    // P = ((abs(power))^2 )*2/(fs*w(n)^2)
    // PSD = 10log(P) - sensitivity
            power[k] = (2*pow(power[k],2))/(fs*pow(window_func[k],2));
    //IV. Save in the 2 dimensional array
            float PSD = 10*log(power[k]) - sen + gain;
            PSD_mat[k][time_index] = PSD;
        }
        time_index++;
    }

        fftw_destroy_plan(plan);


//spectrogram_mat: row is frequency and column is time, just means x is time and y is frequency
//But remember to change the index to exact frequency and time using df and dt
    
    sp_in.PSD = PSD_mat;

    return(spectrogram_mat);
}



unsigned int frequency_mapping(unsigned int input_index, int fs,int N){
// input_index is the frequency index in spectrogram_mat which was output by spectrogram_yhh()
    unsigned int exact_frequency = round((fs/N)*input_index);

    return(exact_frequency);
}

unsigned int inv_freq_map(unsigned int input_frq, int fs, int N){

    unsigned int frq_index = round(input_frq*N/fs);

    return(frq_index);
}

float time_mapping(unsigned int input_index,int fs,int N, float overlap_percent){
// input_index is the time index in spectrogram_mat which was output by spectrogram_yhh()

    int     start_index = (N-round(overlap_percent*N))*input_index;
    float  exact_time  = float(start_index*2+N)/float(2*fs);
    return(exact_time);
}

unsigned int inv_time_map(float input_time,int fs, int N,float overlap_percent){
    
    unsigned int time_index = round((input_time*2*fs*N)/(2*(N-round(overlap_percent*N))));
    return(time_index);
}
//--------------------------------------------------------------------------------------
//function for detect algorithm
//1. simple moving average (not neccessary)
//2. median filter
//3. edge detector
//4. moving_square (time continuos property checking) 
//--------------------------------------------------------------------------------------

// simple_moving average function
void simple_mov_avg(vector<vector<float> > &P, int n_avg=10){

    
    vector<vector<float> >     avg_P(P.size(),vector<float>(P[0].size())); 

    for(int m=0;m<P.size();m++){
        int start_index = 0;
        
        while(start_index!=(P[0].size()-1)){
            float  temp    = 0;   //temperary sum
            for(int n=(start_index-n_avg/2);n<(start_index+n_avg/2);n++){
                if(!(n<0 || n>=P[0].size()))
                    temp += P[m][n]; 
                
            }
            avg_P[m][start_index] = temp/n_avg; 
            start_index++;
        }
    }
    P = avg_P;
}

// 3*3 median_filter function
void median_filter(vector<vector<float> > &P){

    for(unsigned int x = 1 ; x < P.size()-1 ; x++) {
        for(unsigned int y = 1; y < P[0].size()-1 ; y++) {

            unsigned int k = 0;  
            float window[9];  
            for(unsigned int xx = x - 1; xx < x + 2; ++xx){  
                for(unsigned int yy = y - 1; yy < y + 2; ++yy)  
                    window[k++] = P[xx][yy];  
            }
            //   Order elements (only half of them)  
            for (unsigned int m = 0; m < 5; ++m)  
            {  
                unsigned int min = m;  
                for (unsigned int n = m + 1; n < 9; ++n)  
                    if (window[n] < window[min])  
                        min = n;  
                //   Put found minimum element in its place  
               float temp = window[m];  
                window[m] = window[min];  
                window[min] = temp;  
            }  
            P[x][y]= window[4]; 
        }
    }
}

// Edge_detector function
// Can change jump number to higher for wider edge and lower for narrow edge 
void edge_detector(vector<vector<float> > &P,float SNR_threshold,unsigned int jump_num,unsigned int N, int fs, float frq1, float frq2){

    float SNR=0;
    vector<vector<float> > P_new(P.size(),vector<float>(P[0].size())); 
// lower and higher boundary tolerance 5 
    int low_bound = round(N*frq1/fs) - 5; 
    int high_bound = round(N*frq2/fs) + 5;
    if(low_bound < 0 )
        low_bound = 0;
    if(high_bound > P.size())
        high_bound = P.size();
    
    for(int i=0;i<P[0].size();i++){

        vector<float> time_column(P.size(),0);
        for(int k=low_bound;k<high_bound;k++){
            time_column[k] = P[k][i];
        }
             
        for(int j=jump_num;j<time_column.size();j++){
            if(j>=time_column.size()-jump_num)
                P[j][i] = 0;
            else{
                 if(time_column[j-jump_num]!=0 || time_column[j+jump_num]!=0){    
                    SNR= 10*log(time_column[j]/(time_column[j-jump_num]*0.5+time_column[j+jump_num]*0.5));
                    if(SNR > SNR_threshold)
                        P_new[j][i] = 1;
                 }
                 else if(time_column[j]!=0) 
                     P_new[j][i] = 1;
                 else 
                     P_new[j][i] = 0;
            }
        }
    }
    P = P_new;
}

//narrow band checking and time continuous properties checking function 
//bandpath filter with frq1 ~ frq2 Hz
void moving_square(vector<vector<float> > &P,unsigned int fs, unsigned int N, float overlap,float frq1,float frq2){
    vector<vector<float> >     P_new(P.size(),vector<float>(P[0].size()));

    
    vector<int> x_buf;
    vector<int> y_buf;
    int last_time_x;

    float  percent_threshold=0.5;       // threshold for deciding if it need to be left or delete
    unsigned int bandwidth_sample = 5;  // 215 Hz band width checking
    unsigned int time_width_sample = 11; // 0.0234 second time width checking

// band and time width sample number must be odd number
    if(bandwidth_sample%2==0)
        bandwidth_sample-=1;
    if(time_width_sample%2==0)
        time_width_sample+=1;

// only check band   frq1 ~ frq2   
    for(int x = N*frq1/fs ; x < N*frq2/fs-((bandwidth_sample-1)/2) ; x++) {
        for(int y = (time_width_sample-1)/2; y < P[0].size()-((time_width_sample-1)/2) ; y++) {

//over Nyquist frequency , break the loop1. 
            if(x>=N/2)
                break;
            
            float percent = 0;
            int k = 0;  
            x_buf.clear();
            y_buf.clear();

            for(int xx = x-(bandwidth_sample-1)/2; xx < x +((bandwidth_sample-1)/2)+1; ++xx){  
                for(int yy = y-(time_width_sample-1)/2;yy<y+((time_width_sample-1)/2)+1;++yy){  
                    if(P[xx][yy]==1){
                        k++;  
                        x_buf.push_back(xx);
                        y_buf.push_back(yy);
                    }
                        
                }
            }
            percent =float(k)/float(bandwidth_sample*time_width_sample);

            if(percent >= percent_threshold){ 
                    for(int i=0;i<x_buf.size();i++)
                        P_new[x_buf[i]][y_buf[i]] = 1;
            }
        }
//over Nyquist frequency , break the loop2. 
        if(x>=N/2)
            break;
    }
  
    P = P_new;
}

void detect_whistle(vector<vector<float> > &P,int fs,unsigned int N,float overlap,float SNR_threshold,float frq_low,float frq_high){
        
    FILE *fp_first, *fp_second, *fp_third, *fp_fourth, *fp_fifth;
//step0: save data before detection if needed   
    save_data("1_P",fp_first,P);
//step1: simple moving average for each frequency(not neccessary)
clock_t tic_1;
tic_1 = clock();
    simple_mov_avg(P,10);
clock_t toc_1;
toc_1 = clock();
cout<<"simple moving average time:"<<(float)(toc_1-tic_1)/CLOCKS_PER_SEC<<endl;
//step2: median filter
//    median_filter(P);
//    save_data("2_P",fp_second,P);
//step3: edge_detector
clock_t tic_2;
tic_2 = clock();

  edge_detector(P,SNR_threshold,5,N,fs,frq_low,frq_high);

clock_t toc_2;
toc_2 = clock();
cout<<"edge_detector time:"<<(float)(toc_2-tic_2)/CLOCKS_PER_SEC<<endl;
//    save_data("3_P",fp_third,P);
//step4: using moving square for narrow band checking 
clock_t tic_3;
tic_3 = clock();

    moving_square(P,fs,N,overlap,frq_low,frq_high);
clock_t toc_3;
toc_3 = clock();

cout<<"moving square time:"<<(float)(toc_3-tic_3)/CLOCKS_PER_SEC<<endl;
//step5: save data after detection    
    save_data("final_P",fp_fifth,P);
}


std::vector<whistle> check_result(vector<vector<float> > P, int fs,unsigned int N,float overlap){
// this is the function for checking result of the algorithm, but only work
// when there is only one chirp signal   
    
    vector<whistle> output;
    output.clear();
    
    xy_index        last_node;
    xy_index        current_node;
    whistle         wh;
    
    wh.fs = fs;
    wh.N  = N;
    wh.overlap = overlap;

    int     x_ind_tor=7;  //x index tolerance
    int     y_ind_tor=10;  //y index tolerance
    bool    first_node = true;
    int     checking_bandwidth = 300;  //Hz
    int     checking_bandwidth_sample = inv_freq_map(checking_bandwidth,fs,N);
    
    for(int i=0;i<P[0].size();i++){
        for(int j=0;j<P.size();j++){
            if(P[j][i]==1){
                if(first_node){
                    last_node.x = i;
                    last_node.y = j;
                    wh.start_node = last_node;
                    first_node = false;
                }
                else{
                    if(fabs(i-last_node.x)<x_ind_tor && fabs(j-last_node.y)<y_ind_tor){
                        last_node.x = i;
                        last_node.y = j;
                    }
                    else{
                        wh.end_node = last_node;
                        wh.calculate();
                        output.push_back(wh);
                        
                        last_node.x = i;
                        last_node.y = j;
                        wh.start_node = last_node;
                    }
                }
            }    
        }
    }
    
    if(!first_node){
        wh.end_node = last_node;
        wh.calculate();
        output.push_back(wh);
    }

    return(output);
}
