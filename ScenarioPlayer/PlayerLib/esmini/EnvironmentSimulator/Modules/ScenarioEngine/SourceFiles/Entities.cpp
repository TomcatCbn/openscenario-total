/* 
 * esmini - Environment Simulator Minimalistic 
 * https://github.com/esmini/esmini
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 * 
 * Copyright (c) partners of Simulation Scenarios
 * https://sites.google.com/view/simulationscenarios
 */

#include "Entities.hpp"
#include "Controller.hpp"

using namespace scenarioengine;
using namespace roadmanager;

#define ELEVATION_DIFF_THRESHOLD 2.5

void Object::SetEndOfRoad(bool state, double time)
{
	if (state == true)
	{
		end_of_road_timestamp_ = time;
	}
	else
	{
		end_of_road_timestamp_ = 0.0;
	}
}

void Object::SetOffRoad(bool state, double time)
{
	if (state == true)
	{
		off_road_timestamp_ = time;
	}
	else
	{
		off_road_timestamp_ = 0.0;
	}
}

void  Object::SetStandStill(bool state, double time)
{
	if (state == true)
	{
		stand_still_timestamp_ = time;
	}
	else
	{
		stand_still_timestamp_ = 0.0;
	}
}

int Object::GetAssignedControllerType()
{
	if (controller_)
	{
		return controller_->GetType();
	}
	else
	{
		// Report 0 if not assigned or not activated on any domain
		return 0;
	}
}

int Object::GetActivatedControllerType()
{
	if (controller_ && controller_->GetDomain())
	{
		return controller_->GetType();
	}
	else
	{
		// Report 0 if not assigned or not activated on any domain
		if (IsGhost())
		{
			return Controller::Type::GHOST_RESERVED_TYPE;
		}
		else
		{ 
			return 0;
		}
	}
}

bool Object::IsControllerActiveOnDomains(int domainMask)
{
	if (controller_)
	{
		return (controller_->GetDomain() & domainMask) == domainMask;
	}
	else
	{
		return false;
	}
}

bool Object::IsControllerActiveOnAnyOfDomains(int domainMask)
{
	if (controller_)
	{
		return controller_->GetDomain() & domainMask;
	}
	else
	{
		return false;
	}
}

int Object::GetControllerMode()
{
	if (controller_)
	{
		return controller_->GetMode();
	}
	else
	{
		return 0;  // default
	}
}

void Object::SetVisibilityMask(int mask)
{
	visibilityMask_ = mask;
	SetDirtyBits(dirty_ | DirtyBit::VISIBILITY);
}

void Object::SetVel(double x_vel, double y_vel, double z_vel)
{
	pos_.SetVel(x_vel, y_vel, z_vel);
	SetDirtyBits(dirty_ | DirtyBit::VELOCITY);
}

void Object::SetAcc(double x_acc, double y_acc, double z_acc)
{
	pos_.SetAcc(x_acc, y_acc, z_acc);
	SetDirtyBits(dirty_ | DirtyBit::ACCELERATION);
}

void Object::SetAngularVel(double h_vel, double p_vel, double r_vel)
{
	pos_.SetAngularVel(h_vel, p_vel, r_vel);
	SetDirtyBits(dirty_ | DirtyBit::ANGULAR_RATE);
}

void Object::SetAngularAcc(double h_acc, double p_acc, double r_acc)
{
	pos_.SetAngularAcc(h_acc, p_acc, r_acc);
	SetDirtyBits(dirty_ | DirtyBit::ANGULAR_ACC);
}

