//
//  Classification.cpp
//  Terminator Arm
//
//  Created by Cory Bethrant
//
//

//
//  main.cpp
//  Terminator Arm V2
//
//  Copyright © 2017 Team Terminator. All rights reserved.
//

#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "myo.hpp"

using namespace std;
class DataCollector : public myo::DeviceListener {
public:
    DataCollector()
    : emgSamples()
    {
    }
    // onUnpair() is called whenever the Myo is disconnected from Myo Connect by the user.
    void onUnpair(myo::Myo* myo, uint64_t timestamp)
    {
        // We've lost a Myo.
        // Let's clean up some leftover state.
        emgSamples.fill(0);
    }
    // onEmgData() is called whenever a paired Myo has provided new EMG data, and EMG streaming is enabled.
    void onEmgData(myo::Myo* myo, uint64_t timestamp, const int8_t* emg)
    {
        for (int i = 0; i < 8; i++) {
            emgSamples[i] = emg[i];
        }
    }
    // There are other virtual functions in DeviceListener that we could override here, like onAccelerometerData().
    // For this example, the functions overridden above are sufficient.
    // We define this function to print the current values that were updated by the on...() functions above.
    void print()
    {
        // Clear the current line
        cout << '\r';
        // Print out the EMG data.
        for (size_t i = 0; i < emgSamples.size(); i++) {
            ostringstream oss;
            oss << static_cast<int>(emgSamples[i]);
            string emgString = oss.str();
            cout << '[' << emgString << string(4 - emgString.size(), ' ') << ']';
        }
        cout << flush;
    }
    // The values of this array is set by onEmgData() above.
    ::array<int8_t, 8> emgSamples;
};
int main(int argc, char** argv)
{
    // We catch any exceptions that might occur below -- see the catch statement for more details.
    try {
        // First, we create a Hub with our application identifier. Be sure not to use the com.example namespace when
        // publishing your application. The Hub provides access to one or more Myos.
        myo::Hub hub("com.example.emg-data-sample");
        cout << "Attempting to find a Myo..." << endl;
        // Next, we attempt to find a Myo to use. If a Myo is already paired in Myo Connect, this will return that Myo
        // immediately.
        // waitForMyo() takes a timeout value in milliseconds. In this case we will try to find a Myo for 10 seconds, and
        // if that fails, the function will return a null pointer.
        myo::Myo* myo = hub.waitForMyo(10000);
        // If waitForMyo() returned a null pointer, we failed to find a Myo, so exit with an error message.
        if (!myo) {
            throw runtime_error("Unable to find a Myo!");
        }
        // We've found a Myo.
        cout << "Connected to a Myo armband!" << endl << endl;
        // Next we enable EMG streaming on the found Myo.
        myo->setStreamEmg(myo::Myo::streamEmgEnabled);
        // Next we construct an instance of our DeviceListener, so that we can register it with the Hub.
        DataCollector collector;
        // Hub::addListener() takes the address of any object whose class inherits from DeviceListener, and will cause
        // Hub::run() to send events to all registered device listeners.
        hub.addListener(&collector);
        // Finally we enter our main loop.
        while (1) {
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

            hub.run(1000/20);
            // After processing events, we call the print() member function we defined above to print out the values we've
            // obtained from any events that have occurred.
            collector.print();
                cout << "Current Gesture: ";
        }
        // If a standard exception occurred, we print out its message and exit.
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        cerr << "Press enter to continue.";
        cin.ignore();
        return 1;
    }
}


