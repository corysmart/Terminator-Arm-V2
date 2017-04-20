
//  Created by Cory Bethrant
//  Terminator Arm V2
//
//  Copyright © 2017 Team Terminator. All rights reserved.
// Dependencies:
// 1. MyoConnect
// 2. Infile.txt (Containing dynamic final part of output data file)

// Description:
// This is used for testing/Demo with Machine Learning.
// EMG streaming is only supported for one Myo at a time
// Terminator Myo MAC address: db-fa-0c-69-14-78 (Can be used alternatively instead of MyoConnect to attach our Myo to a Hub)
// http://diagnostics.myo.com/ provides diagnostic data of Myo (connected in MyoConnect) on a pc 
// Notes:
// The Myo armband will go into sleep mode if it's not synced and sits idle for a short period of time. You can't turn that off here. (just keep it synced!)
// 
// NB: Replacing system("cls") is advised.


// #define _USE_MATH_DEFINES
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>
#include "windows.h"   // contains Sleep library
#include <time.h>

#include <myo/myo.hpp>            // Myo default header library

// DataCollector class inheriting member functions from myo::DeviceListener
class DataCollector : public myo::DeviceListener {
public:
	DataCollector() : onArm(false), emgSamples()
	{}

	// onArmSync() is called whenever HUTerminator Myo recognizes a Sync Gesture after someone has put it on their
	// arm. This lets Myo know which arm it's on and which way it's facing.
	void onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection, float rotation,
		myo::WarmupState warmupState)
	{
		system("cls"); std::cout << "On ARM SYNC CALLED!!" << std::endl;
		onArm = true;
		whichArm = arm;
	}
	// onArmUnsync() is called whenever HUTerminator Myo has detected that it was moved from a stable position on a person's arm after
	// it recognized the arm. Typically this happens when someone takes it off of their arm, but it can also happen
	// when the Myo is moved around on the arm.
	void onArmUnsync(myo::Myo* myo, uint64_t timestamp)
	{
		system("cls"); std::cout << "On ARM UN-SYNC CALLED!!" << std::endl;
		onArm = false;
		Sleep(5000);
	}

	// onUnpair() is called whenever the Myo is disconnected from Myo Connect by the user
	void onUnpair(myo::Myo* myo, uint64_t timestamp)
	{
		// We've lost our Myo.
		// Zeros are seen in output file if there was a disconnect so test can be repeated
		emgSamples.fill(0);
		bool restartCal = true;		//restarts if connection lost
		onArm = false;
	}

	// onEmgData() is called whenever a paired Myo has provided new EMG data, and EMG streaming is enabled
	// Classifier output data
	void onEmgData(myo::Myo* myo, uint64_t timestamp, const int8_t* emg)
	{
		for (int i = 0; i < 8; i++) {
			emgSamples[i] = emg[i];
		}
	}

	// The values of this array is set by onEmgData() above
	std::array<int8_t, 8> emgSamples;
	bool onArm; myo::Arm whichArm;

	// There are other virtual functions in DeviceListener that we could override here, like onAccelerometerData()
	// For this application, the functions overridden above are sufficient
	std::string SerialIndex, update = "gesture";
	std::ofstream TerminatorFile;
	std::ifstream TerminatorRefFile;
	std::string filepath = "C:\\TeamTerminatorData";
	int NewLineflag = 0;
	vector<string> RefData_S (8);	//vector to store chars from infile, 8 pods
	vector<int> RefData (8);	//vector to hold reference data (for average of 8 pods)
	vector<int> RefData_Total;	//vector to hold all of reference data (for variance)
	const int MAXSIZE = 4;		//reads in, at most, 4 characters
    	char thisVal[MAXSIZE];		//char array to hold individual values from ref data
	char new_line;	
	int counter = 0;	//keeps track of how many additions occur (how many lines are processed per gesture)
	double Pod1_avg, Pod2_avg, Pod3_avg, Pod4_avg, Pod5_avg, Pod6_avg, Pod7_avg, Pod8_avg = 0;//avg values for pods (per gesture)
	double Pod1_var, Pod2_var, Pod3_var, Pod4_var, Pod5_var, Pod6_var, Pod7_var, Pod8_var = 0;//variance for pods (per gesture) 
	vector<double> Rest_avg(8,0), Index_avg(8,0), Middle_avg(8,0), Ring_avg(8,0) Pinky_avg(8,0), Hand_avg(8,0);	//avg values for gestures for calibration
	vector<double> Rest_var(8,0), Index_var(8,0), Middle_var(8,0), Ring_var(8,0) Pinky_var(8,0), Hand_var(8,0);	//avg values for variance gestures for calibration
	

	// We define this function to write the current values that were updated by the on...() functions above
	void writeData(std::string gesture)
	{
		// Create and open dynamic outfile 
		std::string filename = filepath + SerialIndex + ".txt";
		TerminatorFile.open(filename, std::ofstream::app);

		// Write current gesture to outfile
		if (update != gesture){
			TerminatorFile << "\n" << gesture << "\n"; update = gesture;
		}

		// Clear the current line
		std::cout << '\r';
		// write out the EMG classifier data
		for (size_t i = 0; i < emgSamples.size(); i++) {
			std::ostringstream oss;
			oss << static_cast<int>(emgSamples[i]);        // convert 8-bit array into int
			std::string emgString = oss.str();

			if (NewLineflag == 1){
				TerminatorFile << "\n";
				NewLineflag = 0;
			}

			// Write to outfile
			TerminatorFile << emgString << " ";
		}
		NewLineflag = 1;
		std::cout << std::flush;
		TerminatorFile.close();       // flush buffer to outfile
	}

	// We define this function to calibrate our logged data
	void calibrateData(){
		//Open infile for reference data
		TerminatorRefFile.open("Reference.csv");
	for (int a = 0; a < 12; a++){	//cycles through 12 different gestures (asterisks in file)
		while(TerminatorRefFile.getline(new_line, 999,999, '*'){		//reads one gesture at a time
			while(TerminatorRefFile.getline(new_line, 100, '/n'){		//reads one line at a time
				for (int i=0; i<7; i++){				//8 values per line
					while(TerminatorRefFile.getline(thisVal,MAXSIZE,',')) {	//reads in four chars, unless it hits a comma
					RefData_S.at(i)=thisVal;	
					RefData.at(i)=RefData.at(i)+std::stoi(RefData_S.at(i), &sz) //coverts string to int, adds columns to average pod values
					RefData_Total.pushback(std::stoi(RefData_S.at(i), &sz));    //adds each value to the end of RefData_Total vector
					}
				}
			counter++; //keeps track of how many additions occur (how many lines are processed per gesture)
			}
		Pod1_avg=RefData.at(0)/counter;		//calculates average pod values for given gesture
		Pod2_avg=RefData.at(1)/counter;
		Pod3_avg=RefData.at(2)/counter;
		Pod4_avg=RefData.at(3)/counter;
		Pod5_avg=RefData.at(4)/counter;
		Pod6_avg=RefData.at(5)/counter;
		Pod7_avg=RefData.at(6)/counter;
		Pod8_avg=RefData.at(7)/counter;
		counter = 0;
		
		int temp_var;
		double temp_var_sq;
		vector<int> Variance_Data (8);	//vector to hold all of temp_var_sq (for variance)
	
		//In this section, we calculate the varinace for the 8 Pods
		for(int x = 0; x < RefData_Total.size(); x + 8){	//grabs every 8th entry (Pod 1)
			temp_var = RefData_Total.at(x)-Pod1_avg;
			temp_var_sq = temp_var * temp_var;
			Variance_Data.at(0)=Variance_Data.at(0) + temp_var_sq;	//sums all temp_var_sq for Pod 1
			counter ++;		//keeps track of how many additions occur (how many variances are added)
			}
			Pod1_var = Variance_Data.at(0)/counter;
			counter = 0;
		for(int x = 1; x < RefData_Total.size(); x + 8){	//grabs every 8th entry (Pod 2)
			temp_var = RefData_Total.at(x)-Pod2_avg;
			temp_var_sq = temp_var * temp_var;
			Variance_Data.at(1)=Variance_Data.at(1) + temp_var_sq;	
			counter ++;		
			}
			Pod2_var = Variance_Data.at(1)/counter;
			counter = 0;
		for(int x = 2; x < RefData_Total.size(); x + 8){	//grabs every 8th entry (Pod 3)
			temp_var = RefData_Total.at(x)-Pod3_avg;
			temp_var_sq = temp_var * temp_var;
			Variance_Data.at(2)=Variance_Data.at(2) + temp_var_sq;	
			counter ++;		
			}
			Pod3_var = Variance_Data.at(2)/counter;
			counter = 0;
		for(int x = 3; x < RefData_Total.size(); x + 8){	//grabs every 8th entry (Pod 4)
			temp_var = RefData_Total.at(x)-Pod4_avg;
			temp_var_sq = temp_var * temp_var;
			Variance_Data.at(3)=Variance_Data.at(3) + temp_var_sq;	
			counter ++;		
			}
			Pod4_var = Variance_Data.at(3)/counter;
			counter = 0;
		for(int x = 4; x < RefData_Total.size(); x + 8){	//grabs every 8th entry (Pod 5)
			temp_var = RefData_Total.at(x)-Pod5_avg;
			temp_var_sq = temp_var * temp_var;
			Variance_Data.at(4)=Variance_Data.at(4) + temp_var_sq;	
			counter ++;		
			}
			Pod5_var = Variance_Data.at(4)/counter;
			counter = 0;
		for(int x = 5; x < RefData_Total.size(); x + 8){	//grabs every 8th entry (Pod 6)
			temp_var = RefData_Total.at(x)-Pod6_avg;
			temp_var_sq = temp_var * temp_var;
			Variance_Data.at(5)=Variance_Data.at(5) + temp_var_sq;	
			counter ++;		
			}
			Pod6_var = Variance_Data.at(5)/counter;
			counter = 0;
		for(int x = 6; x < RefData_Total.size(); x + 8){	//grabs every 8th entry (Pod 7)
			temp_var = RefData_Total.at(x)-Pod7_avg;
			temp_var_sq = temp_var * temp_var;
			Variance_Data.at(6)=Variance_Data.at(6) + temp_var_sq;	
			counter ++;		
			}
			Pod7_var = Variance_Data.at(6)/counter;
			counter = 0;
		for(int x = 7; x < RefData_Total.size(); x + 8){	//grabs every 8th entry (Pod 8)
			temp_var = RefData_Total.at(x)-Pod8_avg;
			temp_var_sq = temp_var * temp_var;
			Variance_Data.at(7)=Variance_Data.at(7) + temp_var_sq;	
			counter ++;		
			}
			Pod8_var = Variance_Data.at(7)/counter;
			counter = 0;
		}//end of a single gesture
		
		//store avg and variance per gesture (for future access)
		if (a=0){	//Gesture is first rest
			Rest_avg.at(0) = Rest_avg.at(0) + Pod1_avg;
			Rest_avg.at(1) = Rest_avg.at(1) + Pod2_avg;
			Rest_avg.at(2) = Rest_avg.at(2) + Pod3_avg;
			Rest_avg.at(3) = Rest_avg.at(3) + Pod4_avg;
			Rest_avg.at(4) = Rest_avg.at(4) + Pod5_avg;
			Rest_avg.at(5) = Rest_avg.at(5) + Pod6_avg;
			Rest_avg.at(6) = Rest_avg.at(6) + Pod7_avg;
			Rest_avg.at(7) = Rest_avg.at(7) + Pod8_avg;
			
			Rest_var.at(0) = Rest_var.at(0) + Pod1_var;
			Rest_var.at(1) = Rest_var.at(1) + Pod2_var;
			Rest_var.at(2) = Rest_var.at(2) + Pod3_var;
			Rest_var.at(3) = Rest_var.at(3) + Pod4_var;
			Rest_var.at(4) = Rest_var.at(4) + Pod5_var;
			Rest_var.at(5) = Rest_var.at(5) + Pod6_var;
			Rest_var.at(6) = Rest_var.at(6) + Pod7_var;
			Rest_var.at(7) = Rest_var.at(7) + Pod8_var;
			}
		else if (a=1){	//Gesture is thumb
			Thumb_avg(0) = Thumb_avg.at(0) + Pod1_avg;
			Thumb_avg(1) = Thumb_avg.at(1) + Pod1_avg;
			Thumb_avg(2) = Thumb_avg.at(2) + Pod1_avg;
			Thumb_avg(3) = Thumb_avg.at(3) + Pod1_avg;
			Thumb_avg(4) = Thumb_avg.at(4) + Pod1_avg;
			Thumb_avg(5) = Thumb_avg.at(5) + Pod1_avg;
			Thumb_avg(6) = Thumb_avg.at(6) + Pod1_avg;
			Thumb_avg(7) = Thumb_avg.at(7) + Pod1_avg;
			
			Thumb_var.at(0) = Thumb_var.at(0) + Pod1_var;
			Thumb_var.at(1) = Thumb_var.at(1) + Pod2_var;
			Thumb_var.at(2) = Thumb_var.at(2) + Pod3_var;
			Thumb_var.at(3) = Thumb_var.at(3) + Pod4_var;
			Thumb_var.at(4) = Thumb_var.at(4) + Pod5_var;
			Thumb_var.at(5) = Thumb_var.at(5) + Pod6_var;
			Thumb_var.at(6) = Thumb_var.at(6) + Pod7_var;
			Thumb_var.at(7) = Thumb_var.at(7) + Pod8_var;
			}
		else if (a=2){	//Gesture is rest
			Rest_avg.at(0) = Rest_avg.at(0) + Pod1_avg;
			Rest_avg.at(1) = Rest_avg.at(1) + Pod2_avg;
			Rest_avg.at(2) = Rest_avg.at(2) + Pod3_avg;
			Rest_avg.at(3) = Rest_avg.at(3) + Pod4_avg;
			Rest_avg.at(4) = Rest_avg.at(4) + Pod5_avg;
			Rest_avg.at(5) = Rest_avg.at(5) + Pod6_avg;
			Rest_avg.at(6) = Rest_avg.at(6) + Pod7_avg;
			Rest_avg.at(7) = Rest_avg.at(7) + Pod8_avg;
			
			Rest_var.at(0) = Rest_var.at(0) + Pod1_var;
			Rest_var.at(1) = Rest_var.at(1) + Pod2_var;
			Rest_var.at(2) = Rest_var.at(2) + Pod3_var;
			Rest_var.at(3) = Rest_var.at(3) + Pod4_var;
			Rest_var.at(4) = Rest_var.at(4) + Pod5_var;
			Rest_var.at(5) = Rest_var.at(5) + Pod6_var;
			Rest_var.at(6) = Rest_var.at(6) + Pod7_var;
			Rest_var.at(7) = Rest_var.at(7) + Pod8_var;
			}
		else if (a=3){	//Gesture is index
			Index_avg.at(0) = Index_avg.at(0) + Pod1_avg;
			Index_avg.at(1) = Index_avg.at(1) + Pod2_avg;
			Index_avg.at(2) = Index_avg.at(2) + Pod3_avg;
			Index_avg.at(3) = Index_avg.at(3) + Pod4_avg;
			Index_avg.at(4) = Index_avg.at(4) + Pod5_avg;
			Index_avg.at(5) = Index_avg.at(5) + Pod6_avg;
			Index_avg.at(6) = Index_avg.at(6) + Pod7_avg;
			Index_avg.at(7) = Index_avg.at(7) + Pod8_avg;
			
			Index_var.at(0) = Index_var.at(0) + Pod1_var;
			Index_var.at(1) = Index_var.at(1) + Pod2_var;
			Index_var.at(2) = Index_var.at(2) + Pod3_var;
			Index_var.at(3) = Index_var.at(3) + Pod4_var;
			Index_var.at(4) = Index_var.at(4) + Pod5_var;
			Index_var.at(5) = Index_var.at(5) + Pod6_var;
			Index_var.at(6) = Index_var.at(6) + Pod7_var;
			Index_var.at(7) = Index_var.at(7) + Pod8_var;
			}
		else if (a=4){	//Gesture is rest
			Rest_avg.at(0) = Rest_avg.at(0) + Pod1_avg;
			Rest_avg.at(1) = Rest_avg.at(1) + Pod2_avg;
			Rest_avg.at(2) = Rest_avg.at(2) + Pod3_avg;
			Rest_avg.at(3) = Rest_avg.at(3) + Pod4_avg;
			Rest_avg.at(4) = Rest_avg.at(4) + Pod5_avg;
			Rest_avg.at(5) = Rest_avg.at(5) + Pod6_avg;
			Rest_avg.at(6) = Rest_avg.at(6) + Pod7_avg;
			Rest_avg.at(7) = Rest_avg.at(7) + Pod8_avg;
			
			Rest_var.at(0) = Rest_var.at(0) + Pod1_var;
			Rest_var.at(1) = Rest_var.at(1) + Pod2_var;
			Rest_var.at(2) = Rest_var.at(2) + Pod3_var;
			Rest_var.at(3) = Rest_var.at(3) + Pod4_var;
			Rest_var.at(4) = Rest_var.at(4) + Pod5_var;
			Rest_var.at(5) = Rest_var.at(5) + Pod6_var;
			Rest_var.at(6) = Rest_var.at(6) + Pod7_var;
			Rest_var.at(7) = Rest_var.at(7) + Pod8_var;
			}
		else if (a=5){	//Gesture is middle
			Middle_avg.at(0) = Middle_avg.at(0) + Pod1_avg;
			Middle_avg.at(1) = Middle_avg.at(1) + Pod2_avg;
			Middle_avg.at(2) = Middle_avg.at(2) + Pod3_avg;
			Middle_avg.at(3) = Middle_avg.at(3) + Pod4_avg;
			Middle_avg.at(4) = Middle_avg.at(4) + Pod5_avg;
			Middle_avg.at(5) = Middle_avg.at(5) + Pod6_avg;
			Middle_avg.at(6) = Middle_avg.at(6) + Pod7_avg;
			Middle_avg.at(7) = Middle_avg.at(7) + Pod8_avg;
			
			Middle_var.at(0) = Middle_var.at(0) + Pod1_var;
			Middle_var.at(1) = Middle_var.at(1) + Pod2_var;
			Middle_var.at(2) = Middle_var.at(2) + Pod3_var;
			Middle_var.at(3) = Middle_var.at(3) + Pod4_var;
			Middle_var.at(4) = Middle_var.at(4) + Pod5_var;
			Middle_var.at(5) = Middle_var.at(5) + Pod6_var;
			Middle_var.at(6) = Middle_var.at(6) + Pod7_var;
			Middle_var.at(7) = Middle_var.at(7) + Pod8_var;
			}
		else if (a=6){	//Gesture is rest
			Rest_avg.at(0) = Rest_avg.at(0) + Pod1_avg;
			Rest_avg.at(1) = Rest_avg.at(1) + Pod2_avg;
			Rest_avg.at(2) = Rest_avg.at(2) + Pod3_avg;
			Rest_avg.at(3) = Rest_avg.at(3) + Pod4_avg;
			Rest_avg.at(4) = Rest_avg.at(4) + Pod5_avg;
			Rest_avg.at(5) = Rest_avg.at(5) + Pod6_avg;
			Rest_avg.at(6) = Rest_avg.at(6) + Pod7_avg;
			Rest_avg.at(7) = Rest_avg.at(7) + Pod8_avg;
			
			Rest_var.at(0) = Rest_var.at(0) + Pod1_var;
			Rest_var.at(1) = Rest_var.at(1) + Pod2_var;
			Rest_var.at(2) = Rest_var.at(2) + Pod3_var;
			Rest_var.at(3) = Rest_var.at(3) + Pod4_var;
			Rest_var.at(4) = Rest_var.at(4) + Pod5_var;
			Rest_var.at(5) = Rest_var.at(5) + Pod6_var;
			Rest_var.at(6) = Rest_var.at(6) + Pod7_var;
			Rest_var.at(7) = Rest_var.at(7) + Pod8_var;
			}
		else if (a=7){	//Gesture is ring
			Ring_avg.at(0) = Ring_avg.at(0) + Pod1_avg;
			Ring_avg.at(1) = Ring_avg.at(1) + Pod2_avg;
			Ring_avg.at(2) = Ring_avg.at(2) + Pod3_avg;
			Ring_avg.at(3) = Ring_avg.at(3) + Pod4_avg;
			Ring_avg.at(4) = Ring_avg.at(4) + Pod5_avg;
			Ring_avg.at(5) = Ring_avg.at(5) + Pod6_avg;
			Ring_avg.at(6) = Ring_avg.at(6) + Pod7_avg;
			Ring_avg.at(7) = Ring_avg.at(7) + Pod8_avg;
			
			Ring_var.at(0) = Ring_var.at(0) + Pod1_var;
			Ring_var.at(1) = Ring_var.at(1) + Pod2_var;
			Ring_var.at(2) = Ring_var.at(2) + Pod3_var;
			Ring_var.at(3) = Ring_var.at(3) + Pod4_var;
			Ring_var.at(4) = Ring_var.at(4) + Pod5_var;
			Ring_var.at(5) = Ring_var.at(5) + Pod6_var;
			Ring_var.at(6) = Ring_var.at(6) + Pod7_var;
			Ring_var.at(7) = Ring_var.at(7) + Pod8_var;
			}
		else if (a=8){	//Gesture is rest
			Rest_avg.at(0) = Rest_avg.at(0) + Pod1_avg;
			Rest_avg.at(1) = Rest_avg.at(1) + Pod2_avg;
			Rest_avg.at(2) = Rest_avg.at(2) + Pod3_avg;
			Rest_avg.at(3) = Rest_avg.at(3) + Pod4_avg;
			Rest_avg.at(4) = Rest_avg.at(4) + Pod5_avg;
			Rest_avg.at(5) = Rest_avg.at(5) + Pod6_avg;
			Rest_avg.at(6) = Rest_avg.at(6) + Pod7_avg;
			Rest_avg.at(7) = Rest_avg.at(7) + Pod8_avg;
			
			Rest_var.at(0) = Rest_var.at(0) + Pod1_var;
			Rest_var.at(1) = Rest_var.at(1) + Pod2_var;
			Rest_var.at(2) = Rest_var.at(2) + Pod3_var;
			Rest_var.at(3) = Rest_var.at(3) + Pod4_var;
			Rest_var.at(4) = Rest_var.at(4) + Pod5_var;
			Rest_var.at(5) = Rest_var.at(5) + Pod6_var;
			Rest_var.at(6) = Rest_var.at(6) + Pod7_var;
			Rest_var.at(7) = Rest_var.at(7) + Pod8_var;
			}
		else if (a=9){	//Gesture is pinky
			Pinky_avg.at(0) = Pinky_avg.at(0) + Pod1_avg;
			Pinky_avg.at(1) = Pinky_avg.at(1) + Pod2_avg;
			Pinky_avg.at(2) = Pinky_avg.at(2) + Pod3_avg;
			Pinky_avg.at(3) = Pinky_avg.at(3) + Pod4_avg;
			Pinky_avg.at(4) = Pinky_avg.at(4) + Pod5_avg;
			Pinky_avg.at(5) = Pinky_avg.at(5) + Pod6_avg;
			Pinky_avg.at(6) = Pinky_avg.at(6) + Pod7_avg;
			Pinky_avg.at(7) = Pinky_avg.at(7) + Pod8_avg;
			
			Pinky_var.at(0) = Pinky_var.at(0) + Pod1_var;
			Pinky_var.at(1) = Pinky_var.at(1) + Pod2_var;
			Pinky_var.at(2) = Pinky_var.at(2) + Pod3_var;
			Pinky_var.at(3) = Pinky_var.at(3) + Pod4_var;
			Pinky_var.at(4) = Pinky_var.at(4) + Pod5_var;
			Pinky_var.at(5) = Pinky_var.at(5) + Pod6_var;
			Pinky_var.at(6) = Pinky_var.at(6) + Pod7_var;
			Pinky_var.at(7) = Pinky_var.at(7) + Pod8_var;
			}
		else if (a=10){	//Gesture is rest
			Rest_avg.at(0) = Rest_avg.at(0) + Pod1_avg;
			Rest_avg.at(1) = Rest_avg.at(1) + Pod2_avg;
			Rest_avg.at(2) = Rest_avg.at(2) + Pod3_avg;
			Rest_avg.at(3) = Rest_avg.at(3) + Pod4_avg;
			Rest_avg.at(4) = Rest_avg.at(4) + Pod5_avg;
			Rest_avg.at(5) = Rest_avg.at(5) + Pod6_avg;
			Rest_avg.at(6) = Rest_avg.at(6) + Pod7_avg;
			Rest_avg.at(7) = Rest_avg.at(7) + Pod8_avg;
			
			Rest_var.at(0) = Rest_var.at(0) + Pod1_var;
			Rest_var.at(1) = Rest_var.at(1) + Pod2_var;
			Rest_var.at(2) = Rest_var.at(2) + Pod3_var;
			Rest_var.at(3) = Rest_var.at(3) + Pod4_var;
			Rest_var.at(4) = Rest_var.at(4) + Pod5_var;
			Rest_var.at(5) = Rest_var.at(5) + Pod6_var;
			Rest_var.at(6) = Rest_var.at(6) + Pod7_var;
			Rest_var.at(7) = Rest_var.at(7) + Pod8_var;
			}
		else if (a=11){	//Gesture is hand
			Hand_avg.at(0) = Hand_avg.at(0) + Pod1_avg;
			Hand_avg.at(1) = Hand_avg.at(1) + Pod2_avg;
			Hand_avg.at(2) = Hand_avg.at(2) + Pod3_avg;
			Hand_avg.at(3) = Hand_avg.at(3) + Pod4_avg;
			Hand_avg.at(4) = Hand_avg.at(4) + Pod5_avg;
			Hand_avg.at(5) = Hand_avg.at(5) + Pod6_avg;
			Hand_avg.at(6) = Hand_avg.at(6) + Pod7_avg;
			Hand_avg.at(7) = Hand_avg.at(7) + Pod8_avg;
			
			Hand_var.at(0) = Hand_var.at(0) + Pod1_var;
			Hand_var.at(1) = Hand_var.at(1) + Pod2_var;
			Hand_var.at(2) = Hand_var.at(2) + Pod3_var;
			Hand_var.at(3) = Hand_var.at(3) + Pod4_var;
			Hand_var.at(4) = Hand_var.at(4) + Pod5_var;
			Hand_var.at(5) = Hand_var.at(5) + Pod6_var;
			Hand_var.at(6) = Hand_var.at(6) + Pod7_var;
			Hand_var.at(7) = Hand_var.at(7) + Pod8_var;
			}
	}
		//TerminatorRefFile.close();		not sure if this should go here, because reopening it would start reading from beginning again
		
		std::cout << "\tPlease follow the instructions to perform CALIBRATION!" << std::endl;
		std::cout << "\t Allow a couple seconds while Terminator Myo warms up to arm... " << std::endl << std::endl;
		Sleep(5000);             // suspend execution of current/active thread for time-argument

		for (int i = 0; i < sizeof(gestures) / sizeof(*gestures); i++){
			system("cls");
			std::cout << std::endl; std::cout << "\n\n\n \t\t Perform:  " << gestures[i] << " for (5) secs" << std::endl;

			// Get current CPU time
			double startTime = GetTickCount();
			double currentTime = 0;

			while ((GetTickCount() - startTime) <= 1000) {}; // wait for 1 extra sec for user change
			
			// Record data for 3 seconds
			while (currentTime <= 3000)
			{
				// In each iteration of this loop, we run the Myo event loop for a set number of milliseconds
				// In this case, we wish to update our display 50 times a second. (Myo provides EMG at 200Hz and IMU data at 50Hz and is unaffected by display rates)
				hub.run(1);
				//hub.runOnce();
				//std::cout << "HUB running!" << std::endl;
				// After processing events, we call the writeData() function to write new data to our outfile
				collector.writeData(gestures[i]);
				//std::cout << "Written data!" << std::endl;
				// Update time for iteration purposes
				currentTime = GetTickCount() - startTime;
			}
			while ((GetTickCount() - startTime) <= 5000) {}; // wait for 1 extra sec for user change
		}

			system("cls"); std::cout << "\t\t Calibrating logged Data..." << std::endl;
			// Data calibration approach/ machine learning algorithm goes here
			std::cout << "\t\t Calibration complete!" << std::endl;
	}

	// We define this function for gesture recognition with calibration results as long as HUTerminator Myo is worn
	void listenforGesture(){
		while (onArm){
			system("cls");
			std::cout << "Terminator Myo is still on arm!" << std::endl;
			// Record data
			hub.run(1);
			//hub.runOnce();
			//std::cout << "HUB running!" << std::endl;
			// Data is now incoming
			// Live Data Processing Goes Here
		} 
	}

	void holdState()
	{
		while (!onArm){}
	}
};


int main(int argc, char** argv)
{
	// Declare & open infile 
	std::ifstream Serialfile;
	Serialfile.open("Infile.txt");

	// Define gestures
	std::string gestures[12] = { "*REST/RELAX* position", "*(THUMB)* contraction", "*REST/RELAX* position", "*(INDEX fing.)* contraction", "*REST/RELAX* position",
		"*(MIDDLE fing.)* contraction", "*REST/RELAX* position", "*(RING fing.)* contraction", "*REST/RELAX* position",
		"*(PINKY fing.)* contraction", "*REST/RELAX* position", "*(HAND)* contraction" };

	// We catch any exceptions that might occur below -- see the catch statement for more details
	try {
		// First, we create a Hub without any application identifier (I deemed it unnecessary), the Hub provides access to one or more Myos
		myo::Hub hub("");

		std::cout << "\t\t Attempting to find HU Terminator Myo..." << std::endl;

		// Next, we attempt to find a Myo to use. If a Myo is already paired in Myo Connect, this will return that Myo immediately
		// waitForMyo() takes a timeout value in milliseconds. We try to find Terminator Myo for 10 seconds, and
		// if that fails, the function will return a null pointer
		myo::Myo* myo = hub.waitForMyo(0); //  Times-out until Terminator Myo is found!

		///////////////////////// To-do: CONNECT TERMINATOR MYO BY MAC ADDRESS AND KEEP UNLOCKED //////////////////////////////////////////////////////
		hub.setLockingPolicy(hub.lockingPolicyNone);      // Keep Terminator Myo unlocked
		//Hub::a               attachByMacAddress(String macAddress)
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// If waitForMyo() returned a null pointer, we failed to find our Myo, so exit with an error message
		if (!myo) {
			throw std::runtime_error("\t\t Unable to find HU Terminator Myo! \nCheck MyoConnect!");
		}

		// We've found our Myo!
		std::cout << "\t\t  Connected to HU Terminator Myo!" << std::endl << std::endl;

		// Next we enable EMG streaming on the found Myo
		myo->setStreamEmg(myo::Myo::streamEmgEnabled);
		myo->unlockHold;

		// Next we construct an instance of our DeviceListener, so that we can register it with the Hub
		DataCollector collector;

		// Grab index of subject for file-writing purposes
		while (!Serialfile.eof())
			Serialfile >> collector.SerialIndex;

		// Hub::addListener() takes the address of any object whose class inherits from DeviceListener, and will cause
		// Hub::run() to send events to all registered device listeners
		hub.addListener(&collector);

		// Set console font parameters (Make easier to follow instructions)
		CONSOLE_FONT_INFOEX cfi;
		cfi.cbSize = sizeof cfi;
		cfi.nFont = 0;
		cfi.dwFontSize.X = 0;
		cfi.dwFontSize.Y = 16;
		cfi.FontFamily = FF_DONTCARE;
		cfi.FontWeight = FW_NORMAL;
		wcscpy_s(cfi.FaceName, L"Consolas");
		SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
		
     // Calibration:
		collector.calibrateData();
		// Data succesfully logged
		// Calibration complete! listen for gestures for as long as HUTerminator myo remains synced with arm
		system("cls");
		std::cout << "\n\n\n \t\t Now listening for gestures..." << std::endl;
		collector.listenforGesture();
		// Ask to re-calibrate, when re-connected to arm by same or subsequent user
		string response;
		cout << "Would You Like To Restart Calibration or Continue Listening?";
		cin >> response;
		while response = (Yes || YES || yes || y || Y){
			collector.calibrateData();
			collector.listenforGesture();
			cout << "Would You Like To Restart Calibration or Continue Listening?";
			cin >> response;
		}
		else{
		collector.listenforGesture();
		}
		system("cls");
		std::cout << "\n\n\n \t\t Please re-calibrate Terminator Myo for best results!" << std::endl;
		collector.holdState();
		//goto Recalibrate;

		// Tidy up & End program
		//Exit:
		/*std::cout << "Saving Data... " << std::endl;
		system("cls");
		Sleep(3000);
		return 0;*/

	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		std::cerr << "Press enter to continue.";
		std::cin.ignore();
		return 1;
	}
}
