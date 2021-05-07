#include <iostream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "osi_common.pb.h"
#include "osi_object.pb.h"
#include "osi_sensorview.pb.h"
#include "osi_version.pb.h"
#include "esminiLib.hpp"
#include <vector>
#include <stdexcept>
#include <fstream>

#define _USE_MATH_DEFINES
#include <math.h>

class GetNumberOfObjectsTest : public ::testing::TestWithParam<std::tuple<std::string, int>>
{
};
// inp: scenario file
// expected: number of objects in the scenario

TEST_P(GetNumberOfObjectsTest, number_of_objects)
{

	std::string scenario_file = std::get<0>(GetParam());
	//std::string scenario_file = "../../esmini/resources/xosc/cut-in.xosc";

	const char *Scenario_file = scenario_file.c_str();
	SE_Init(Scenario_file, 0, 0, 0, 0);

	int n_Objects = SE_GetNumberOfObjects();
	SE_Close();

	EXPECT_EQ(n_Objects, std::get<1>(GetParam()));
	//EXPECT_EQ(n_Objects, 2);
}

INSTANTIATE_TEST_CASE_P(EsminiAPITests, GetNumberOfObjectsTest, ::testing::Values(std::make_tuple("../../../resources/xosc/cut-in.xosc", 2), std::make_tuple("../../../resources/xosc/highway_merge.xosc", 6), std::make_tuple("../../../resources/xosc/full_e6mini.xosc", 15)));

TEST(GetNumberOfObjectsTest, number_of_objects_no_init)
{

	int n_Objects = SE_GetNumberOfObjects();

	EXPECT_EQ(n_Objects, 0);
}

TEST(GetOSILaneBoundaryIdsTest, lane_boundary_ids)
{

	std::string scenario_file = "../../../resources/xosc/full_e6mini.xosc";

	SE_Init(scenario_file.c_str(), 0, 0, 0, 0);

	int n_Objects = SE_GetNumberOfObjects();
	EXPECT_EQ(n_Objects, 15);

	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();

	std::vector<std::vector<int>> lane_bound = {{10, 9, 8, -1},
												{0, 10, 9, 8},
												{1, 0, 10, 9},
												{2, 1, 0, 10},
												{3, 2, 1, 0},
												{11, 3, 2, 1},
												{4, 11, 3, 2},
												{3, 11, 4, 5}, //right side start
												{11, 4, 5, 6},
												{4, 5, 6, 7},
												{5, 6, 7, 12},
												{6, 7, 12, 13},
												{7, 12, 13, 14},
												{12, 13, 14, -1}};

	std::vector<int> veh_id = {14, 13, 12, 11, 10, 9, 8, 6, 5, 4, 3, 2, 1, 0};
	for (int i = 0; i < lane_bound.size(); i++)
	{
		SE_LaneBoundaryId lanes_id;
		lanes_id.far_left_lb_id = lane_bound[i][0];
		lanes_id.left_lb_id = lane_bound[i][1];
		lanes_id.right_lb_id = lane_bound[i][2];
		lanes_id.far_right_lb_id = lane_bound[i][3];
		SE_LaneBoundaryId ids;

		SE_GetOSILaneBoundaryIds(veh_id[i], &ids);

		EXPECT_EQ(ids.far_left_lb_id, lanes_id.far_left_lb_id);
		EXPECT_EQ(ids.left_lb_id, lanes_id.left_lb_id);
		EXPECT_EQ(ids.right_lb_id, lanes_id.right_lb_id);
		EXPECT_EQ(ids.far_right_lb_id, lanes_id.far_right_lb_id);
	}

	SE_Close();
}

TEST(GetOSILaneBoundaryIdsTest, lane_boundary_ids_no_obj)
{

	std::string scenario_file = "../../../resources/xosc/cut-in.xosc";
	const char *Scenario_file = scenario_file.c_str();
	SE_Init(Scenario_file, 0, 0, 0, 0);
	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();

	SE_LaneBoundaryId ids;
	SE_LaneBoundaryId right_lanes_id = {-1, -1, -1, -1};

	SE_GetOSILaneBoundaryIds(10, &ids);
	EXPECT_EQ(ids.far_left_lb_id, right_lanes_id.far_left_lb_id);
	EXPECT_EQ(ids.far_right_lb_id, right_lanes_id.far_right_lb_id);
	EXPECT_EQ(ids.left_lb_id, right_lanes_id.left_lb_id);
	EXPECT_EQ(ids.right_lb_id, right_lanes_id.right_lb_id);

	SE_Close();
}

