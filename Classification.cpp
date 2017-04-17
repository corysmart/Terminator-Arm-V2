
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

	// We define this function to write the current values that were updated by the on...() functions above
	void writeData(std::string gesture)
	{
		//Open infile for reference data
		TerminatorRefFile.open("Reference.csv");
		
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

			// Real-time gesture-recognition goes here 
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
		// Data succesfully logged, start Calibration!
		system("cls");
		std::cout << "\n\n\n \t\t Now calibrating data..." << std::endl;
		collector.calibrateData(); // Get general code working(User exceptions, sync test), use FANN/ openNN http://opennn.cimne.com/ , explore tics.

		// Calibration complete! listen for gestures for as long as HUTerminator myo remains synced with arm
		system("cls");
		std::cout << "\n\n\n \t\t Now listening for gestures..." << std::endl;
		collector.listenforGesture();

		// Ask to re-calibrate, when re-connected to arm by same or subsequent user
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