bool Object::Collision(Object* target)
{
	// Apply method Separating Axis Theorem (SAT)
	// http://www.euclideanspace.com/threed/games/examples/cars/collisions/
	// https://www.sevenson.com.au/actionscript/sat/

	// Idea:
	// For each side of the bounding boxes:
	//   The normal of that edge will be the projection axis
	//   Project all points of the two bounding boxes onto that axis
	//   If we find ONE side with a gap between point clusters from BB1 and BB2,
	//   it's enough to conclude they are not overlapping/colliding
	// 
	// Optimization: Since the bounding boxes are boxes with parallel
	// sides, we only need to check half of the sides

	if (target == 0)
	{
		return false;
	}

	Object* obj0 = this;
	Object* obj1 = target;

	// First do a rough check to rule out potential overlap/collision
	// Compare radial/euclidean distance with sum of the diagonal dimension of the bounding boxes
	double x = 0, y = 0;
	double dist = fabs(this->pos_.getRelativeDistance(target->pos_.GetX(), target->pos_.GetY(), x, y));
	double max_length = this->boundingbox_.dimensions_.length_ + target->boundingbox_.dimensions_.length_;
	double max_width = this->boundingbox_.dimensions_.width_ + target->boundingbox_.dimensions_.width_;
	double dist_threshold = sqrt(max_length * max_length + max_width * max_width);

	if (dist > dist_threshold)
	{
		return false;
	}

	// Also do a Z sanity check, to rule out on different road elevations
	if (fabs(obj0->pos_.GetZ() - obj1->pos_.GetZ()) > ELEVATION_DIFF_THRESHOLD)
	{
		return false;
	}

	for (int i = 0; i < 2; i++)  // for each of the two BBs
	{
		if (i == 1)
		{
			// swap order, now check all edges of target object
			obj0 = target;
			obj1 = this;
		}

		double n0[2] = { 0.0, 0.0 };
		for (int j = 0; j < 2; j++)  // for longitudinal and lateral sides
		{
			if (j == 0)
			{
				// Normal for longitudinal side (sides) points along lateral direction
				n0[0] = 0.0;
				n0[1] = 1.0;
			}
			else
			{
				// Normal for lateral side (front/rear) points along lateral direction
				n0[0] = 1.0;
				n0[1] = 0.0;
			}

			// Rotate the normal/projection axis to align with bounding box/vehicle
			double n1[2] = { 0.0, 0.0 };
			RotateVec2D(n0[0], n0[1], obj0->pos_.GetH(), n1[0], n1[1]);

			// Now, project each point of each BB onto the rotated normal
			// And register min and max for point cluster of each BB
			double min[2] = { 0.0, 0.0 }, max[2] = { 0.0, 0.0 };
			for (int k = 0; k < 2; k++)
			{
				Object* obj = (k == 0 ? obj0 : obj1);

				// Specify bounding box corner vertices, starting at first quadrant
				double vertices[4][2] =
				{
					{ obj->boundingbox_.center_.x_ + obj->boundingbox_.dimensions_.length_ / 2.0, obj->boundingbox_.center_.y_ + obj->boundingbox_.dimensions_.width_ / 2.0 },
					{ obj->boundingbox_.center_.x_ - obj->boundingbox_.dimensions_.length_ / 2.0, obj->boundingbox_.center_.y_ + obj->boundingbox_.dimensions_.width_ / 2.0 },
					{ obj->boundingbox_.center_.x_ - obj->boundingbox_.dimensions_.length_ / 2.0, obj->boundingbox_.center_.y_ - obj->boundingbox_.dimensions_.width_ / 2.0 },
					{ obj->boundingbox_.center_.x_ + obj->boundingbox_.dimensions_.length_ / 2.0, obj->boundingbox_.center_.y_ - obj->boundingbox_.dimensions_.width_ / 2.0 }
				};

				for (int l = 0; l < 4; l++)
				{
					double point_to_project[2];

					// Align projection points to object heading
					RotateVec2D(vertices[l][0], vertices[l][1], obj->pos_.GetH(), point_to_project[0], point_to_project[1]);

					double dot_p = GetDotProduct2D(
						obj->pos_.GetX() + point_to_project[0],
						obj->pos_.GetY() + point_to_project[1],
						n1[0], n1[1]);

					if (l == 0)
					{
						min[k] = max[k] = dot_p;
					}
					else
					{
						min[k] = MIN(dot_p, min[k]);
						max[k] = MAX(dot_p, max[k]);
					}
				}
			}

			if (min[0] < min[1] - SMALL_NUMBER && max[0] < min[1] - SMALL_NUMBER ||
				max[0] > max[1] + SMALL_NUMBER && min[0] > max[1] + SMALL_NUMBER)
			{
				// gap found
				return false;
			}
		}
	}
	
	return true;
}