TEST(OSIintersections, threeway)
{

	std::string scenario_file = "../../../EnvironmentSimulator/Unittest/scenarios/simple_3_way_intersection_osi.xosc";
	const char *Scenario_file = scenario_file.c_str();
	SE_Init(Scenario_file, 0, 0, 0, 0);
	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();
	int lanes_found = 0;
	bool intersection_found = false;
	osi3::GroundTruth osi_gt;
	int sv_size = 0;
	const char *gt = SE_GetOSIGroundTruth(&sv_size);
	osi_gt.ParseFromArray(gt, sv_size);
	for (int i = 0; i < osi_gt.lane_size(); i++)
	{
		if (osi_gt.mutable_lane(i)->mutable_classification()->type() == osi3::Lane_Classification_Type::Lane_Classification_Type_TYPE_INTERSECTION)
		{
			intersection_found = true;
			// should not exist a centerline
			ASSERT_EQ(osi_gt.mutable_lane(i)->mutable_classification()->centerline_size(), 0);
			ASSERT_EQ(osi_gt.mutable_lane(i)->mutable_classification()->lane_pairing_size(), 6);
			ASSERT_EQ(osi_gt.mutable_lane(i)->mutable_classification()->free_lane_boundary_id_size(), 3);
		}
		else
		{
			lanes_found++;
		}
	}
	ASSERT_TRUE(intersection_found);
	ASSERT_EQ(lanes_found, 6);
	SE_Close();
}

TEST(OSIintersections, fourway)
{

	std::string scenario_file = "../../../EnvironmentSimulator/Unittest/scenarios/simple_4_way_intersection_osi.xosc";
	const char *Scenario_file = scenario_file.c_str();
	SE_Init(Scenario_file, 0, 0, 0, 0);
	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();
	int lanes_found = 0;
	bool intersection_found = false;
	osi3::GroundTruth osi_gt;
	int sv_size = 0;
	const char *gt = SE_GetOSIGroundTruth(&sv_size);
	osi_gt.ParseFromArray(gt, sv_size);
	for (int i = 0; i < osi_gt.lane_size(); i++)
	{
		if (osi_gt.mutable_lane(i)->mutable_classification()->type() == osi3::Lane_Classification_Type::Lane_Classification_Type_TYPE_INTERSECTION)
		{
			intersection_found = true;
			// should not exist a centerline
			ASSERT_EQ(osi_gt.mutable_lane(i)->mutable_classification()->centerline_size(), 0);
			ASSERT_EQ(osi_gt.mutable_lane(i)->mutable_classification()->lane_pairing_size(), 12);
			ASSERT_EQ(osi_gt.mutable_lane(i)->mutable_classification()->free_lane_boundary_id_size(), 4);
		}
		else
		{
			lanes_found++;
		}
	}
	ASSERT_TRUE(intersection_found);
	ASSERT_EQ(lanes_found, 8);
	SE_Close();
}

TEST(OSIintersections, motorway)
{

	std::string scenario_file = "../../../EnvironmentSimulator/Unittest/scenarios/simple_motorway_osi_intersection.xosc";
	const char *Scenario_file = scenario_file.c_str();
	SE_Init(Scenario_file, 0, 0, 0, 0);
	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();
	int lanes_found = 0;
	bool intersection_found = false;
	osi3::GroundTruth osi_gt;
	int sv_size = 0;
	const char *gt = SE_GetOSIGroundTruth(&sv_size);
	osi_gt.ParseFromArray(gt, sv_size);
	for (int i = 0; i < osi_gt.lane_size(); i++)
	{
		if (osi_gt.mutable_lane(i)->mutable_classification()->type() == osi3::Lane_Classification_Type::Lane_Classification_Type_TYPE_INTERSECTION)
		{
			intersection_found = true;
		}
		else
		{
			lanes_found++;
		}
	}
	ASSERT_FALSE(intersection_found);
	ASSERT_EQ(lanes_found, 12);
	SE_Close();
}

TEST(OSIintersections, multilane)
{
	std::string scenario_file = "../../../EnvironmentSimulator/Unittest/scenarios/multilane_3way_intersection_osi.xosc";
	const char *Scenario_file = scenario_file.c_str();
	SE_Init(Scenario_file, 0, 0, 0, 0);
	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();
	int lanes_found = 0;
	bool intersection_found = false;
	osi3::GroundTruth osi_gt;
	int sv_size = 0;
	const char *gt = SE_GetOSIGroundTruth(&sv_size);
	osi_gt.ParseFromArray(gt, sv_size);
	for (int i = 0; i < osi_gt.lane_size(); i++)
	{
		if (osi_gt.mutable_lane(i)->mutable_classification()->type() == osi3::Lane_Classification_Type::Lane_Classification_Type_TYPE_INTERSECTION)
		{
			intersection_found = true;
			// should not exist a centerline
			ASSERT_EQ(osi_gt.mutable_lane(i)->mutable_classification()->centerline_size(), 0);
			ASSERT_EQ(osi_gt.mutable_lane(i)->mutable_classification()->lane_pairing_size(), 18);
			ASSERT_EQ(osi_gt.mutable_lane(i)->mutable_classification()->free_lane_boundary_id_size(), 3);
		}
		else
		{
			lanes_found++;
		}
	}
	ASSERT_TRUE(intersection_found);
	ASSERT_EQ(lanes_found, 18);

	SE_Close();
}

