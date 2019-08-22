/**
 * @file keyboard_offboard_control.cpp
 * @brief Example that demonstrates offboard control with keyboard by using body frame
 *
 * @authors Author: Burak Han Corak <burakhancorak@gmail.com>,
 * @date 2019-07-29
 */

#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <iostream>

#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/offboard/offboard.h>
#include <mavsdk/plugins/telemetry/telemetry.h>

#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include "openCV_TCP_Example.h"

using namespace mavsdk;
using std::this_thread::sleep_for;
using std::chrono::milliseconds;
using std::chrono::seconds;
using namespace std;

#define ERROR_CONSOLE_TEXT "\033[31m" // Turn text on console red
#define TELEMETRY_CONSOLE_TEXT "\033[34m" // Turn text on console blue
#define NORMAL_CONSOLE_TEXT "\033[0m" // Restore normal console colour

// Handles Action's result
inline void action_error_exit(Action::Result result, const std::string& message)
{
    if (result != Action::Result::SUCCESS) {
        std::cerr << ERROR_CONSOLE_TEXT << message << Action::result_str(result)
                  << NORMAL_CONSOLE_TEXT << std::endl;
        exit(EXIT_FAILURE);
    }
}

// Handles Offboard's result
inline void offboard_error_exit(Offboard::Result result, const std::string& message)
{
    if (result != Offboard::Result::SUCCESS) {
        std::cerr << ERROR_CONSOLE_TEXT << message << Offboard::result_str(result)
                  << NORMAL_CONSOLE_TEXT << std::endl;
        exit(EXIT_FAILURE);
    }
}

// Handles connection result
inline void connection_error_exit(ConnectionResult result, const std::string& message)
{
    if (result != ConnectionResult::SUCCESS) {
        std::cerr << ERROR_CONSOLE_TEXT << message << connection_result_str(result)
                  << NORMAL_CONSOLE_TEXT << std::endl;
        exit(EXIT_FAILURE);
    }
}

// Logs during Offboard control
inline void offboard_log(const std::string& offb_mode, const std::string msg)
{
    std::cout << "[" << offb_mode << "] " << msg << std::endl;
}


void usage(std::string bin_name)
{
    std::cout << NORMAL_CONSOLE_TEXT << "Usage : " << bin_name << " <connection_url>" << std::endl
              << "Connection URL format should be :" << std::endl
              << " For TCP : tcp://[server_host][:server_port]" << std::endl
              << " For UDP : udp://[bind_host][:bind_port]" << std::endl
              << " For Serial : serial:///path/to/serial/dev[:baudrate]" << std::endl
              << "For example, to connect to the simulator use URL: udp://:14540" << std::endl;
}

int main(int argc, char** argv)
{
    Mavsdk dc;
    std::string connection_url;
    ConnectionResult connection_result;

    if (argc == 2) {
        connection_url = argv[1];
        connection_result = dc.add_any_connection(connection_url);
    } else {
        usage(argv[0]);
        return 1;
    }

    if (connection_result != ConnectionResult::SUCCESS) {
        std::cout << ERROR_CONSOLE_TEXT
                  << "Connection failed: " << connection_result_str(connection_result)
                  << NORMAL_CONSOLE_TEXT << std::endl;
        return 1;
    }

    // Wait for the system to connect via heartbeat
    while (!dc.is_connected()) {
        std::cout << "Wait for system to connect via heartbeat" << std::endl;
        sleep_for(seconds(1));
    }

    // System got discovered.
    System& system = dc.system();
    auto action = std::make_shared<Action>(system);
    auto offboard = std::make_shared<Offboard>(system);
    auto telemetry = std::make_shared<Telemetry>(system);

    while (!telemetry->health_all_ok()) {
        std::cout << "Waiting for system to be ready" << std::endl;
        sleep_for(seconds(1));
    }
    std::cout << "System is ready" << std::endl;

    Action::Result arm_result = action->arm();
    action_error_exit(arm_result, "Arming failed");
    std::cout << "Armed" << std::endl;

    Action::Result takeoff_result = action->takeoff();
    action_error_exit(takeoff_result, "Takeoff failed");
    std::cout << "In Air..." << std::endl;
    sleep_for(seconds(10));


     struct termios oldSettings, newSettings;
    bool should_exit = false;
    tcgetattr( fileno( stdin ), &oldSettings );
    newSettings = oldSettings;
    newSettings.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr( fileno( stdin ), TCSANOW, &newSettings );    

	const std::string offb_mode = "BODY";

    // Send it once before starting offboard, otherwise it will be rejected.
    offboard->set_velocity_body({0.0f, 0.0f, 0.0f, 0.0f});

    Offboard::Result offboard_result = offboard->start();
	offboard_error_exit(offboard_result, "Offboard start failed");



    //grab the port number
    int port = 8081;
    //buffer to send and receive messages with
    char msg[1500];
     
    //setup a socket and connection tools
    sockaddr_in servAddr;
    bzero((char*)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);
 
    //open stream oriented socket with internet address
    //also keep track of the socket descriptor
    int serverSd = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSd < 0)
    {
        //cerr << "Error establishing the server socket" << endl;
        exit(0);
    }
    //bind the socket to its local address
    int bindStatus = bind(serverSd, (struct sockaddr*) &servAddr, 
        sizeof(servAddr));
    if(bindStatus < 0)
    {
       // cerr << "Error binding socket to local address" << endl;
        exit(0);
    }
    cout << "Waiting for a client to connect..." << endl;
    //listen for up to 5 requests at a time
    listen(serverSd, 5);
    //receive a request from client using accept
    //we need a new address to connect with the client
    sockaddr_in newSockAddr;
    socklen_t newSockAddrSize = sizeof(newSockAddr);
    //accept, create a new socket descriptor to 
    //handle the new connection with client
    int newSd = accept(serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize);
    if(newSd < 0)
    {
      //  cerr << "Error accepting request from client!" << endl;
        exit(1);
    }
    cout << "Connected with client!" << endl;
    //lets keep track of the session time

    gettimeofday(&start1, NULL);
    //also keep track of the amount of data sent as well


       



   while ( !should_exit )
    {

	memset(&msg, 0, sizeof(msg));//clear the buffer
	bytesRead += recv(newSd, (char*)&msg, sizeof(msg), 0);

	cout << "Client: " << msg << endl;
	char * pch;
    pch = strtok (msg," ,");
	while (pch != NULL)
	{
		cout << "Client: " << pch << endl;
		pch = strtok (NULL, " , ");
	}		
		
     
    }

    
    close(newSd);
    close(serverSd);
   

    sleep_for(seconds(2));

    offboard_result = offboard->stop();
    offboard_error_exit(offboard_result, "Offboard stop failed: ");
    offboard_log(offb_mode, "Offboard stopped");

    const Action::Result land_result = action->land();
    action_error_exit(land_result, "Landing failed");

    // Check if vehicle is still in air
    while (telemetry->in_air()) {
        std::cout << "Vehicle is landing..." << std::endl;
        sleep_for(seconds(1));
    }
    std::cout << "Landed!" << std::endl;

    // We are relying on auto-disarming but let's keep watching the telemetry for a bit longer.
    sleep_for(seconds(3));
    std::cout << "Finished..." << std::endl;

    return EXIT_SUCCESS;
}