double Object::PointCollision(double x, double y)
{
	// Apply method Separating Axis Theorem (SAT)
	// http://www.euclideanspace.com/threed/games/examples/cars/collisions/
	// https://www.sevenson.com.au/actionscript/sat/

	// Idea:
	// For each side of the bounding box:
	//   The normal of that edge will be the projection axis
	//   Project the point and all axis of the bounding boxe onto that axis
	//   If we find ONE side with a gap between point clusters from BB1 and BB2,
	//   it's enough to conclude they are not overlapping/colliding
	// 
	// Optimization: Since the bounding boxes are boxes with parallel
	// sides, we only need to check half of the sides

	Object* obj0 = this;

	double n0[2] = { 0.0, 0.0 };
	for (int j = 0; j < 2; j++)  // for longitudinal and lateral sides
	{
		if (j == 0)
		{
			// Normal for longitudinal side (sides) points along lateral side
			n0[0] = 0.0;
			n0[1] = 1.0;
		}
		else
		{
			// Normal for lateral side (front/rear) points along lateral side
			n0[0] = 1.0;
			n0[1] = 0.0;
		}

		// Rotate the normal/projection axis to align with bounding box/vehicle
		double n1[2] = { 0.0, 0.0 };
		RotateVec2D(n0[0], n0[1], obj0->pos_.GetH(), n1[0], n1[1]);

		// Now, project each point of each BB onto the rotated normal
		// And register min and max for point cluster of each BB
		double min[2] = { 0.0, 0.0 }, max[2] = { 0.0, 0.0 };

		// Specify bounding box corner vertices, starting at first quadrant
		double vertices[4][2] =
		{
			{ obj0->boundingbox_.center_.x_ + obj0->boundingbox_.dimensions_.length_ / 2.0, obj0->boundingbox_.center_.y_ + obj0->boundingbox_.dimensions_.width_ / 2.0 },
			{ obj0->boundingbox_.center_.x_ - obj0->boundingbox_.dimensions_.length_ / 2.0, obj0->boundingbox_.center_.y_ + obj0->boundingbox_.dimensions_.width_ / 2.0 },
			{ obj0->boundingbox_.center_.x_ - obj0->boundingbox_.dimensions_.length_ / 2.0, obj0->boundingbox_.center_.y_ - obj0->boundingbox_.dimensions_.width_ / 2.0 },
			{ obj0->boundingbox_.center_.x_ + obj0->boundingbox_.dimensions_.length_ / 2.0, obj0->boundingbox_.center_.y_ - obj0->boundingbox_.dimensions_.width_ / 2.0 }
		};

		for (int l = 0; l < 4; l++)
		{
			double point_to_project[2];

			// Align projection points to object heading
			RotateVec2D(vertices[l][0], vertices[l][1], obj0->pos_.GetH(), point_to_project[0], point_to_project[1]);

			double dot_p = GetDotProduct2D(
				obj0->pos_.GetX() + point_to_project[0],
				obj0->pos_.GetY() + point_to_project[1],
				n1[0], n1[1]);

			if (l == 0)
			{
				min[0] = max[0] = dot_p;
			}
			else
			{
				min[0] = MIN(dot_p, min[0]);
				max[0] = MAX(dot_p, max[0]);
			}
		}

		double dot_p = GetDotProduct2D(x, y, n1[0], n1[1]);

		if (min[0] < dot_p - SMALL_NUMBER && max[0] < dot_p - SMALL_NUMBER ||
			max[0] > dot_p + SMALL_NUMBER && min[0] > dot_p + SMALL_NUMBER)
		{
			// gap found - no collision
			return false;
		}
	}

	return true;
}