TEST(GetOSIRoadLaneTest, lane_no_obj)
{

	std::string scenario_file = "../../../resources/xosc/cut-in.xosc";
	const char *Scenario_file = scenario_file.c_str();

	SE_Init(Scenario_file, 0, 0, 0, 0);
	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();

	int road_lane_size;

	const char *road_lane = SE_GetOSIRoadLane(&road_lane_size, 15);

	EXPECT_EQ(road_lane_size, 0);
	EXPECT_EQ(road_lane, nullptr);

	SE_Close();
}

TEST(GetOSIRoadLaneTest, lane_id)
{

	std::string scenario_file = "../../../resources/xosc/full_e6mini.xosc";
	const char *Scenario_file = scenario_file.c_str();

	SE_Init(Scenario_file, 0, 0, 0, 0);
	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();

	std::vector<int> lanes = {0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14};
	std::vector<int> veh_id = {14, 13, 12, 11, 10, 9, 8, 6, 5, 4, 3, 2, 1, 0};

	int road_lane_size;
	osi3::Lane osi_lane;

	for (int i = 0; i < lanes.size(); i++)
	{
		int lane_id = lanes[i];

		const char *road_lane = SE_GetOSIRoadLane(&road_lane_size, veh_id[i]);
		osi_lane.ParseFromArray(road_lane, road_lane_size);

		EXPECT_EQ(osi_lane.id().value(), lane_id);
	}
	SE_Close();
}

TEST(GetOSIRoadLaneTest, left_lane_id)
{

	std::string scenario_file = "../../../resources/xosc/full_e6mini.xosc";
	const char *Scenario_file = scenario_file.c_str();
	SE_Init(Scenario_file, 0, 0, 0, 0);

	int n_Objects = SE_GetNumberOfObjects();
	EXPECT_EQ(n_Objects, 15);

	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();
	int road_lane_size;
	osi3::Lane osi_lane;

	// explicitly writing lanes ID so that it will be easy to adapt the test for more complex roads in the future
	std::vector<int> lanes = {0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14};
	std::vector<int> veh_id = {14, 13, 12, 11, 10, 9, 8, 6, 5, 4, 3, 2, 1, 0};

	for (int i = 0; i < lanes.size(); i++)
	{
		const char *road_lane = SE_GetOSIRoadLane(&road_lane_size, veh_id[i]);
		osi_lane.ParseFromArray(road_lane, road_lane_size);
		if (lanes[i] < 6)
		{
			EXPECT_EQ(osi_lane.classification().left_adjacent_lane_id(0).value(), lanes[i + 1]);
		}
		else if (lanes[i] == 6)
		{
			EXPECT_EQ(osi_lane.classification().left_adjacent_lane_id(0).value(), 8);
		}
		else if (lanes[i] == 8)
		{
			EXPECT_EQ(osi_lane.classification().left_adjacent_lane_id(0).value(), 6);
		}
		else
		{
			EXPECT_EQ(osi_lane.classification().left_adjacent_lane_id(0).value(), lanes[i - 1]);
		}
	}

	SE_Close();
}

TEST(GetOSIRoadLaneTest, right_lane_id)
{

	std::string scenario_file = "../../../resources/xosc/full_e6mini.xosc";
	const char *Scenario_file = scenario_file.c_str();

	SE_Init(Scenario_file, 0, 0, 0, 0);
	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();
	int road_lane_size;
	osi3::Lane osi_lane;

	// explicitly writing lanes ID so that it will be easy to adapt the test for more complex roads in the future
	std::vector<int> lanes = {0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14};
	std::vector<int> veh_id = {14, 13, 12, 11, 10, 9, 8, 6, 5, 4, 3, 2, 1, 0};

	for (int i = 0; i < lanes.size(); i++)
	{
		const char *road_lane = SE_GetOSIRoadLane(&road_lane_size, veh_id[i]);
		osi_lane.ParseFromArray(road_lane, road_lane_size);

		if (lanes[i] == 0)
		{
			EXPECT_EQ(osi_lane.classification().right_adjacent_lane_id_size(), 0);
		}
		else if (lanes[i] == 14)
		{
			EXPECT_EQ(osi_lane.classification().right_adjacent_lane_id_size(), 0);
		}
		else if (lanes[i] < 7)
		{
			EXPECT_EQ(osi_lane.classification().right_adjacent_lane_id(0).value(), lanes[i - 1]);
		}
		else
		{
			EXPECT_EQ(osi_lane.classification().right_adjacent_lane_id(0).value(), lanes[i + 1]);
		}
	}

	SE_Close();
}

