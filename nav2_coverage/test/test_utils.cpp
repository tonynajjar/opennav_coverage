// Copyright (c) 2023 Open Navigation LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "gtest/gtest.h"
#include "rclcpp/rclcpp.hpp"
#include "nav2_coverage/utils.hpp"
#include "tf2/utils.h"

// Luckily, F2C has very high test coverage so we only need to test what we touch

class RosLockGuard
{
public:
  RosLockGuard() {rclcpp::init(0, nullptr);}
  ~RosLockGuard() {rclcpp::shutdown();}
};
RosLockGuard g_rclcpp;

namespace nav2_coverage
{

TEST(UtilsTests, TestpointToPoint32)
{
  geometry_msgs::msg::Point32 pt_in;
  pt_in.x = 1.0;
  pt_in.y = 1.1;
  pt_in.z = 1.2;
  geometry_msgs::msg::Point pt_out = util::pointToPoint32(pt_in);
  EXPECT_NEAR(pt_out.x, 1.0, 1e-6);
  EXPECT_NEAR(pt_out.y, 1.1, 1e-6);
  EXPECT_NEAR(pt_out.z, 1.2, 1e-6);
}

TEST(UtilsTests, TestpointToMsg)
{
  Point pt_in{1.0, 2.0, 3.0};
  geometry_msgs::msg::Point32 pt_out = util::toMsg(pt_in);
  EXPECT_EQ(pt_out.x, 1.0);
  EXPECT_EQ(pt_out.y, 2.0);
  EXPECT_EQ(pt_out.z, 3.0);
}

TEST(UtilsTests, TestpathStateToMsg)
{
  PathState state_in;
  state_in.point = Point(1.0, 2.0, 3.0);
  state_in.angle = M_PI;

  geometry_msgs::msg::PoseStamped pose_out = util::toMsg(state_in);
  EXPECT_EQ(pose_out.pose.position.x, 1.0);
  EXPECT_EQ(pose_out.pose.position.y, 2.0);
  EXPECT_EQ(pose_out.pose.position.z, 3.0);
  EXPECT_NEAR(tf2::getYaw(pose_out.pose.orientation), M_PI, 0.01);
}

TEST(UtilsTests, TesttoUpper)
{
  std::string test_str = "hi";
  util::toUpper(test_str);
  EXPECT_EQ(test_str, std::string("HI"));
}

TEST(UtilsTests, TesttoCoveragePathMsg1)
{
  bool ordered = false;
  std_msgs::msg::Header header_in;
  header_in.frame_id = "test";
  Swaths swaths_in;

  swaths_in.push_back(Swath());
  swaths_in.push_back(Swath());
  swaths_in.push_back(Swath());

  auto msg = util::toCoveragePathMsg(swaths_in, ordered, header_in);
  EXPECT_EQ(msg.header.frame_id, "test");
  EXPECT_EQ(msg.swaths_ordered, false);
  EXPECT_EQ(msg.swaths.size(), 3u);
  EXPECT_EQ(msg.contains_turns, false);

  ordered = true;
  swaths_in = Swaths();
  msg = util::toCoveragePathMsg(swaths_in, ordered, header_in);
  EXPECT_EQ(msg.swaths_ordered, true);
  EXPECT_EQ(msg.swaths.size(), 0u);
  EXPECT_EQ(msg.contains_turns, false);
}

TEST(UtilsTests, TesttoNavPathMsg)
{
  std_msgs::msg::Header header_in;
  header_in.frame_id = "test";
  Path path_in;
  path_in.states.resize(10);

  auto msg = util::toNavPathMsg(path_in, header_in);
  EXPECT_EQ(msg.header.frame_id, "test");
  EXPECT_EQ(msg.poses.size(), 10u);
}

TEST(UtilsTests, TesttoCoveragePathMsg2)
{
  std_msgs::msg::Header header_in;
  header_in.frame_id = "test";
  Path path_in;
  auto msg = util::toCoveragePathMsg(path_in, header_in);
  EXPECT_EQ(msg.swaths.size(), 0u);
  EXPECT_EQ(msg.turns.size(), 0u);
  EXPECT_EQ(msg.contains_turns, true);
  EXPECT_EQ(msg.swaths_ordered, true);

  // states are not valid (e.g. non-marked as turn or swath)
  path_in.states.resize(10);
  EXPECT_THROW(util::toCoveragePathMsg(path_in, header_in), std::runtime_error);

  // Now lets make it valid
  using f2c::types::PathSectionType;
  path_in.states[0].type = PathSectionType::SWATH;
  path_in.states[1].type = PathSectionType::SWATH;
  path_in.states[2].type = PathSectionType::TURN;
  path_in.states[3].type = PathSectionType::TURN;
  path_in.states[4].type = PathSectionType::SWATH;
  path_in.states[5].type = PathSectionType::SWATH;
  path_in.states[6].type = PathSectionType::TURN;
  path_in.states[7].type = PathSectionType::TURN;
  path_in.states[8].type = PathSectionType::SWATH;
  path_in.states[9].type = PathSectionType::SWATH;

  msg = util::toCoveragePathMsg(path_in, header_in);
  EXPECT_EQ(msg.swaths.size(), 3u);
  EXPECT_EQ(msg.turns.size(), 2u);
  EXPECT_EQ(msg.turns[0].poses.size(), 2u);

  path_in.states[0].type = PathSectionType::TURN;
  path_in.states[1].type = PathSectionType::TURN;
  path_in.states[2].type = PathSectionType::SWATH;
  path_in.states[3].type = PathSectionType::SWATH;
  path_in.states[4].type = PathSectionType::TURN;
  path_in.states[5].type = PathSectionType::TURN;
  path_in.states[6].type = PathSectionType::SWATH;
  path_in.states[7].type = PathSectionType::SWATH;
  path_in.states[8].type = PathSectionType::TURN;
  path_in.states[9].type = PathSectionType::TURN;

  msg = util::toCoveragePathMsg(path_in, header_in);
  EXPECT_EQ(msg.swaths.size(), 2u);
  EXPECT_EQ(msg.turns.size(), 3u);
  EXPECT_EQ(msg.turns[0].poses.size(), 2u);
}

TEST(UtilsTests, TestgetFieldFromGoal)
{
  auto goal = std::make_shared<typename ComputeCoveragePath::Goal>();

  // No polygons set
  EXPECT_THROW(util::getFieldFromGoal(goal), std::invalid_argument);

  // Polygons set invalid
  goal->polygons.resize(1);
  goal->polygons[0].coordinates.resize(3);
  goal->polygons[0].coordinates[2].axis1 = 1.0;
  EXPECT_THROW(util::getFieldFromGoal(goal), std::invalid_argument);

  // Should work now, with a trivial polygon of 3 nodes of (0, 0)
  goal->polygons[0].coordinates[2].axis1 = 0.0;
  auto field = util::getFieldFromGoal(goal);
  EXPECT_EQ(field.getGeometry(0).size(), 3u);

  // Test with inner polygons, first invalid
  goal->polygons.resize(2);
  goal->polygons[1].coordinates.resize(3);
  goal->polygons[1].coordinates[2].axis1 = 1.0;
  EXPECT_THROW(util::getFieldFromGoal(goal), std::invalid_argument);

  // Now valid
  goal->polygons[0].coordinates[0].axis1 = 0.0;
  goal->polygons[1].coordinates[0].axis1 = 1.0;
  field = util::getFieldFromGoal(goal);
  EXPECT_EQ(field.getGeometry(1).size(), 3u);
}

}  // namespace nav2_coverage