double Object::FreeSpaceDistance(Object* target, double* latDist, double* longDist)
{
	double minDist = LARGE_NUMBER;

	if (target == 0)
	{
		return minDist;
	}

	if (Collision(target))
	{
		return 0.0;
	}

	// OK, they are not overlapping. Now find the distance.
	// Strategy: Brute force check all vertices of one bounding box 
	// against all sides of the other bounding box - then switch to 
	// check vertices of the other bounding box against the sides 
	// of the first bounding box.

	double vertices[2][4][2];


	for (int i = 0; i < 2; i++)  // for each of the two BBs
	{
		Object* obj = (i == 0 ? this : target);

		// Specify bounding box corner vertices, starting at first quadrant
		double vtmp[4][2] =
		{
			{ obj->boundingbox_.center_.x_ + obj->boundingbox_.dimensions_.length_ / 2.0, obj->boundingbox_.center_.y_ + obj->boundingbox_.dimensions_.width_ / 2.0 },
			{ obj->boundingbox_.center_.x_ - obj->boundingbox_.dimensions_.length_ / 2.0, obj->boundingbox_.center_.y_ + obj->boundingbox_.dimensions_.width_ / 2.0 },
			{ obj->boundingbox_.center_.x_ - obj->boundingbox_.dimensions_.length_ / 2.0, obj->boundingbox_.center_.y_ - obj->boundingbox_.dimensions_.width_ / 2.0 },
			{ obj->boundingbox_.center_.x_ + obj->boundingbox_.dimensions_.length_ / 2.0, obj->boundingbox_.center_.y_ - obj->boundingbox_.dimensions_.width_ / 2.0 }
		};

		for (int j = 0; j < 4; j++)  // for all vertices
		{
			// Align points to object heading and position
			RotateVec2D(vtmp[j][0], vtmp[j][1], obj->pos_.GetH(), vertices[i][j][0], vertices[i][j][1]);
			vertices[i][j][0] += obj->pos_.GetX();
			vertices[i][j][1] += obj->pos_.GetY();
		}
	}

	for (int i = 0; i < 2; i++)  // for each of the two BBs
	{
		int vindex = (i == 0 ? 0 : 1);

		for (int j = 0; j < 4; j++)  // for all vertices
		{
			double point[2] = { vertices[vindex][j][0], vertices[vindex][j][1] };

			for (int k = 0; k < 4; k++)  // for all sides/edges in the other bounding box
			{
				double edge[2][2];
				edge[0][0] = vertices[(vindex + 1) % 2][k][0];
				edge[0][1] = vertices[(vindex + 1) % 2][k][1];
				edge[1][0] = vertices[(vindex + 1) % 2][(k + 1) % 4][0];
				edge[1][1] = vertices[(vindex + 1) % 2][(k + 1) % 4][1];

				double xComp = 0;
				double yComp = 0;
				double tmpDist = DistanceFromPointToEdge2D(point[0], point[1], edge[0][0], edge[0][1], edge[1][0], edge[1][1], &xComp, &yComp);

				if (tmpDist < minDist)
				{
					minDist = tmpDist;
					
					// Calculate x, y components of the distance in vehicle reference system
					double pv[2];
					if (i == 0)
					{
						// From this object to the target
						pv[0] = xComp - point[0];
						pv[1] = yComp - point[1];
					}
					else
					{
						// From the target to this object 
						pv[0] = point[0] - xComp;
						pv[1] = point[1] - yComp;
					}
					
					double pvr[2];
					RotateVec2D(pv[0], pv[1], -this->pos_.GetH(), pvr[0], pvr[1]);
					if (latDist) *latDist = pvr[1]; // y points left in vehicle ref system
					if (longDist) *longDist = pvr[0]; // x points forward in vehicle ref system
				}
			}
		}
	}

	return minDist;
}