TEST(GetOSIRoadLaneTest, right_lane_boundary_id)
{

	std::string scenario_file = "../../../resources/xosc/full_e6mini.xosc";
	const char *Scenario_file = scenario_file.c_str();

	SE_Init(Scenario_file, 0, 0, 0, 0);
	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();
	int road_lane_size;
	osi3::Lane osi_lane;

	// explicitly writing lanes ID so that it will be easy to adapt the test for more complex roads in the future
	std::vector<int> lane_bound = {8, 9, 10, 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14};
	std::vector<int> veh_id = {14, 13, 12, 11, 10, 9, 8, 6, 5, 4, 3, 2, 1, 0};

	for (int i = 0; i < lane_bound.size(); i++)
	{
		const char *road_lane = SE_GetOSIRoadLane(&road_lane_size, veh_id[i]);
		osi_lane.ParseFromArray(road_lane, road_lane_size);

		EXPECT_EQ(osi_lane.classification().right_lane_boundary_id(0).value(), lane_bound[i]);
	}

	SE_Close();
}

TEST(GetOSIRoadLaneTest, left_lane_boundary_id)
{

	std::string scenario_file = "../../../resources/xosc/full_e6mini.xosc";
	const char *Scenario_file = scenario_file.c_str();

	SE_Init(Scenario_file, 0, 0, 0, 0);
	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();
	int road_lane_size;
	osi3::Lane osi_lane;

	// explicitly writing lanes ID so that it will be easy to adapt the test for more complex roads in the future
	std::vector<int> lane_bound = {8, 9, 10, 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14};
	std::vector<int> veh_id = {14, 13, 12, 11, 10, 9, 8, 6, 5, 4, 3, 2, 1, 0};

	for (int i = 0; i < veh_id.size(); i++)
	{
		const char *road_lane = SE_GetOSIRoadLane(&road_lane_size, veh_id[i]);
		osi_lane.ParseFromArray(road_lane, road_lane_size);

		if (veh_id[i] == 6 || veh_id[i] == 8)
		{
			EXPECT_EQ(osi_lane.classification().left_lane_boundary_id(0).value(), 11);
		}
		else if (veh_id[i] > 7)
		{
			EXPECT_EQ(osi_lane.classification().left_lane_boundary_id(0).value(), lane_bound[i + 1]);
		}
		else
		{
			EXPECT_EQ(osi_lane.classification().left_lane_boundary_id(0).value(), lane_bound[i - 1]);
		}
	}

	SE_Close();
}

class GetOSIRoadLaneTest : public ::testing::TestWithParam<std::tuple<std::string, bool, bool>>
{
};
// inp: scenario file
// expected: bool defining if driving direction is the same of road definition

TEST_P(GetOSIRoadLaneTest, centerline_is_driving_direction)
{

	std::string scenario_file = std::get<0>(GetParam());
	const char *Scenario_file = scenario_file.c_str();

	SE_Init(Scenario_file, 0, 0, 0, 0);
	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();

	int road_lane_size;
	osi3::Lane osi_lane;

	// explicitly writing lanes ID so that it will be easy to adapt the test for more complex roads in the future
	std::vector<int> lanes = {0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14};
	std::vector<int> veh_id = {14, 13, 12, 11, 10, 9, 8, 6, 5, 4, 3, 2, 1, 0};

	for (int i = 0; i < lanes.size(); i++)
	{
		const char *road_lane = SE_GetOSIRoadLane(&road_lane_size, veh_id[i]);
		osi_lane.ParseFromArray(road_lane, road_lane_size);
		if (veh_id[i] <= 7)
		{
			EXPECT_EQ(osi_lane.classification().centerline_is_driving_direction(), std::get<1>(GetParam()));
		}
		else
		{
			EXPECT_EQ(osi_lane.classification().centerline_is_driving_direction(), std::get<2>(GetParam()));
		}
	}

	SE_Close();
}

INSTANTIATE_TEST_CASE_P(EsminiAPITests, GetOSIRoadLaneTest, ::testing::Values(std::make_tuple("../../../resources/xosc/full_e6mini.xosc", true, false), std::make_tuple("../../../resources/xosc/full_e6mini_reverse.xosc", true, false)));

TEST(GetOSIRoadLaneTest, is_host_vehicle_lane)
{

	std::string scenario_file = "../../../resources/xosc/full_e6mini.xosc";
	const char *Scenario_file = scenario_file.c_str();

	SE_Init(Scenario_file, 0, 0, 0, 0);
	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();
	int road_lane_size;
	osi3::Lane osi_lane;

	// explicitly writing lanes ID so that it will be easy to adapt the test for more complex roads in the future
	std::vector<int> lanes = {0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14};
	std::vector<int> veh_id = {14, 13, 12, 11, 10, 9, 8, 6, 5, 4, 3, 2, 1, 0};

	for (int i = 0; i < lanes.size(); i++)
	{
		const char *road_lane = SE_GetOSIRoadLane(&road_lane_size, veh_id[i]);
		osi_lane.ParseFromArray(road_lane, road_lane_size);

		EXPECT_EQ(osi_lane.classification().is_host_vehicle_lane(), false);
	}

	SE_Close();
}

