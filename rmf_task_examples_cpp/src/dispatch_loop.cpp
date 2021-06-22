/**
 *  These is the C++ script for triggering a SubmitTask(Loop) Service Call
 *  
 *  `ros2 run rmf_task_examples_cpp loop_dispatch -s <start> -f <finish> -n <loop_num> --ros-args -p use_sim_time:=true`  
 */

#include "rclcpp/rclcpp.hpp"
#include "unistd.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "rmf_task_msgs/msg/loop.hpp"
#include "rmf_task_msgs/srv/submit_task.hpp"
#include "rmf_task_msgs/msg/priority.hpp"
#include "rmf_task_msgs/msg/task_description.hpp"
#include "rmf_task_msgs/msg/task_type.hpp"

/**
 *  These is the script "help" function
 *  The help message will show the various command line inputs for the loop dispatch task script
 *  For the script to work the "start", "finish" and "loop_num" inputs are required
 */
static void show_usage(std::string name) {
    std::cerr << "Usage: " << name << "\n"
              << "Options:\n"
              << "\t-h,--help\t\tShow this help message\n"
              << "\t-s,--start\t\tSpecify the start waypoint\n"
              << "\t-f,--finish\t\tSpecify the end waypoint\n"
              << "\t-n,--loop_num\t\tSpecify the number of loops\n"
            //   << "\t--use_sim_time\t\tSpecify to use sim time, Default false\n"
              << std::endl;
}

int main(int argc, char **argv) {
    
    /**
     *  This checks the number of inputs given
     */
    if (argc < 10) {
        std::cerr << "You might have missing input variables" << std::endl;
        show_usage(argv[0]);
        return 1;
    }

    /**
     * Initialising storage variables to used  
     */
    std::string start;
    std::string finish;
    int loop;
    // bool simulation = false;

    /**
     * Saving parsed command line input values into stored variables
     */
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if ((arg == "-h") || (arg == "--help")) {
            show_usage(argv[0]);
            return 0;
        }
        if ((arg == "-s") || (arg == "--start")) {
            start = argv[++i];
            continue;
        }
        if ((arg == "-f") || (arg == "--finish")) {
            finish = argv[++i];
            continue;
        }
        if ((arg == "-n") || (arg == "--loop_num")) {
            std::stringstream num(argv[++i]); 
            num >> loop;
            continue;
        }
        // if ((arg == "--use_sim_time")) {
        //     simulation = true;
        //     continue;
        // }
    }

    rclcpp::init(argc, argv);

    /**
     * [Currently this method is not working, issue has been raised to OSRC]
     * Setting use_sim_time variable 
     */
    // rclcpp::Parameter sim_time("use_sim_time", rclcpp::ParameterValue(simulation));
    // TaskRequester->set_parameter(sim_time);

    /**
     *  Initialising the ROS2 node and creating the service client instance
     */
    auto TaskRequester = rclcpp::Node::make_shared("task_requester");
    auto SubmitTaskClient = TaskRequester->create_client<rmf_task_msgs::srv::SubmitTask>("submit_task");
    
    /**
     *  Checking if Service is available in network, service is provided by the dispatcher node in RoMi-H
     */
    while (!SubmitTaskClient->wait_for_service(std::chrono::seconds(1))) {
        if (!rclcpp::ok()) {
            RCLCPP_ERROR(TaskRequester->get_logger(), "client interrupted while waiting for service to appear.");
            return 1;
        }
        RCLCPP_INFO(TaskRequester->get_logger(), "waiting for service to appear...");
    }    

    /**
     *  Initialing the SubmitTask components
     */
    auto request = std::make_shared<rmf_task_msgs::srv::SubmitTask::Request>();
    auto description = rmf_task_msgs::msg::TaskDescription();
    auto priority = rmf_task_msgs::msg::Priority();
    auto tasktype = rmf_task_msgs::msg::TaskType();
    auto task = rmf_task_msgs::msg::Loop();

    /**
     *  Setting the required values
     *  
     *  priority.value is set to default 0
     * 
     *  tasktype is set to TYPE_LOOP
     * 
     *  task: 1) only 'start_name', 'finish_name', 'num_loops' are required
     *        2) 'task_id' and 'robottype' are not needed  as these are decided by the RoMi-H Dispatcher node
     */
    priority.value = 0;
    
    tasktype.type = rmf_task_msgs::msg::TaskType().TYPE_LOOP; 
    
    task.start_name = start;
    task.finish_name = finish;
    task.num_loops = loop;
    
    description.start_time = TaskRequester->now();
    description.priority = priority;
    description.task_type = tasktype;
    description.loop = task;
    
    request->description = description;
    request->requester = "cpp_loop_dispatcher_script";

    /**
     *  Sending 'request' as the ROS2 service call 
     *  and waiting for an acknowledgement from service node
     */
    RCLCPP_INFO(TaskRequester->get_logger(), "Submitting Loop Request");
    auto result_future = SubmitTaskClient->async_send_request(request);
    if (rclcpp::spin_until_future_complete(TaskRequester, result_future) !=
            rclcpp::FutureReturnCode::SUCCESS)
        {
            RCLCPP_ERROR(TaskRequester->get_logger(), "service call failed :");
            return 1;
        }

    /**
     *  Checking service call return response
     */
    auto result = result_future.get();
    if (!(result->success))
    {
        RCLCPP_INFO(TaskRequester->get_logger(), result->message); 
        RCLCPP_INFO(TaskRequester->get_logger(), result->task_id);
        return 1;
    }
    RCLCPP_INFO(TaskRequester->get_logger(), "service call successful, the task ID to watch is '%s'", (result->task_id).c_str());
    rclcpp::shutdown();
    return 0;
}