double Object::FreeSpaceDistancePoint(double x, double y, double* latDist, double* longDist)
{
	double minDist = LARGE_NUMBER;

	if (PointCollision(x, y))
	{
		return 0.0;
	}

	// OK, they are not overlapping. Now find the distance.
	// Strategy: Brute force check point against all sides 
	// of the bounding box 

	Object* obj = this;

	double vertices[4][2];

	// Specify bounding box corner vertices, starting at first quadrant
	double vtmp[4][2] =
	{
		{ obj->boundingbox_.center_.x_ + obj->boundingbox_.dimensions_.length_ / 2.0, obj->boundingbox_.center_.y_ + obj->boundingbox_.dimensions_.width_ / 2.0 },
		{ obj->boundingbox_.center_.x_ - obj->boundingbox_.dimensions_.length_ / 2.0, obj->boundingbox_.center_.y_ + obj->boundingbox_.dimensions_.width_ / 2.0 },
		{ obj->boundingbox_.center_.x_ - obj->boundingbox_.dimensions_.length_ / 2.0, obj->boundingbox_.center_.y_ - obj->boundingbox_.dimensions_.width_ / 2.0 },
		{ obj->boundingbox_.center_.x_ + obj->boundingbox_.dimensions_.length_ / 2.0, obj->boundingbox_.center_.y_ - obj->boundingbox_.dimensions_.width_ / 2.0 }
	};

	for (int j = 0; j < 4; j++)  // for all vertices
	{
		// Align points to object heading and position
		RotateVec2D(vtmp[j][0], vtmp[j][1], obj->pos_.GetH(), vertices[j][0], vertices[j][1]);
		vertices[j][0] += obj->pos_.GetX();
		vertices[j][1] += obj->pos_.GetY();
	}


	double point[2] = { x, y };

	for (int k = 0; k < 4; k++)  // for all sides/edges of the bounding box
	{
		double edge[2][2];
		edge[0][0] = vertices[k][0];
		edge[0][1] = vertices[k][1];
		edge[1][0] = vertices[(k + 1) % 4][0];
		edge[1][1] = vertices[(k + 1) % 4][1];

		double xComp = 0;
		double yComp = 0;
		double tmpDist = DistanceFromPointToEdge2D(point[0], point[1], edge[0][0], edge[0][1], edge[1][0], edge[1][1], 
			&xComp, &yComp);

		if (tmpDist < minDist)
		{
			minDist = tmpDist;

			// Calculate x, y components of the distance in vehicle reference system

			double pv[2];
			// From this object to the target point
			pv[0] = point[0] - xComp;
			pv[1] = point[1] - yComp;

			double pvr[2];
			RotateVec2D(pv[0], pv[1], -this->pos_.GetH(), pvr[0], pvr[1]);
			if (latDist) *latDist = pvr[1]; // y points left in vehicle ref system
			if (longDist) *longDist = pvr[0]; // x points forward in vehicle ref system
		}
	}

	return minDist;
}

int Object::FreeSpaceDistancePointRoadLane(double x, double y, double* latDist, double* longDist, CoordinateSystem cs)
{
	if (cs != CoordinateSystem::CS_LANE && cs != CoordinateSystem::CS_ROAD)
	{
		LOG("Unexpected coordinateSystem (%d). %d or %d expected.", CoordinateSystem::CS_LANE, CoordinateSystem::CS_ROAD);
		return -1;
	}

	if (cs == CoordinateSystem::CS_LANE)
	{
		LOG("freespace LANE coordinateSystem not supported yet, falling back to freespace ROAD");
		cs = CoordinateSystem::CS_ROAD;
	}

	// Specify bounding box corner vertices, starting at first quadrant
	double vtmp[4][2] =
	{
		{ boundingbox_.center_.x_ + boundingbox_.dimensions_.length_ / 2.0, boundingbox_.center_.y_ + boundingbox_.dimensions_.width_ / 2.0 },
		{ boundingbox_.center_.x_ - boundingbox_.dimensions_.length_ / 2.0, boundingbox_.center_.y_ + boundingbox_.dimensions_.width_ / 2.0 },
		{ boundingbox_.center_.x_ - boundingbox_.dimensions_.length_ / 2.0, boundingbox_.center_.y_ - boundingbox_.dimensions_.width_ / 2.0 },
		{ boundingbox_.center_.x_ + boundingbox_.dimensions_.length_ / 2.0, boundingbox_.center_.y_ - boundingbox_.dimensions_.width_ / 2.0 }
	};
	
	// Align points to object heading and position
	double vertices[4][2];
	for (int i = 0; i < 4; i++) 
	{
		RotateVec2D(vtmp[i][0], vtmp[i][1], pos_.GetH(), vertices[i][0], vertices[i][1]);
		vertices[i][0] += pos_.GetX();
		vertices[i][1] += pos_.GetY();
	}

	// Evaluate road and lane coordinates of each point
	Position pos[4];
	double maxS = 0.0;
	double minS = LARGE_NUMBER;
	double maxT = 0.0;
	double minT = LARGE_NUMBER;
	for (int j = 0; j < 4; j++)
	{
		pos[j].SetInertiaPos(vertices[j][0], vertices[j][1], 0.0);

		if (j == 0 || pos[j].GetS() < minS)
		{
			minS = pos[j].GetS();
		}

		if (j == 0 || pos[j].GetT() < minT)
		{
			minT = pos[j].GetT();
		}

		if (j == 0 || pos[j].GetS() > maxS)
		{
			maxS = pos[j].GetS();
		}

		if (j == 0 || pos[j].GetT() > maxT)
		{
			maxT = pos[j].GetT();
		}
	}

	// Find min distance for each dimension
	double minLatDist = LARGE_NUMBER;
	double minLongDist = LARGE_NUMBER;

	for (int k = 0; k < 4; k++)
	{
		if (k == 0 || fabs(pos[k].GetS() - pos_.GetS()) < fabs(minLongDist))
		{
			minLongDist = pos[k].GetS() - pos_.GetS();
		}
		
		if (k == 0 || fabs(pos[k].GetT() - pos_.GetT()) < fabs(minLatDist))
		{
			minLatDist = pos[k].GetT() - pos_.GetT();
		}
	}
	
	if (pos_.GetS() > minS && pos_.GetS() < maxS)
	{
		*longDist = 0.0;  // Overlap, point is inside bounding box longitudinal span
	}
	else
	{
		*longDist = minLongDist;
	}

	if (pos_.GetT() > minT && pos_.GetT() < maxT)
	{
		*latDist = 0.0;  // Overlap, point is inside bounding box lateral span
	}
	else
	{
		*latDist = minLatDist;
	}

	// Ajust direction according to object heading
	if (pos_.GetHRelative() > M_PI_2 && pos_.GetHRelative() < 3 * M_PI_2)
	{
		*latDist *= -1;
		*longDist *= -1;
	}

	return 0;
}

