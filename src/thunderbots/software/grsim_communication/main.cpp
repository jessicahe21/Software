#include <ros/ros.h>
#include <thunderbots_msgs/Primitive.h>
#include <thunderbots_msgs/PrimitiveArray.h>
#include <thunderbots_msgs/World.h>

<<<<<<< HEAD
<<<<<<< HEAD
#include "ai/primitive/move_primitive.h"
#include "ai/primitive/chip_primitive.h"
#include "ai/primitive/kick_primitive.h"
=======
>>>>>>> b20161dc21a1bd66f3c4e146f1399408ab1a16f6
=======
#include "ai/primitive/chip_primitive.h"
>>>>>>> 2e64dafe0f472a05db6aefebd9d16952abd142b8
#include "ai/primitive/primitive.h"
#include "ai/primitive/primitive_factory.h"
#include "grsim_communication/grsim_backend.h"
#include "util/constants.h"
#include "util/logger/init.h"
#include "util/parameter/dynamic_parameter_utils.h"
#include "util/parameter/dynamic_parameters.h"
#include "util/ros_messages.h"

// Member variables we need to maintain state
// They are kept in an anonymous namespace so they are not accessible outside this
// file and are not created as global static variables.
namespace
{
    // The GrSimBackend responsible for handling communication with grSim
    GrSimBackend grsim_backend(Util::Constants::GRSIM_COMMAND_NETWORK_ADDRESS,
                               Util::Constants::GRSIM_COMMAND_NETWORK_PORT);
    // The current state of the world
    World world;
}  // namespace

void primitiveUpdateCallback(const thunderbots_msgs::PrimitiveArray::ConstPtr& msg)
{
    std::vector<std::unique_ptr<Primitive>> primitives;
    thunderbots_msgs::PrimitiveArray prim_array_msg = *msg;
    for (const thunderbots_msgs::Primitive& prim_msg : prim_array_msg.primitives)
    {
        primitives.emplace_back(AI::Primitive::createPrimitiveFromROSMessage(prim_msg));
    }

    grsim_backend.sendPrimitives(primitives, world.friendlyTeam(), world.ball());
}

void worldUpdateCallback(const thunderbots_msgs::World::ConstPtr& msg)
{
    thunderbots_msgs::World world_msg = *msg;
    world = Util::ROSMessages::createWorldFromROSMessage(world_msg);
}

int main(int argc, char** argv)
{
    // Init ROS node
    ros::init(argc, argv, "grsim_communication");
    ros::NodeHandle node_handle;

    // Create subscribers to topics we care about
    ros::Subscriber primitive_subscriber = node_handle.subscribe(
        Util::Constants::AI_PRIMITIVES_TOPIC, 1, primitiveUpdateCallback);
    ros::Subscriber world_subscriber = node_handle.subscribe(
        Util::Constants::NETWORK_INPUT_WORLD_TOPIC, 1, worldUpdateCallback);

    // Initialize the logger
    Util::Logger::LoggerSingleton::initializeLogger(node_handle);

<<<<<<< HEAD
<<<<<<< HEAD
    // Initialize variables
    primitives                 = std::vector<std::unique_ptr<Primitive>>();
    GrSimBackend grsim_backend = GrSimBackend(NETWORK_ADDRESS, NETWORK_PORT);

    // We loop at a set rate so that we don't overload the network with too many packets
    ros::Rate tick_rate(TICK_RATE);

    // Main loop
    while (ros::ok())
    {
        // Clear all primitives each tick
        primitives.clear();

        // Spin once to let all necessary callbacks run
        // The callbacks will populate the primitives vector
        ros::spinOnce();

        primitives.emplace_back(new ChipPrimitive(0, Point(0, 0), Angle::zero(), 3));
        primitives.emplace_back(new KickPrimitive(0, Point(0, 0), Angle::zero(), 1));

        grsim_backend.sendPrimitives(primitives, friendly_team);

        tick_rate.sleep();
    }
=======
=======
    // Initialize Dynamic Parameters
    auto update_subscribers =
        Util::DynamicParameters::initUpdateSubscriptions(node_handle);

>>>>>>> 2e64dafe0f472a05db6aefebd9d16952abd142b8
    // Services any ROS calls in a separate thread "behind the scenes". Does not return
    // until the node is shutdown
    // http://wiki.ros.org/roscpp/Overview/Callbacks%20and%20Spinning
    ros::spin();
>>>>>>> b20161dc21a1bd66f3c4e146f1399408ab1a16f6

    return 0;
}
