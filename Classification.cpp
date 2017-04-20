
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
	vector<int> RefData (8,0);	//vector to hold reference data (for average of 8 pods)
	vector<int> RefData_Total;	//vector to hold all of reference data (for variance)
	
	const int MAXSIZE = 4;		//reads in, at most, 4 characters
    	char thisVal[MAXSIZE];		//char array to hold individual values from ref data
	char new_line;	
	int counter = 0;	//keeps track of how many additions occur (how many lines are processed per gesture)
	double Pod1_avg, Pod2_avg, Pod3_avg, Pod4_avg, Pod5_avg, Pod6_avg, Pod7_avg, Pod8_avg = 0;//avg values for pods (per gesture)
	double Pod1_var, Pod2_var, Pod3_var, Pod4_var, Pod5_var, Pod6_var, Pod7_var, Pod8_var = 0;//variance for pods (per gesture) 
	vector<double> RefRest_avg(8,0), RefIndex_avg(8,0), RefMiddle_avg(8,0), RefRing_avg(8,0), RefPinky_avg(8,0), RefHand_avg(8,0);	//avg values for gestures for calibration
	vector<double> RefRest_var(8,0), RefIndex_var(8,0), RefMiddle_var(8,0), RefRing_var(8,0), RefPinky_var(8,0), RefHand_var(8,0);	//avg values for variance gestures for calibration
	vector<double> Rest_avg(8,0), Index_avg(8,0), Middle_avg(8,0), Ring_avg(8,0), Pinky_avg(8,0), Hand_avg(8,0);	//avg values for gestures for real time
	vector<double> Rest_var(8,0), Index_var(8,0), Middle_var(8,0), Ring_var(8,0), Pinky_var(8,0), Hand_var(8,0);	//avg values for variance gestures for real time
	
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
				RefRest_avg.at(0) = RefRest_avg.at(0) + Pod1_avg;
				RefRest_avg.at(1) = RefRest_avg.at(1) + Pod2_avg;
				RefRest_avg.at(2) = RefRest_avg.at(2) + Pod3_avg;
				RefRest_avg.at(3) = RefRest_avg.at(3) + Pod4_avg;
				RefRest_avg.at(4) = RefRest_avg.at(4) + Pod5_avg;
				RefRest_avg.at(5) = RefRest_avg.at(5) + Pod6_avg;
				RefRest_avg.at(6) = RefRest_avg.at(6) + Pod7_avg;
				RefRest_avg.at(7) = RefRest_avg.at(7) + Pod8_avg;
				RefRest_var.at(0) = RefRest_var.at(0) + Pod1_var;
				RefRest_var.at(1) = RefRest_var.at(1) + Pod2_var;
				RefRest_var.at(2) = RefRest_var.at(2) + Pod3_var;
				RefRest_var.at(3) = RefRest_var.at(3) + Pod4_var;
				RefRest_var.at(4) = RefRest_var.at(4) + Pod5_var;
				RefRest_var.at(5) = RefRest_var.at(5) + Pod6_var;
				RefRest_var.at(6) = RefRest_var.at(6) + Pod7_var;
				RefRest_var.at(7) = RefRest_var.at(7) + Pod8_var;
				}
			else if (a=1){	//Gesture is thumb
				RefThumb_avg.at(0) = RefThumb_avg.at(0) + Pod1_avg;
				RefThumb_avg.at(1) = RefThumb_avg.at(1) + Pod2_avg;
				RefThumb_avg.at(2) = RefThumb_avg.at(2) + Pod3_avg;
				RefThumb_avg.at(3) = RefThumb_avg.at(3) + Pod4_avg;
				RefThumb_avg.at(4) = RefThumb_avg.at(4) + Pod5_avg;
				RefThumb_avg.at(5) = RefThumb_avg.at(5) + Pod6_avg;
				RefThumb_avg.at(6) = RefThumb_avg.at(6) + Pod7_avg;
				RefThumb_avg.at(7) = RefThumb_avg.at(7) + Pod8_avg;
				RefThumb_var.at(0) = RefThumb_var.at(0) + Pod1_var;
				RefThumb_var.at(1) = RefThumb_var.at(1) + Pod2_var;
				RefThumb_var.at(2) = RefThumb_var.at(2) + Pod3_var;
				RefThumb_var.at(3) = RefThumb_var.at(3) + Pod4_var;
				RefThumb_var.at(4) = RefThumb_var.at(4) + Pod5_var;
				RefThumb_var.at(5) = RefThumb_var.at(5) + Pod6_var;
				RefThumb_var.at(6) = RefThumb_var.at(6) + Pod7_var;
				RefThumb_var.at(7) = RefThumb_var.at(7) + Pod8_var;
				}
			else if (a=2){	//Gesture is rest
				RefRest_avg.at(0) = RefRest_avg.at(0) + Pod1_avg;
				RefRest_avg.at(1) = RefRest_avg.at(1) + Pod2_avg;
				RefRest_avg.at(2) = RefRest_avg.at(2) + Pod3_avg;
				RefRest_avg.at(3) = RefRest_avg.at(3) + Pod4_avg;
				RefRest_avg.at(4) = RefRest_avg.at(4) + Pod5_avg;
				RefRest_avg.at(5) = RefRest_avg.at(5) + Pod6_avg;
				RefRest_avg.at(6) = RefRest_avg.at(6) + Pod7_avg;
				RefRest_avg.at(7) = RefRest_avg.at(7) + Pod8_avg;
				RefRest_var.at(0) = RefRest_var.at(0) + Pod1_var;
				RefRest_var.at(1) = RefRest_var.at(1) + Pod2_var;
				RefRest_var.at(2) = RefRest_var.at(2) + Pod3_var;
				RefRest_var.at(3) = RefRest_var.at(3) + Pod4_var;
				RefRest_var.at(4) = RefRest_var.at(4) + Pod5_var;
				RefRest_var.at(5) = RefRest_var.at(5) + Pod6_var;
				RefRest_var.at(6) = RefRest_var.at(6) + Pod7_var;
				RefRest_var.at(7) = RefRest_var.at(7) + Pod8_var;
				}
			else if (a=3){	//Gesture is index
				RefIndex_avg.at(0) = RefIndex_avg.at(0) + Pod1_avg;
				RefIndex_avg.at(1) = RefIndex_avg.at(1) + Pod2_avg;
				RefIndex_avg.at(2) = RefIndex_avg.at(2) + Pod3_avg;
				RefIndex_avg.at(3) = RefIndex_avg.at(3) + Pod4_avg;
				RefIndex_avg.at(4) = RefIndex_avg.at(4) + Pod5_avg;
				RefIndex_avg.at(5) = RefIndex_avg.at(5) + Pod6_avg;
				RefIndex_avg.at(6) = RefIndex_avg.at(6) + Pod7_avg;
				RefIndex_avg.at(7) = RefIndex_avg.at(7) + Pod8_avg;
				RefIndex_var.at(0) = RefIndex_var.at(0) + Pod1_var;
				RefIndex_var.at(1) = RefIndex_var.at(1) + Pod2_var;
				RefIndex_var.at(2) = RefIndex_var.at(2) + Pod3_var;
				RefIndex_var.at(3) = RefIndex_var.at(3) + Pod4_var;
				RefIndex_var.at(4) = RefIndex_var.at(4) + Pod5_var;
				RefIndex_var.at(5) = RefIndex_var.at(5) + Pod6_var;
				RefIndex_var.at(6) = RefIndex_var.at(6) + Pod7_var;
				RefIndex_var.at(7) = RefIndex_var.at(7) + Pod8_var;
				}
			else if (a=4){	//Gesture is rest
				RefRest_avg.at(0) = RefRest_avg.at(0) + Pod1_avg;
				RefRest_avg.at(1) = RefRest_avg.at(1) + Pod2_avg;
				RefRest_avg.at(2) = RefRest_avg.at(2) + Pod3_avg;
				RefRest_avg.at(3) = RefRest_avg.at(3) + Pod4_avg;
				RefRest_avg.at(4) = RefRest_avg.at(4) + Pod5_avg;
				RefRest_avg.at(5) = RefRest_avg.at(5) + Pod6_avg;
				RefRest_avg.at(6) = RefRest_avg.at(6) + Pod7_avg;
				RefRest_avg.at(7) = RefRest_avg.at(7) + Pod8_avg;
				RefRest_var.at(0) = RefRest_var.at(0) + Pod1_var;
				RefRest_var.at(1) = RefRest_var.at(1) + Pod2_var;
				RefRest_var.at(2) = RefRest_var.at(2) + Pod3_var;
				RefRest_var.at(3) = RefRest_var.at(3) + Pod4_var;
				RefRest_var.at(4) = RefRest_var.at(4) + Pod5_var;
				RefRest_var.at(5) = RefRest_var.at(5) + Pod6_var;
				RefRest_var.at(6) = RefRest_var.at(6) + Pod7_var;
				RefRest_var.at(7) = RefRest_var.at(7) + Pod8_var;
				}
			else if (a=5){	//Gesture is middle
				RefMiddle_avg.at(0) = RefMiddle_avg.at(0) + Pod1_avg;
				RefMiddle_avg.at(1) = RefMiddle_avg.at(1) + Pod2_avg;
				RefMiddle_avg.at(2) = RefMiddle_avg.at(2) + Pod3_avg;
				RefMiddle_avg.at(3) = RefMiddle_avg.at(3) + Pod4_avg;
				RefMiddle_avg.at(4) = RefMiddle_avg.at(4) + Pod5_avg;
				RefMiddle_avg.at(5) = RefMiddle_avg.at(5) + Pod6_avg;
				RefMiddle_avg.at(6) = RefMiddle_avg.at(6) + Pod7_avg;
				RefMiddle_avg.at(7) = RefMiddle_avg.at(7) + Pod8_avg;
				RefMiddle_var.at(0) = RefMiddle_var.at(0) + Pod1_var;
				RefMiddle_var.at(1) = RefMiddle_var.at(1) + Pod2_var;
				RefMiddle_var.at(2) = RefMiddle_var.at(2) + Pod3_var;
				RefMiddle_var.at(3) = RefMiddle_var.at(3) + Pod4_var;
				RefMiddle_var.at(4) = RefMiddle_var.at(4) + Pod5_var;
				RefMiddle_var.at(5) = RefMiddle_var.at(5) + Pod6_var;
				RefMiddle_var.at(6) = RefMiddle_var.at(6) + Pod7_var;
				RefMiddle_var.at(7) = RefMiddle_var.at(7) + Pod8_var;
				}
			else if (a=6){	//Gesture is rest
				RefRest_avg.at(0) = RefRest_avg.at(0) + Pod1_avg;
				RefRest_avg.at(1) = RefRest_avg.at(1) + Pod2_avg;
				RefRest_avg.at(2) = RefRest_avg.at(2) + Pod3_avg;
				RefRest_avg.at(3) = RefRest_avg.at(3) + Pod4_avg;
				RefRest_avg.at(4) = RefRest_avg.at(4) + Pod5_avg;
				RefRest_avg.at(5) = RefRest_avg.at(5) + Pod6_avg;
				RefRest_avg.at(6) = RefRest_avg.at(6) + Pod7_avg;
				RefRest_avg.at(7) = RefRest_avg.at(7) + Pod8_avg;
				RefRest_var.at(0) = RefRest_var.at(0) + Pod1_var;
				RefRest_var.at(1) = RefRest_var.at(1) + Pod2_var;
				RefRest_var.at(2) = RefRest_var.at(2) + Pod3_var;
				RefRest_var.at(3) = RefRest_var.at(3) + Pod4_var;
				RefRest_var.at(4) = RefRest_var.at(4) + Pod5_var;
				RefRest_var.at(5) = RefRest_var.at(5) + Pod6_var;
				RefRest_var.at(6) = RefRest_var.at(6) + Pod7_var;
				RefRest_var.at(7) = RefRest_var.at(7) + Pod8_var;
				}
			else if (a=7){	//Gesture is ring
				RefRing_avg.at(0) = RefRing_avg.at(0) + Pod1_avg;
				RefRing_avg.at(1) = RefRing_avg.at(1) + Pod2_avg;
				RefRing_avg.at(2) = RefRing_avg.at(2) + Pod3_avg;
				RefRing_avg.at(3) = RefRing_avg.at(3) + Pod4_avg;
				RefRing_avg.at(4) = RefRing_avg.at(4) + Pod5_avg;
				RefRing_avg.at(5) = RefRing_avg.at(5) + Pod6_avg;
				RefRing_avg.at(6) = RefRing_avg.at(6) + Pod7_avg;
				RefRing_avg.at(7) = RefRing_avg.at(7) + Pod8_avg;
				RefRing_var.at(0) = RefRing_var.at(0) + Pod1_var;
				RefRing_var.at(1) = RefRing_var.at(1) + Pod2_var;
				RefRing_var.at(2) = RefRing_var.at(2) + Pod3_var;
				RefRing_var.at(3) = RefRing_var.at(3) + Pod4_var;
				RefRing_var.at(4) = RefRing_var.at(4) + Pod5_var;
				RefRing_var.at(5) = RefRing_var.at(5) + Pod6_var;
				RefRing_var.at(6) = RefRing_var.at(6) + Pod7_var;
				RefRing_var.at(7) = RefRing_var.at(7) + Pod8_var;
				}
			else if (a=8){	//Gesture is rest
				RefRest_avg.at(0) = RefRest_avg.at(0) + Pod1_avg;
				RefRest_avg.at(1) = RefRest_avg.at(1) + Pod2_avg;
				RefRest_avg.at(2) = RefRest_avg.at(2) + Pod3_avg;
				RefRest_avg.at(3) = RefRest_avg.at(3) + Pod4_avg;
				RefRest_avg.at(4) = RefRest_avg.at(4) + Pod5_avg;
				RefRest_avg.at(5) = RefRest_avg.at(5) + Pod6_avg;
				RefRest_avg.at(6) = RefRest_avg.at(6) + Pod7_avg;
				RefRest_avg.at(7) = RefRest_avg.at(7) + Pod8_avg;
				RefRest_var.at(0) = RefRest_var.at(0) + Pod1_var;
				RefRest_var.at(1) = RefRest_var.at(1) + Pod2_var;
				RefRest_var.at(2) = RefRest_var.at(2) + Pod3_var;
				RefRest_var.at(3) = RefRest_var.at(3) + Pod4_var;
				RefRest_var.at(4) = RefRest_var.at(4) + Pod5_var;
				RefRest_var.at(5) = RefRest_var.at(5) + Pod6_var;
				RefRest_var.at(6) = RefRest_var.at(6) + Pod7_var;
				RefRest_var.at(7) = RefRest_var.at(7) + Pod8_var;
				}
			else if (a=9){	//Gesture is pinky
				RefPinky_avg.at(0) = RefPinky_avg.at(0) + Pod1_avg;
				RefPinky_avg.at(1) = RefPinky_avg.at(1) + Pod2_avg;
				RefPinky_avg.at(2) = RefPinky_avg.at(2) + Pod3_avg;
				RefPinky_avg.at(3) = RefPinky_avg.at(3) + Pod4_avg;
				RefPinky_avg.at(4) = RefPinky_avg.at(4) + Pod5_avg;
				RefPinky_avg.at(5) = RefPinky_avg.at(5) + Pod6_avg;
				RefPinky_avg.at(6) = RefPinky_avg.at(6) + Pod7_avg;
				RefPinky_avg.at(7) = RefPinky_avg.at(7) + Pod8_avg;
				RefPinky_var.at(0) = RefPinky_var.at(0) + Pod1_var;
				RefPinky_var.at(1) = RefPinky_var.at(1) + Pod2_var;
				RefPinky_var.at(2) = RefPinky_var.at(2) + Pod3_var;
				RefPinky_var.at(3) = RefPinky_var.at(3) + Pod4_var;
				RefPinky_var.at(4) = RefPinky_var.at(4) + Pod5_var;
				RefPinky_var.at(5) = RefPinky_var.at(5) + Pod6_var;
				RefPinky_var.at(6) = RefPinky_var.at(6) + Pod7_var;
				RefPinky_var.at(7) = RefPinky_var.at(7) + Pod8_var;
				}
			else if (a=10){	//Gesture is rest
				RefRest_avg.at(0) = RefRest_avg.at(0) + Pod1_avg;
				RefRest_avg.at(1) = RefRest_avg.at(1) + Pod2_avg;
				RefRest_avg.at(2) = RefRest_avg.at(2) + Pod3_avg;
				RefRest_avg.at(3) = RefRest_avg.at(3) + Pod4_avg;
				RefRest_avg.at(4) = RefRest_avg.at(4) + Pod5_avg;
				RefRest_avg.at(5) = RefRest_avg.at(5) + Pod6_avg;
				RefRest_avg.at(6) = RefRest_avg.at(6) + Pod7_avg;
				RefRest_avg.at(7) = RefRest_avg.at(7) + Pod8_avg;
				RefRest_var.at(0) = RefRest_var.at(0) + Pod1_var;
				RefRest_var.at(1) = RefRest_var.at(1) + Pod2_var;
				RefRest_var.at(2) = RefRest_var.at(2) + Pod3_var;
				RefRest_var.at(3) = RefRest_var.at(3) + Pod4_var;
				RefRest_var.at(4) = RefRest_var.at(4) + Pod5_var;
				RefRest_var.at(5) = RefRest_var.at(5) + Pod6_var;
				RefRest_var.at(6) = RefRest_var.at(6) + Pod7_var;
				RefRest_var.at(7) = RefRest_var.at(7) + Pod8_var;
				}
			else if (a=11){	//Gesture is hand
				RefHand_avg.at(0) = RefHand_avg.at(0) + Pod1_avg;
				RefHand_avg.at(1) = RefHand_avg.at(1) + Pod2_avg;
				RefHand_avg.at(2) = RefHand_avg.at(2) + Pod3_avg;
				RefHand_avg.at(3) = RefHand_avg.at(3) + Pod4_avg;
				RefHand_avg.at(4) = RefHand_avg.at(4) + Pod5_avg;
				RefHand_avg.at(5) = RefHand_avg.at(5) + Pod6_avg;
				RefHand_avg.at(6) = RefHand_avg.at(6) + Pod7_avg;
				RefHand_avg.at(7) = RefHand_avg.at(7) + Pod8_avg;
				RefHand_var.at(0) = RefHand_var.at(0) + Pod1_var;
				RefHand_var.at(1) = RefHand_var.at(1) + Pod2_var;
				RefHand_var.at(2) = RefHand_var.at(2) + Pod3_var;
				RefHand_var.at(3) = RefHand_var.at(3) + Pod4_var;
				RefHand_var.at(4) = RefHand_var.at(4) + Pod5_var;
				RefHand_var.at(5) = RefHand_var.at(5) + Pod6_var;
				RefHand_var.at(6) = RefHand_var.at(6) + Pod7_var;
				RefHand_var.at(7) = RefHand_var.at(7) + Pod8_var;
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
			vector<int> LiveData (8,0)
			vector<int> LiveData_Total;	//vector to hold all of the real time data
			vector<int> LiveData_Var (8);	//vector to hold all of temp_var_sq (for variance)
			vector<double> Live_Rest_avg(8,0), Live_Index_avg(8,0), Live_Middle_avg(8,0), Live_Ring_avg(8,0), Live_Pinky_avg(8,0), Live_Hand_avg(8,0);	//avg values for gestures for calibration
			vector<double> Live_Rest_var(8,0), Live_Index_var(8,0), Live_Middle_var(8,0), Live_Ring_var(8,0), Live_Pinky_var(8,0), Live_Hand_var(8,0);	//avg values for variance gestures for calibration
			double LivePod1_avg, LivePod2_avg, LivePod3_avg, LivePod4_avg, LivePod5_avg, LivePod6_avg, LivePod7_avg, LivePod8_avg = 0;//avg values for pods (real time)
			double LivePod1_var, LivePod2_var, LivePod3_var, LivePod4_var, LivePod5_var, LivePod6_var, LivePod7_var, LivePod8_var = 0;//variance for pods (real time) 
	
			int Quill, avg_ticker, var_ticker, Livetemp, Livetemp_sq = 0;
			// Get current CPU time
			double beginTime = GetTickCount();
			double nowTime = GetTickCount() - beginTime;
			bool pod_found = false;
			
			while ( (nowTime <= 120000){	//listens for 2 minutes
				while (pod_found = false){
					20_frames(beginTime, nowTime, pod_found);
					}
				while (pod_found = true){
					50_frames(beginTime, nowTime, pod_found, LiveData, LiveData_Total, LiveData_Var, 
		       				LivePod1_avg, LivePod2_avg, LivePod3_avg, LivePod4_avg, LivePod5_avg , LivePod6_avg,
		       				LivePod7_avg, LivePod8_avg, LivePod1_var, LivePod2_var, LivePod3_var, LivePod4_var, 
		       				LivePod5_var, LivePod6_var, LivePod7_var, LivePod8_var, Livetemp, Livetemp_sq);
					}
				nowTime = GetTickCount() - beginTime;
				} 
		}

	void holdState(){
		while (!onArm){}
		}

	void 20_frames(beginTime, &nowTime, &pod_found){	//20 frame intervals (looking for active pods)
		
		//all variables have limited scope, because data is only recorded in 50 frame function
		vector<int> LiveData (8,0)
		vector<int> LiveData_Total;	//vector to hold all of the real time data
		vector<int> LiveData_Var (8);	//vector to hold all of temp_var_sq (for variance)
		double LivePod1_avg, LivePod2_avg, LivePod3_avg, LivePod4_avg, LivePod5_avg, LivePod6_avg, LivePod7_avg, LivePod8_avg = 0;//avg values for pods (real time)
		double LivePod1_var, LivePod2_var, LivePod3_var, LivePod4_var, LivePod5_var, LivePod6_var, LivePod7_var, LivePod8_var = 0;//variance for pods (real time) 
		int Quill, avg_ticker, var_ticker, Livetemp, Livetemp_sq = 0;
		
		//20 frame intervals (looking for active pods)
		while (nowTime <= 4000 & pod_found = false){	
			for (int i = 0; i < 8; i++) {
				Quill = emg[i];
				LiveData.at(i) = LiveData.at(i) + Quill;
				LiveData_Total.pushback(Quill);
				LiveData.at(i) = LiveData.at(i) + Quill;	//Pods summed 
				avg_ticker++;
				}
	
		//Pods averaged (real time)
		LivePod1_avg=LiveData.at(0)/avg_ticker;	
		LivePod2_avg=LiveData.at(1)/avg_ticker;
		LivePod3_avg=LiveData.at(2)/avg_ticker;
		LivePod4_avg=LiveData.at(3)/avg_ticker;
		LivePod5_avg=LiveData.at(4)/avg_ticker;
		LivePod6_avg=LiveData.at(5)/avg_ticker;
		LivePod7_avg=LiveData.at(6)/avg_ticker;
		LivePod8_avg=LiveData.at(7)/avg_ticker;

		//Find Pod Variance
		for(int x = 0; x < LiveData_Total.size(); x + 8){	//grabs every 8th entry (Pod 1)
			Livetemp = LiveData_Total.at(x) - LivePod1_avg;
			Livetemp_sq = Livetemp * Livetemp;
			LiveData_Var.at(0) = LiveData_Var.at(0) + Livetemp_sq;	//pools var data for Pod 1
			var_ticker++;		//keeps track of how many additions occur (how many variances are added)
			}
			LivePod1_var = LiveData_Var.at(0)/var_ticker;
			var_ticker = 0;
		for(int x = 1; x < LiveData_Total.size(); x + 8){	//grabs every 8th entry (Pod 2)
			Livetemp = LiveData_Total.at(x) - LivePod2_avg;
			Livetemp_sq = Livetemp * Livetemp;
			LiveData_Var.at(1) = LiveData_Var.at(1) + Livetemp_sq;	
			var_ticker++;		
			}
			LivePod2_var = LiveData_Var.at(1)/var_ticker;
			var_ticker = 0;
		for(int x = 2; x < LiveData_Total.size(); x + 8){	//grabs every 8th entry (Pod 3)
			Livetemp = LiveData_Total.at(x) - LivePod3_avg;
			Livetemp_sq = Livetemp * Livetemp;
			LiveData_Var.at(2) = LiveData_Var.at(2) + Livetemp_sq;	
			var_ticker++;		
			}
			LivePod3_var = LiveData_Var.at(2)/var_ticker;
			var_ticker = 0;
		for(int x = 3; x < LiveData_Total.size(); x + 8){	//grabs every 8th entry (Pod 4)
			Livetemp = LiveData_Total.at(x) - LivePod4_avg;
			Livetemp_sq = Livetemp * Livetemp;
			LiveData_Var.at(3) = LiveData_Var.at(3) + Livetemp_sq;	
			var_ticker++;		
			}
			LivePod4_var = LiveData_Var.at(3)/var_ticker;
			var_ticker = 0;
		for(int x = 4; x < LiveData_Total.size(); x + 8){	//grabs every 8th entry (Pod 5)
			Livetemp = LiveData_Total.at(x) - LivePod5_avg;
			Livetemp_sq = Livetemp * Livetemp;
			LiveData_Var.at(4) = LiveData_Var.at(4) + Livetemp_sq;	
			var_ticker++;		
			}
			LivePod5_var = LiveData_Var.at(4)/var_ticker;
			var_ticker = 0;
		for(int x = 5; x < LiveData_Total.size(); x + 8){	//grabs every 8th entry (Pod 6)
			Livetemp = LiveData_Total.at(x) - LivePod6_avg;
			Livetemp_sq = Livetemp * Livetemp;
			LiveData_Var.at(5) = LiveData_Var.at(5) + Livetemp_sq;	
			var_ticker++;		
			}
			LivePod6_var = LiveData_Var.at(5)/var_ticker;
			var_ticker = 0;
		for(int x = 6; x < LiveData_Total.size(); x + 8){	//grabs every 8th entry (Pod 7)
			Livetemp = LiveData_Total.at(x) - LivePod7_avg;
			Livetemp_sq = Livetemp * Livetemp;
			LiveData_Var.at(6) = LiveData_Var.at(6) + Livetemp_sq;	
			var_ticker++;		
			}
			LivePod7_var = LiveData_Var.at(6)/var_ticker;
			var_ticker = 0;
		for(int x = 7; x < LiveData_Total.size(); x + 8){	//grabs every 8th entry (Pod 8)
			Livetemp = LiveData_Total.at(x) - LivePod8_avg;
			Livetemp_sq = Livetemp * Livetemp;
			LiveData_Var.at(7) = LiveData_Var.at(7) + Livetemp_sq;	
			var_ticker++;		
			}
			LivePod8_var = LiveData_Var.at(7)/var_ticker;
			var_ticker = 0;
		
		LiveData_Var.at(0) = LivePod1_var;
		LiveData_Var.at(1) = LivePod2_var;
		LiveData_Var.at(2) = LivePod3_var;
		LiveData_Var.at(3) = LivePod4_var;
		LiveData_Var.at(4) = LivePod5_var;
		LiveData_Var.at(5) = LivePod6_var;
		LiveData_Var.at(6) = LivePod7_var;
		LiveData_Var.at(7) = LivePod8_var;
				
		for (int r=0; r <8; r++){
			if (LiveData_Var.at(r) > 20){		//found active pod
				/*LiveData_Var.assign(8,0);	//resets vectors and Pods
				LiveData_Total.erase(LiveData_Total.begin(), LiveData_Total.end());
				LivePod1 = 0;
				LivePod2 = 0;
				LivePod3 = 0;
				LivePod4 = 0;
				LivePod5 = 0;
				LivePod6 = 0;
				LivePod7 = 0;
				LivePod8 = 0;*/
		
				//jump to 50 frame function
				pod_found = true;
				}
			}
		nowTime = GetTickCount() - beginTime;
		}	
	}
	
	void 50_frames(beginTime, &nowTime, &pod_found, &LiveData, &LiveData_Total, &LiveData_Var, 
		       &LivePod1_avg, &LivePod2_avg, &LivePod3_avg, &LivePod4_avg, &LivePod5_avg , &LivePod6_avg,
		       &LivePod7_avg, &LivePod8_avg, &LivePod1_var, &LivePod2_var, &LivePod3_var, &LivePod4_var, 
		       &LivePod5_var, &LivePod6_var, &LivePod7_var, &LivePod8_var, &Livetemp, &Livetemp_sq){
	//50 frame intervals (found active pods, recording data)
	int Quill, avg_ticker, var_ticker, Livetemp, Livetemp_sq = 0;
	while (nowTime <= 10000 & pod_found = true){
		for (int i = 0; i < 8; i++) {
			Quill = emg[i];
			LiveData.at(i) = LiveData.at(i) + Quill;
			LiveData_Total.pushback(Quill);
			LiveData.at(i) = LiveData.at(i) + Quill;	//Pods summed 
			avg_ticker++;
			}
		//Pods averaged (real time)
		LivePod1_avg=LiveData.at(0)/avg_ticker;	
		LivePod2_avg=LiveData.at(1)/avg_ticker;
		LivePod3_avg=LiveData.at(2)/avg_ticker;
		LivePod4_avg=LiveData.at(3)/avg_ticker;
		LivePod5_avg=LiveData.at(4)/avg_ticker;
		LivePod6_avg=LiveData.at(5)/avg_ticker;
		LivePod7_avg=LiveData.at(6)/avg_ticker;
		LivePod8_avg=LiveData.at(7)/avg_ticker;

		//Find Pod Variance
		for(int x = 0; x < LiveData_Total.size(); x + 8){	//grabs every 8th entry (Pod 1)
			Livetemp = LiveData_Total.at(x) - LivePod1_avg;
			Livetemp_sq = Livetemp * Livetemp;
			LiveData_Var.at(0) = LiveData_Var.at(0) + Livetemp_sq;	//pools var data for Pod 1
			var_ticker++;		//keeps track of how many additions occur (how many variances are added)
			}
			LivePod1_var = LiveData_Var.at(0)/var_ticker;
			var_ticker = 0;
		for(int x = 1; x < LiveData_Total.size(); x + 8){	//grabs every 8th entry (Pod 2)
			Livetemp = LiveData_Total.at(x) - LivePod2_avg;
			Livetemp_sq = Livetemp * Livetemp;
			LiveData_Var.at(1) = LiveData_Var.at(1) + Livetemp_sq;	
			var_ticker++;		
			}
			LivePod2_var = LiveData_Var.at(1)/var_ticker;
			var_ticker = 0;
		for(int x = 2; x < LiveData_Total.size(); x + 8){	//grabs every 8th entry (Pod 3)
			Livetemp = LiveData_Total.at(x) - LivePod3_avg;
			Livetemp_sq = Livetemp * Livetemp;
			LiveData_Var.at(2) = LiveData_Var.at(2) + Livetemp_sq;	
			var_ticker++;		
			}
			LivePod3_var = LiveData_Var.at(2)/var_ticker;
			var_ticker = 0;
		for(int x = 3; x < LiveData_Total.size(); x + 8){	//grabs every 8th entry (Pod 4)
			Livetemp = LiveData_Total.at(x) - LivePod4_avg;
			Livetemp_sq = Livetemp * Livetemp;
			LiveData_Var.at(3) = LiveData_Var.at(3) + Livetemp_sq;	
			var_ticker++;		
			}
			LivePod4_var = LiveData_Var.at(3)/var_ticker;
			var_ticker = 0;
		for(int x = 4; x < LiveData_Total.size(); x + 8){	//grabs every 8th entry (Pod 5)
			Livetemp = LiveData_Total.at(x) - LivePod5_avg;
			Livetemp_sq = Livetemp * Livetemp;
			LiveData_Var.at(4) = LiveData_Var.at(4) + Livetemp_sq;	
			var_ticker++;		
			}
			LivePod5_var = LiveData_Var.at(4)/var_ticker;
			var_ticker = 0;
		for(int x = 5; x < LiveData_Total.size(); x + 8){	//grabs every 8th entry (Pod 6)
			Livetemp = LiveData_Total.at(x) - LivePod6_avg;
			Livetemp_sq = Livetemp * Livetemp;
			LiveData_Var.at(5) = LiveData_Var.at(5) + Livetemp_sq;	
			var_ticker++;		
			}
			LivePod6_var = LiveData_Var.at(5)/var_ticker;
			var_ticker = 0;
		for(int x = 6; x < LiveData_Total.size(); x + 8){	//grabs every 8th entry (Pod 7)
			Livetemp = LiveData_Total.at(x) - LivePod7_avg;
			Livetemp_sq = Livetemp * Livetemp;
			LiveData_Var.at(6) = LiveData_Var.at(6) + Livetemp_sq;	
			var_ticker++;		
			}
			LivePod7_var = LiveData_Var.at(6)/var_ticker;
			var_ticker = 0;
		for(int x = 7; x < LiveData_Total.size(); x + 8){	//grabs every 8th entry (Pod 8)
			Livetemp = LiveData_Total.at(x) - LivePod8_avg;
			Livetemp_sq = Livetemp * Livetemp;
			LiveData_Var.at(7) = LiveData_Var.at(7) + Livetemp_sq;	
			var_ticker++;		
			}
			LivePod8_var = LiveData_Var.at(7)/var_ticker;
			var_ticker = 0;
		
		LiveData_Var.at(0) = LivePod1_var;
		LiveData_Var.at(1) = LivePod2_var;
		LiveData_Var.at(2) = LivePod3_var;
		LiveData_Var.at(3) = LivePod4_var;
		LiveData_Var.at(4) = LivePod5_var;
		LiveData_Var.at(5) = LivePod6_var;
		LiveData_Var.at(6) = LivePod7_var;
		LiveData_Var.at(7) = LivePod8_var;
		nowTime = GetTickCount() - beginTime;
		}
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