int Object::FreeSpaceDistanceObjectRoadLane(Object* target, double* latDist, double* longDist, CoordinateSystem cs)
{
	if (cs != CoordinateSystem::CS_LANE && cs != CoordinateSystem::CS_ROAD)
	{
		LOG("Unexpected coordinateSystem (%d). %d or %d expected.", CoordinateSystem::CS_LANE, CoordinateSystem::CS_ROAD);
		return -1;
	}

	if (cs == CoordinateSystem::CS_LANE)
	{
		LOG("freespace LANE coordinateSystem not supported yet, falling back to freespace ROAD");
		cs = CoordinateSystem::CS_ROAD;
	}

	double vertices[2][4][2];

	for (int i = 0; i < 2; i++)  // for each of the two BBs
	{
		Object* obj = (i == 0 ? this : target);

		// Specify bounding box corner vertices, starting at first quadrant
		double vtmp[4][2] =
		{
			{ obj->boundingbox_.center_.x_ + obj->boundingbox_.dimensions_.length_ / 2.0, obj->boundingbox_.center_.y_ + obj->boundingbox_.dimensions_.width_ / 2.0 },
			{ obj->boundingbox_.center_.x_ - obj->boundingbox_.dimensions_.length_ / 2.0, obj->boundingbox_.center_.y_ + obj->boundingbox_.dimensions_.width_ / 2.0 },
			{ obj->boundingbox_.center_.x_ - obj->boundingbox_.dimensions_.length_ / 2.0, obj->boundingbox_.center_.y_ - obj->boundingbox_.dimensions_.width_ / 2.0 },
			{ obj->boundingbox_.center_.x_ + obj->boundingbox_.dimensions_.length_ / 2.0, obj->boundingbox_.center_.y_ - obj->boundingbox_.dimensions_.width_ / 2.0 }
		};

		for (int j = 0; j < 4; j++)  // for all vertices
		{
			// Align points to object heading and position
			RotateVec2D(vtmp[j][0], vtmp[j][1], obj->pos_.GetH(), vertices[i][j][0], vertices[i][j][1]);
			vertices[i][j][0] += obj->pos_.GetX();
			vertices[i][j][1] += obj->pos_.GetY();
		}
	}

	// Evaluate road and lane coordinates of each point
	Position pos[2][4];
	double maxS[2] = { 0.0, 0.0 };
	double minS[2] = { LARGE_NUMBER, LARGE_NUMBER };
	double maxT[2] = { 0.0, 0.0 };
	double minT[2] = { LARGE_NUMBER, LARGE_NUMBER };
	for (int k = 0; k < 2; k++)
	{
		for (int l = 0; l < 4; l++)
		{ 
			pos[k][l].SetInertiaPos(vertices[k][l][0], vertices[k][l][1], 0.0);

			if (l == 0 || pos[k][l].GetS() < minS[k])
			{
				minS[k] = pos[k][l].GetS();
			}

			if (l == 0 || pos[k][l].GetT() < minT[k])
			{
				minT[k] = pos[k][l].GetT();
			}

			if (l == 0 || pos[k][l].GetS() > maxS[k])
			{
				maxS[k] = pos[k][l].GetS();
			}

			if (l == 0 || pos[k][l].GetT() > maxT[k])
			{
				maxT[k] = pos[k][l].GetT();
			}
		}
	}

	// Find min distance for each dimension
	double minLatDist = LARGE_NUMBER;
	double minLongDist = LARGE_NUMBER;

	for (int k = 0; k < 4; k++)
	{
		for (int l = 0; l < 4; l++)
		{
			if (k == 0 || fabs(pos[1][k].GetS() - pos[0][l].GetS()) < fabs(minLongDist))
			{
				minLongDist = pos[1][k].GetS() - pos[0][l].GetS();
			}

			if (k == 0 || fabs(pos[1][k].GetT() - pos[0][l].GetT()) < fabs(minLatDist))
			{
				minLatDist = pos[1][k].GetT() - pos[0][l].GetT();
			}
		}
	}

	if (minS[0] > minS[1] && minS[0] < maxS[1] ||
		maxS[0] > minS[1] && maxS[0] < maxS[1])
	{
		*longDist = 0.0;  // Overlap, point is inside bounding box longitudinal span
	}
	else
	{
		*longDist = minLongDist;
	}

	if (minT[0] > minT[1] && minT[0] < maxT[1])
	{
		*latDist = 0.0;  // Overlap, point is inside bounding box lateral span
	}
	else
	{
		*latDist = minLatDist;
	}

	// Ajust direction according to object heading
	if (pos_.GetHRelative() > M_PI_2 && pos_.GetHRelative() < 3 * M_PI_2)
	{
		*latDist *= -1;
		*longDist *= -1;
	}

	return 0;
}