TEST(GetOSIRoadLaneTest, lane_classification)
{

	std::string scenario_file = "../../../resources/xosc/full_e6mini.xosc";
	const char *Scenario_file = scenario_file.c_str();

	SE_Init(Scenario_file, 0, 0, 0, 0);
	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();
	int road_lane_size;
	osi3::Lane osi_lane;

	// explicitly writing lanes ID so that it will be easy to adapt the test for more complex roads in the future
	std::vector<int> lanes = {0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14};
	std::vector<int> veh_id = {14, 13, 12, 11, 10, 9, 8, 6, 5, 4, 3, 2, 1, 0};

	int obj_id = 0;

	for (int i = 0; i < lanes.size(); i++)
	{
		int lane_id = lanes[i];

		const char *road_lane = SE_GetOSIRoadLane(&road_lane_size, veh_id[i]);
		osi_lane.ParseFromArray(road_lane, road_lane_size);

		osi3::Lane_Classification_Type lane_type = osi_lane.classification().type();

		if (lane_id == 3 || lane_id == 4 || lane_id == 5 || lane_id == 9 || lane_id == 10 || lane_id == 11)
		{
			EXPECT_EQ(lane_type, osi3::Lane_Classification_Type::Lane_Classification_Type_TYPE_DRIVING);
		}
		else
		{
			EXPECT_EQ(lane_type, osi3::Lane_Classification_Type::Lane_Classification_Type_TYPE_NONDRIVING);
		}

		obj_id++;
	}

	SE_Close();
}

TEST(GetOSILaneBoundaryTests, lane_boundary_id_existing)
{

	std::string scenario_file = "../../../resources/xosc/full_e6mini.xosc";
	const char *Scenario_file = scenario_file.c_str();

	SE_Init(Scenario_file, 0, 0, 0, 0);
	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();
	int lb_size;
	osi3::LaneBoundary osi_lb;

	// explicitly writing lanes ID so that it will be easy to adapt the test for more complex roads in the future
	std::vector<int> lanes = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
	std::vector<int> lane_bound = {-1, 8, 9, 10, 0, 1, 2, 3, 11, 4, 5, 6, 7, 12, 13, 14, -1};

	for (int i = 0; i < lane_bound.size(); i++)
	{
		int lb_global_id = lane_bound[i];
		if (lb_global_id == -1)
		{
			continue;
		}
		const char *lb = SE_GetOSILaneBoundary(&lb_size, lb_global_id);
		osi_lb.ParseFromArray(lb, lb_size);

		EXPECT_EQ(osi_lb.id().value(), lb_global_id);
	}

	SE_Close();
}

class GetOSILaneBoundaryTests : public ::testing::TestWithParam<std::tuple<int, int>>
{
};
// inp:  excisting lane boundary global id
// expected: size of osi lane boundary message = 0

TEST_P(GetOSILaneBoundaryTests, lane_boundary_id_not_existing)
{

	std::string scenario_file = "../../../resources/xosc/full_e6mini.xosc";
	const char *Scenario_file = scenario_file.c_str();

	SE_Init(Scenario_file, 0, 0, 0, 0);
	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();
	int lb_size = 0;

	const char *lb = SE_GetOSILaneBoundary(&lb_size, std::get<0>(GetParam()));

	EXPECT_EQ(lb_size, std::get<1>(GetParam()));
	EXPECT_EQ(lb, nullptr);

	SE_Close();
}

INSTANTIATE_TEST_CASE_P(EsminiAPITests, GetOSILaneBoundaryTests, ::testing::Values(std::make_tuple(15, 0), std::make_tuple(-15, 0)));

TEST(OSIFile, writeosifile_two_step)
{

	std::string scenario_file = "../../../resources/xosc/cut-in.xosc";
	const char *Scenario_file = scenario_file.c_str();
	std::streamoff file_size1, file_size2, file_sizeend;

	SE_Init(Scenario_file, 0, 0, 0, 0);

	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();
	SE_OSIFileOpen(0);
	SE_OSIFileWrite(true);

	std::ifstream in_file("ground_truth.osi", std::ios::binary);
	in_file.seekg(0, std::ios::end);
	file_size1 = in_file.tellg();
	//std::cout <<"Size of the file at first step "<< file_size1 << " bytes" << std::endl;

	SE_StepDT(0.001f);
	SE_UpdateOSIGroundTruth();
	SE_OSIFileWrite(true);

	in_file.seekg(0, std::ios::end);
	file_size2 = in_file.tellg();
	//std::cout <<"Size of the file at second step "<< file_size2 << " bytes" << std::endl;

	SE_Close();

	in_file.seekg(0, std::ios::end);
	file_sizeend = in_file.tellg();
	//std::cout <<"Size of closing step "<< file_sizeend << " bytes" << std::endl;

	//	EXPECT_EQ(file_size2, file_sizeend);  // File might not be flushed until it's closed, unless it is done explicitly
	EXPECT_LT(file_size1, file_size2);
}

