//
//  Classification.cpp
//  Terminator Arm
//
//  Created by Cory Bethrant
//
//

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
using namespace std;

int main()
{
    // Function receiving an array of data with 240 integers.
    // i.e an 80-emg x 30-Datalines array every sec. or so at run-time
    
    ifstream infile;
    infile.open("inputdata.txt");
    
    // Function Variables
    int dataSize = 240;
    no_pods = 8; string gesture = "Rest";
    
    for (int i =0; i < dataSize; i++)
    {
        infile >> input_Char[i];
        if (input_Char[i] < 0) input_Char[i] = sqrt(input_Char[i] *input_Char[i]);
    }
    float Rest = 0.0, Thumb = 0.0, Index = 0.0, Middle = 0.0, Ring = 0.0, Pinky = 0.0, Wrist = 0.0;
    int Pod0 = 0, Pod1 = 0, Pod2 = 0, Pod3 = 0, Pod4 = 0, Pod5 = 0, Pod6 = 0, Pod7 = 0;
    int checkvalues[] = { 4, 5, 6, 7, 8 };
    float Percents[5][no_pods]; float finalvalue[7];
    


    // Determine percentages for values between 4 through 8
    for (int i = 0; i < sizeof(checkvalues) / sizeof(checkvalues[0]); i++){
        for (int j = 0; j < datasize; j++){            // Score no_times data is less than 8
            if (input_Char[j] < checkvalues[i]){
                if (j == 0 || j % 8 == 0)  Pod0++;                   // Pod 0
                else if (j == 1 || (j - 1) % 8 == 0)  Pod1++;        // Pod 1
                else if (j == 2 || (j - 2) % 8 == 0)  Pod2++;        // Pod 2
                else if (j == 3 || (j - 3) % 8 == 0)  Pod3++;        // Pod 3
                else if (j == 4 || (j - 4) % 8 == 0)  Pod4++;        // Pod 4
                else if (j == 5 || (j - 5) % 8 == 0)  Pod5++;        // Pod 5
                else if (j == 6 || (j - 6) % 8 == 0)  Pod6++;        // Pod 6
                else Pod7++;                                         // Pod 7
            }
        }
        
        // Compute percent based off no_times
        Percents[i][0] = (Pod0 * no_pods * 100) / datasize; 
        Percents[i][1] = (Pod1 * no_pods * 100) / datasize; 
        Percents[i][2] = (Pod2 * no_pods * 100) / datasize;
        Percents[i][3] = (Pod3 * no_pods * 100) / datasize; 
        Percents[i][4] = (Pod4 * no_pods * 100) / datasize; 
        Percents[i][5] = (Pod5 * no_pods * 100) / datasize;
        Percents[i][6] = (Pod6 * no_pods * 100) / datasize; 
        Percents[i][7] = (Pod7 * no_pods * 100) / datasize;
        
        // Reset Pod values
        Pod0 = 0, Pod1 = 0, Pod2 = 0, Pod3 = 0, Pod4 = 0, Pod5 = 0, Pod6 = 0, Pod7 = 0;
    }

    bool Flag = 0;
    // Rest
    for (int i = 0; i < 6; i++){
        if (Percents[4][i + 1] < 90.0) Flag = 1;
    } if (!Flag) Rest = 4.0;