int Object::Distance(Object* target, roadmanager::CoordinateSystem cs, roadmanager::RelativeDistanceType relDistType, bool freeSpace, double& dist)
{
	if (freeSpace)
	{
		double latDist, longDist;

		if (cs == roadmanager::CoordinateSystem::CS_ENTITY ||
			relDistType == RelativeDistanceType::REL_DIST_EUCLIDIAN ||
			relDistType == RelativeDistanceType::REL_DIST_CARTESIAN)
		{
			// Get Cartesian/Euclidian distance
			dist = FreeSpaceDistance(target, &latDist, &longDist);

			// sign indicates target being in front (+) or behind (-) 
			dist *= SIGN(longDist);

			if (cs == roadmanager::CoordinateSystem::CS_ENTITY)
			{
				if (relDistType == roadmanager::RelativeDistanceType::REL_DIST_LATERAL)
				{
					dist = latDist;
				}
				else if (relDistType == roadmanager::RelativeDistanceType::REL_DIST_LONGITUDINAL)
				{
					dist = longDist;
				}
			}
		}
		else if (cs == CoordinateSystem::CS_ROAD || cs == CoordinateSystem::CS_LANE)
		{
			if (FreeSpaceDistanceObjectRoadLane(target, &latDist, &longDist, cs) != 0)
			{
				return -1;
			}
			else
			{
				if (relDistType == RelativeDistanceType::REL_DIST_LATERAL)
				{
					dist = latDist;
				}
				else if (relDistType == RelativeDistanceType::REL_DIST_LONGITUDINAL)
				{
					dist = longDist;
				}
				else
				{
					LOG("Unexpected relativeDistanceType: %d", relDistType);
					return -1;
				}
			}
		}
		else
		{
			LOG("Unhandled case: cs %d reDistType %d freeSpace %d\n", cs, relDistType, freeSpace);
			return -1;
		}
	}
	else  // not freeSpace
	{
		return pos_.Distance(&target->pos_, cs, relDistType, dist);
	}

	return 0;
}