TEST(OSIFile, writeosifile_no_init)
{

	bool open = SE_OSIFileOpen(0);
	bool write = SE_OSIFileWrite();

	EXPECT_EQ(open, false);
	EXPECT_EQ(write, false);
}

typedef struct
{
	float length;
	float width;
	float height;
	float centerOffsetX;
	float centerOffsetY;
	float centerOffsetZ;
} bounding_box;

class GetGroundTruthTests : public ::testing::TestWithParam<std::tuple<std::string, int, int, bounding_box>>
{
};
// inp: nto excisting lane boundary global id
// expected: size of osi lane boundary message = 0

TEST_P(GetGroundTruthTests, receive_GroundTruth)
{

	std::string scenario_file = std::get<0>(GetParam());
	const char *Scenario_file = scenario_file.c_str();
	int sv_size = 0;
	osi3::GroundTruth osi_gt;

	SE_Init(Scenario_file, 0, 0, 0, 0);

	//SE_OSIFileOpen(0);

	SE_StepDT(0.001f);

	SE_UpdateOSIGroundTruth();

	const char *sv = SE_GetOSIGroundTruth(&sv_size);
	osi_gt.ParseFromArray(sv, sv_size);

	int n_lanes = osi_gt.lane_size();

	int n_objects = osi_gt.mutable_moving_object()->size();

	int ego_index = 0; // ego vehicle are always first in tested scenarios

	float ego_length = (float)osi_gt.mutable_moving_object(ego_index)->mutable_base()->mutable_dimension()->length();
	float ego_width = (float)osi_gt.mutable_moving_object(ego_index)->mutable_base()->mutable_dimension()->width();
	float ego_height = (float)osi_gt.mutable_moving_object(ego_index)->mutable_base()->mutable_dimension()->height();
	float ego_xoffset = (float)osi_gt.mutable_moving_object(ego_index)->mutable_vehicle_attributes()->mutable_bbcenter_to_rear()->x();
	float ego_yoffset = (float)osi_gt.mutable_moving_object(ego_index)->mutable_vehicle_attributes()->mutable_bbcenter_to_rear()->y();
	float ego_zoffset = (float)osi_gt.mutable_moving_object(ego_index)->mutable_vehicle_attributes()->mutable_bbcenter_to_rear()->z();

	EXPECT_EQ(n_lanes, std::get<1>(GetParam()));
	EXPECT_EQ(n_objects, std::get<2>(GetParam()));
	EXPECT_EQ(ego_length, std::get<3>(GetParam()).length);
	EXPECT_EQ(ego_width, std::get<3>(GetParam()).width);
	EXPECT_EQ(ego_height, std::get<3>(GetParam()).height);
	EXPECT_EQ(ego_xoffset, std::get<3>(GetParam()).centerOffsetX);
	EXPECT_EQ(ego_yoffset, std::get<3>(GetParam()).centerOffsetY);
	EXPECT_EQ(ego_zoffset, std::get<3>(GetParam()).centerOffsetZ);
}

INSTANTIATE_TEST_CASE_P(EsminiAPITests, GetGroundTruthTests, ::testing::Values(std::make_tuple("../../../resources/xosc/cut-in.xosc", 14, 2, bounding_box{5.0f, 2.0f, 1.8f, 1.4f, 0.0f, 0.9f}), std::make_tuple("../../../resources/xosc/straight_500m.xosc", 6, 2, bounding_box{5.0f, 2.0f, 1.8f, 1.4f, 0.0f, 0.9f}), std::make_tuple("../../../resources/xosc/highway_merge.xosc", 33, 6, bounding_box{5.0f, 2.0f, 1.8f, 1.4f, 0.0f, 0.9f})));
// scenario_file_name, number_of_lanes, number_of_objects, ego_bounding_box

TEST(GetGroundTruthTests, receive_GroundTruth_no_init)
{

	int sv_size = 0;
	osi3::GroundTruth osi_gt;

	const char *sv = SE_GetOSIGroundTruth(&sv_size);

	EXPECT_EQ(sv_size, 0);
	EXPECT_EQ(sv, nullptr);
}

