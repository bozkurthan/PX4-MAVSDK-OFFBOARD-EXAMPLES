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
#include <X11/Xlib.h>

#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/offboard/offboard.h>
#include <mavsdk/plugins/telemetry/telemetry.h>

using namespace mavsdk;
using std::this_thread::sleep_for;
using std::chrono::milliseconds;
using std::chrono::seconds;

#define ERROR_CONSOLE_TEXT "\033[31m" // Turn text on console red
#define TELEMETRY_CONSOLE_TEXT "\033[34m" // Turn text on console blue
#define NORMAL_CONSOLE_TEXT "\033[0m" // Restore normal console colour

constexpr const char* key_name[] = {
    "first",
    "second (or middle)",
    "third",
    "fourth",  // :D
    "fivth"    // :|
};

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

    Display *display;
    XEvent xevent;
    Window window;

    if( (display = XOpenDisplay(NULL)) == NULL )
        return -1;


    window = DefaultRootWindow(display);
    XAllowEvents(display, AsyncBoth, CurrentTime);

    XGrabPointer(display, 
                 window,
                 1, 
                 PointerMotionMask | ButtonPressMask | ButtonReleaseMask , 
                 GrabModeAsync,
                 GrabModeAsync, 
                 None,
                 None,
                 CurrentTime);


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
    bool _exit_buttonpress=true;
    bool button1_check=false;
    bool button2_check=false;
    bool button3_check=false;
    tcgetattr( fileno( stdin ), &oldSettings );
    newSettings = oldSettings;
    newSettings.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr( fileno( stdin ), TCSANOW, &newSettings );    

	const std::string offb_mode = "BODY";

    // Send it once before starting offboard, otherwise it will be rejected.
    offboard->set_velocity_body({0.0f, 0.0f, 0.0f, 0.0f});

    Offboard::Result offboard_result = offboard->start();
	offboard_error_exit(offboard_result, "Offboard start failed");
   while ( !should_exit )
    {

       fd_set set;
               struct timeval tv;

               tv.tv_sec = 0;
               tv.tv_usec = 400;

               FD_ZERO( &set );
               FD_SET( fileno( stdin ), &set );

               int res = select( fileno( stdin )+1, &set, NULL, NULL, &tv );

               if( res > 0 )
               {
                   char c;

                                   read( fileno( stdin ), &c, 1 );
                                switch(c)
                                   {
                                case 'P':
                                case 'p':
                                      std::cout << "P was pressed. Exiting... \n";
                                      should_exit = true;
                                      break;
                                default:
                                    std::cout << "Other input \n";


                                }
               }
               else if( res < 0 )
               {
                   perror( "select error" );
                   break;
               }
               else
               {
                   XNextEvent(display, &xevent);
                   if(xevent.type == ButtonPress)
                   {
                       _exit_buttonpress=false;
                       if(xevent.xbutton.button - 1 == 0)
                       {
                           button1_check=true;
                       }
                       if(xevent.xbutton.button - 1 == 2)
                       {
                           button2_check=true;
                       }
                       if(xevent.xbutton.button - 1 == 1)
                       {
                           if(button3_check==true)
                               button3_check= false;
                           else
                               button3_check= true;
                       }
                   }

                   if(_exit_buttonpress == false)
                   {
                                  if(button1_check)
                                  {
                                          if(xevent.xmotion.x_root-960>=0 && xevent.xmotion.y_root-540>=0){
                                          offboard->set_velocity_body({-(xevent.xmotion.y_root-540)/108.0f, (xevent.xmotion.x_root-960)/192.0f, 0.0f, 0.0f});
                                          printf("Horizontal Y: %f, Horizontal X: %f \n",-(xevent.xmotion.y_root-540)/108.0f, (xevent.xmotion.x_root-960)/192.0f);
                                          }

                                          else if(xevent.xmotion.x_root-960 >= 0  && xevent.xmotion.y_root-540<0)
                                          {
                                          offboard->set_velocity_body({-(xevent.xmotion.y_root-540)/108.0f, (xevent.xmotion.x_root-960)/192.0f, 0.0f, 0.0f});
                                          printf("Horizontal Y: %f, Horizontal X: %f \n",-(xevent.xmotion.y_root-540)/108.0f, (xevent.xmotion.x_root-960)/192.0f);
                                          }

                                          else if(xevent.xmotion.x_root-960<=0 && xevent.xmotion.y_root-540<=0)
                                          {
                                          offboard->set_velocity_body({-(xevent.xmotion.y_root-540)/108.0f, (xevent.xmotion.x_root-960)/192.0f, 0.0f, 0.0f});
                                          printf("Horizontal Y: %f, Horizontal X: %f \n",-(xevent.xmotion.y_root-540)/108.0f, (xevent.xmotion.x_root-960)/192.0f);
                                          }

                                          else
                                          {
                                          offboard->set_velocity_body({-(xevent.xmotion.y_root-540)/108.0f, (xevent.xmotion.x_root-960)/192.0f, 0.0f, 0.0f});
                                          printf("Horizontal Y: %f, Horizontal X: %f \n",-(xevent.xmotion.y_root-540)/108.0f, (xevent.xmotion.x_root-960)/192.0f);
                                          }


                                  }

                                  if(button2_check)
                                  {
                                      if(xevent.xmotion.x_root-960>=0 && xevent.xmotion.y_root-540>=0){
                                      offboard->set_velocity_body({0.0f, 0.0f,(xevent.xmotion.y_root-540)/108.0f, (xevent.xmotion.x_root-960)/21.33f});
                                      printf("Vertical: %f, Yaw: %f \n",(xevent.xmotion.y_root-540)/108.0f, (xevent.xmotion.x_root-960)/21.33f);
                                      }

                                      else if(xevent.xmotion.x_root-960 >= 0  && xevent.xmotion.y_root-540<0)
                                      {
                                      offboard->set_velocity_body({0.0f, 0.0f,(xevent.xmotion.y_root-540)/108.0f, (xevent.xmotion.x_root-960)/21.33f});
                                      printf("Vertical: %f, Yaw:%f \n",(xevent.xmotion.y_root-540)/108.0f, (xevent.xmotion.x_root-960)/21.33f);
                                      }

                                      else if(xevent.xmotion.x_root-960<=0 && xevent.xmotion.y_root-540<=0)
                                      {
                                      offboard->set_velocity_body({0.0f, 0.0f,(xevent.xmotion.y_root-540)/108.0f, (xevent.xmotion.x_root-960)/21.33f});
                                      printf("Vertical: %f, Yaw:%f \n",(xevent.xmotion.y_root-540)/108.0f, (xevent.xmotion.x_root-960)/21.33f);
                                      }

                                      else
                                      {
                                      offboard->set_velocity_body({0.0f, 0.0f,(xevent.xmotion.y_root-540)/108.0f, (xevent.xmotion.x_root-960)/21.33f});
                                      printf("Vertical: %f, Yaw:%f \n",(xevent.xmotion.y_root-540)/108.0f, (xevent.xmotion.x_root-960)/21.33f);
                                      }

                                  }

                                  else if(xevent.xbutton.button - 1 == 1)
                                  {
                                  printf("mid");
                                  }
                      }

                   if(xevent.type == ButtonRelease)
                   {

                           if(xevent.xbutton.button - 1 == 0)
                           {
                               button1_check=false;
                           }
                           if(xevent.xbutton.button - 1 == 2)
                           {
                               button2_check=false;
                           }
                           if(!button1_check && !button2_check)
                           {
                               _exit_buttonpress = true;
                               if(button3_check==false)
                               {
                               offboard->set_velocity_body({ 0.0f, 0.0f, 0.0f, 0.0f});
                               }
                           }
                       }

               }


    }

    tcsetattr( fileno( stdin ), TCSANOW, &oldSettings );

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