int Object::Distance(double x, double y, roadmanager::CoordinateSystem cs, roadmanager::RelativeDistanceType relDistType, bool freeSpace, double& dist)
{
	if (freeSpace)
	{
		double latDist, longDist;

		if (cs == roadmanager::CoordinateSystem::CS_ENTITY || 
			relDistType == RelativeDistanceType::REL_DIST_EUCLIDIAN || 
			relDistType == RelativeDistanceType::REL_DIST_CARTESIAN)
		{
			// Get Cartesian/Euclidian distance
			dist = FreeSpaceDistancePoint(x, y, &latDist, &longDist);

			// sign indicates target being in front (+) or behind (-) 
			dist *= SIGN(longDist);

			if (cs == roadmanager::CoordinateSystem::CS_ENTITY)
			{
				if (relDistType == roadmanager::RelativeDistanceType::REL_DIST_LATERAL)
				{
					dist = latDist;
				}
				else if (relDistType == roadmanager::RelativeDistanceType::REL_DIST_LONGITUDINAL)
				{
					dist = longDist;
				}
			}
		}
		else if (cs == CoordinateSystem::CS_ROAD || cs == CoordinateSystem::CS_LANE)
		{
			if (FreeSpaceDistancePointRoadLane(x, y, &latDist, &longDist, cs) != 0)
			{
				return -1;
			}
			else
			{
				if (relDistType == RelativeDistanceType::REL_DIST_LATERAL)
				{
					dist = latDist;
				}
				else if (relDistType == RelativeDistanceType::REL_DIST_LONGITUDINAL)
				{
					dist = longDist;
				}
				else
				{
					LOG("Unexpected relativeDistanceType: %d", relDistType);
					return -1;
				}
			}
		}
		else 
		{
			LOG("Unhandled case: cs %d reDistType %d freeSpace %d\n", cs, relDistType, freeSpace);
			return -1;	
		}
	}
	else  // not freeSpace
	{
		return pos_.Distance(x, y, cs, relDistType, dist);
	}
	
	return 0;
}

int Entities::addObject(Object* obj)
{
	obj->id_ = getNewId();
	object_.push_back(obj);
	return obj->id_;
}

void Entities::removeObject(int id) 
{
	for (size_t i = 0; i < object_.size(); i++) 
	{
		if (object_[i]->id_ == id) 
		{
			object_.erase(object_.begin() + i);
		}
	}
}

void Entities::removeObject(std::string name)
{
	for (size_t i = 0; i < object_.size(); i++) 
	{
		if (object_[i]->name_ == name) 
		{
			object_.erase(object_.begin() + i);
		}
	}
}

bool Entities::nameExists(std::string name)
{
	for (size_t i = 0; i < object_.size(); i++) 
	{
		if (object_[i]->name_ == name) 
		{
			return true;
		}
	}
	return false;
}

bool Entities::indexExists(int id) 
{
	for (size_t i = 0; i < object_.size(); i++) 
	{
		if (object_[i]->id_ == id) 
		{
			return false;
		}
	}
	return true;
}

int Entities::getNewId()
{
	int retint = 0;
	while (!indexExists(retint))
	{
		retint += 1;
	}
	return retint;
}


Object* Entities::GetObjectByName(std::string name)
{
	for (size_t i = 0; i < object_.size(); i++)
	{
		if (name == object_[i]->name_)
		{
			return object_[i];
		}
	}

	LOG("Failed to find object %s", name.c_str());
	
	return 0;
}

Object* Entities::GetObjectById(int id)
{
	for (size_t i = 0; i < object_.size(); i++)
	{
		if (id == object_[i]->id_)
		{
			return object_[i];
		}
	}

	LOG("Failed to find object with id %d", id);

	return 0;
}