TEST(GetMiscObjFromGroundTruth, receive_miscobj)
{

	int sv_size = 0;
	osi3::GroundTruth osi_gt;

	SE_Init("../../../EnvironmentSimulator/Unittest/scenarios/miscobj_basic.xosc", 0, 0, 0, 0);

	SE_StepDT(0.001f);

	SE_UpdateOSIGroundTruth();

	const char *gt = SE_GetOSIGroundTruth(&sv_size);
	osi_gt.ParseFromArray(gt, sv_size);

	int n_miscobjects = osi_gt.mutable_stationary_object()->size();

	uint64_t miscobj_id = osi_gt.mutable_stationary_object(0)->mutable_id()->value();
	osi3::StationaryObject_Classification_Type miscobj_type = osi_gt.mutable_stationary_object(0)->mutable_classification()->type();

	double miscobj_length = osi_gt.mutable_stationary_object(0)->mutable_base()->mutable_dimension()->length();
	double miscobj_width = osi_gt.mutable_stationary_object(0)->mutable_base()->mutable_dimension()->width();
	double miscobj_height = osi_gt.mutable_stationary_object(0)->mutable_base()->mutable_dimension()->height();

	double miscobj_x = osi_gt.mutable_stationary_object(0)->mutable_base()->mutable_position()->x();
	double miscobj_y = osi_gt.mutable_stationary_object(0)->mutable_base()->mutable_position()->y();
	double miscobj_z = osi_gt.mutable_stationary_object(0)->mutable_base()->mutable_position()->z();

	double miscobj_roll = osi_gt.mutable_stationary_object(0)->mutable_base()->mutable_orientation()->roll();
	double miscobj_pitch = osi_gt.mutable_stationary_object(0)->mutable_base()->mutable_orientation()->pitch();
	double miscobj_yaw = osi_gt.mutable_stationary_object(0)->mutable_base()->mutable_orientation()->yaw();

	EXPECT_EQ(n_miscobjects, 1);
	EXPECT_EQ(miscobj_id, 0);

	EXPECT_EQ(miscobj_type, osi3::StationaryObject_Classification_Type::StationaryObject_Classification_Type_TYPE_BARRIER);

	EXPECT_EQ(miscobj_length, 200);
	EXPECT_EQ(miscobj_width, 100);
	EXPECT_EQ(miscobj_height, 5);

	EXPECT_EQ(miscobj_x, 10);
	EXPECT_EQ(miscobj_y, 10);
	EXPECT_EQ(miscobj_z, 0.0); // adjusted to the road z

	EXPECT_EQ(miscobj_roll, 5.0);  // Aligned to the road (so if road roll is 1.0 total roll will be 6.0)
	EXPECT_EQ(miscobj_pitch, 5.0); // Aligned to the road (so if road pitch is 1.0 total pitch will be 6.0)
	EXPECT_EQ(miscobj_yaw, 5.0);
}

TEST(ParameterTest, GetTypedParameterValues)
{
	std::string scenario_file = "../../../resources/xosc/lane_change.xosc";
	const char *Scenario_file = scenario_file.c_str();
	SE_Init(Scenario_file, 0, 0, 0, 0);

	bool boolVar;
	int retVal;
	retVal = SE_GetParameterBool("DummyParameter2", &boolVar);
	EXPECT_EQ(retVal, 0);
	EXPECT_EQ(boolVar, true);

	// Unavailable
	retVal = SE_GetParameterBool("DoesNotExist", &boolVar);
	EXPECT_EQ(retVal, -1);

	// Set value
	SE_SetParameterBool("DummyParameter2", false);
	retVal = SE_GetParameterBool("DummyParameter2", &boolVar);
	EXPECT_EQ(retVal, 0);
	EXPECT_EQ(boolVar, false);

	// Wrong name
	retVal = SE_SetParameterBool("DummyParameter3", false);
	EXPECT_EQ(retVal, -1);

	// Wrong type
	retVal = SE_SetParameterInt("DummyParameter2", false);
	EXPECT_EQ(retVal, -1);

	// String
	const char *strVar;
	retVal = SE_GetParameterString("DummyParameter3", &strVar);
	EXPECT_EQ(retVal, 0);
	EXPECT_STREQ(strVar, "lane_change_param");

	retVal = SE_SetParameterString("DummyParameter3", "Kalle");
	EXPECT_EQ(retVal, 0);
	retVal = SE_GetParameterString("DummyParameter3", &strVar);
	EXPECT_EQ(retVal, 0);
	EXPECT_STREQ(strVar, "Kalle");

	retVal = SE_GetParameterString("DoesNotExist", &strVar);
	EXPECT_EQ(retVal, -1);

	SE_Close();
}

