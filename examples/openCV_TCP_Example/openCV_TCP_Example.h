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

 struct timeval start1, end1;
    int bytesRead = 0;
    bool shouldexit = true;