TEST(OverrideActionTest, TestGetAndSet)
{
	std::string scenario_file = "../../../EnvironmentSimulator/Unittest/scenarios/override_action.xosc";
	const char *Scenario_file = scenario_file.c_str();
	float dt = 0.1f;
	float t = 0.0f;

	SE_OverrideActionList list;

	ASSERT_EQ(SE_Init(Scenario_file, 0, 0, 0, 0), 0);

	EXPECT_EQ(SE_GetOverrideActionStatus(0, &list), 0);
	EXPECT_EQ(list.throttle.active, false);
	EXPECT_DOUBLE_EQ(list.throttle.value, 0.0);

	for (; t < 2.5; t += dt)
	{
		SE_StepDT(dt);
	}
	// Should still be no changes
	EXPECT_EQ(SE_GetOverrideActionStatus(0, &list), 0);
	EXPECT_EQ(list.throttle.active, false);
	EXPECT_DOUBLE_EQ(list.throttle.value, 0.0);

	for (; t < 3.1; t += dt)
	{
		SE_StepDT(dt);
	}
	// Now there should be some settings done
	EXPECT_EQ(SE_GetOverrideActionStatus(0, &list), 0);
	EXPECT_EQ(list.throttle.active, true);
	EXPECT_DOUBLE_EQ(list.throttle.value, 0.5);
	EXPECT_EQ(list.clutch.active, false);
	EXPECT_DOUBLE_EQ(list.clutch.value, 1.0);
	EXPECT_EQ(list.steeringWheel.active, false);
	EXPECT_DOUBLE_EQ(list.steeringWheel.value, 0.0);

	for (; t < 5.1; t += dt)
	{
		SE_StepDT(dt);
	}
	EXPECT_EQ(SE_GetOverrideActionStatus(0, &list), 0);
	EXPECT_EQ(list.throttle.active, true);
	EXPECT_DOUBLE_EQ(list.throttle.value, 0.5);
	EXPECT_EQ(list.clutch.active, true);
	EXPECT_DOUBLE_EQ(list.clutch.value, 0.7);
	EXPECT_EQ(list.steeringWheel.active, false);
	EXPECT_NEAR(list.steeringWheel.value, 2 * M_PI, 0.01);
}

TEST(PropertyTest, TestGetAndSet)
{
	std::string scenario_file = "../../../EnvironmentSimulator/Unittest/scenarios/VehiclePropertyTest.xosc";
	const char *Scenario_file = scenario_file.c_str();
	SE_Init(Scenario_file, 0, 0, 0, 0);

	//object 0, 3 properties
	EXPECT_EQ(SE_GetNumberOfProperties(0), 3);
	//    <Property name="model_id" value="1"/>
	//    <Property name="dummy0_1" value="2"/>
	//    <Property name="dummy0_2" value="3"/>
	EXPECT_STREQ(SE_GetObjectPropertyName(0, 0), "model_id");
	EXPECT_STREQ(SE_GetObjectPropertyName(0, 1), "dummy0_1");
	EXPECT_STREQ(SE_GetObjectPropertyName(0, 2), "dummy0_2");
	EXPECT_STREQ(SE_GetObjectPropertyValue(0, "model_id"), "1");
	EXPECT_STREQ(SE_GetObjectPropertyValue(0, "dummy0_1"), "2");
	EXPECT_STREQ(SE_GetObjectPropertyValue(0, "dummy0_2"), "3");
	//object 1, 4 properties
	//    <Property name="model_id" value="1"/>
	//    <Property name="dummy1_1" value="2"/>
	//    <Property name="dummy1_2" value="3"/>
	//    <Property name="dummy1_3" value="4"/>
	EXPECT_EQ(SE_GetNumberOfProperties(1), 4);
	EXPECT_STREQ(SE_GetObjectPropertyName(1, 0), "model_id");
	EXPECT_STREQ(SE_GetObjectPropertyName(1, 1), "dummy1_1");
	EXPECT_STREQ(SE_GetObjectPropertyName(1, 2), "dummy1_2");
	EXPECT_STREQ(SE_GetObjectPropertyName(1, 3), "dummy1_3");
	// propertyIndex exceeds both limits
	EXPECT_STREQ(SE_GetObjectPropertyName(1, -1), "");
	EXPECT_STREQ(SE_GetObjectPropertyName(1, 4), "");
	// propertyName does not exist
	EXPECT_STREQ(SE_GetObjectPropertyValue(1, "superdummy"), "");


	//object 2, 0 properties
	EXPECT_EQ(SE_GetNumberOfProperties(2), 0);
	EXPECT_STREQ(SE_GetObjectPropertyName(2, 0), "");
	EXPECT_STREQ(SE_GetObjectPropertyValue(2,"model_id"), "");

	//object -1 and 3, does not exist
	EXPECT_EQ(SE_GetNumberOfProperties(3), -1);
	EXPECT_EQ(SE_GetNumberOfProperties(-1), -1);
}

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);

	SE_LogToConsole(false);

	return RUN_ALL_TESTS();
}