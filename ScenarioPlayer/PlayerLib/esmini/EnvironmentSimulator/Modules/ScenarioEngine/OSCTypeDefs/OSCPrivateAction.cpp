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

#define _USE_MATH_DEFINES
#include <math.h>

#include "OSCPrivateAction.hpp"

#define MAX(x, y) (y > x ? y : x)
#define MIN(x, y) (y < x ? y : x)
#define MAX_DECELERATION -8.0
#define LONGITUDINAL_DISTANCE_THRESHOLD 0.1

using namespace scenarioengine;

double OSCPrivateAction::TransitionDynamics::Evaluate(double factor, double start_value, double end_value)
{
	if (factor > 1.0)
	{
		factor = 1.0;
	}
	if (shape_ == DynamicsShape::STEP)
	{
		return end_value;
	}
	else if (shape_ == DynamicsShape::LINEAR)
	{
		return start_value + factor * (end_value - start_value);
	}
	else if (shape_ == DynamicsShape::SINUSOIDAL)
	{
		// cosine(angle + PI) gives a value in interval [-1 : 1], add 1 and normalize (divide by 2)
		double val = start_value + (end_value - start_value) * (1 + cos(M_PI * (1 + factor))) / 2.0;
		return val;
	}
	else if (shape_ == DynamicsShape::CUBIC)
	{
		// Equation: https://www.desmos.com/calculator/j9h7jzcowe
		// t(3(x/d)^2-2(x/d)^3)
		double val = start_value + (end_value - start_value) * (3 * factor * factor - 2 * factor * factor * factor);
		return val;
	}
	else
	{
		LOG("Invalid Dynamics shape: %d", shape_);
	}
	
	return end_value;
}

void AssignRouteAction::Start()
{
	object_->pos_.SetRoute(route_);

	OSCAction::Start();

	if (object_->GetControllerMode() == Controller::Mode::MODE_OVERRIDE &&
		object_->IsControllerActiveOnDomains(Controller::Domain::CTRL_LATERAL))
	{
		// lateral motion controlled elsewhere
		return;
	}
}

void AssignRouteAction::End()
{
	// Disconnect route
	object_->pos_.SetRoute(0);

	OSCAction::End();
}

void AssignRouteAction::ReplaceObjectRefs(Object* obj1, Object* obj2)
{
	if (object_ == obj1)
	{
		object_ = obj2;
	}
	for (size_t i = 0; i < route_->waypoint_.size(); i++)
	{
		route_->waypoint_[i]->ReplaceObjectRefs(&obj1->pos_, &obj2->pos_);
	}
}

void FollowTrajectoryAction::Start()
{
	OSCAction::Start();

	if (object_->GetControllerMode() == Controller::Mode::MODE_OVERRIDE &&
		object_->IsControllerActiveOnDomains(Controller::Domain::CTRL_LATERAL))
	{
		// lateral motion controlled elsewhere
		// other action or controller already updated lateral dimension of object 
		// potentially longitudinal dimension could be updated separatelly - but skip that for now
		return;
	}

	traj_->Freeze();
	object_->pos_.SetTrajectory(traj_);

	object_->pos_.SetTrajectoryS(initialDistanceOffset_);
	time_ = traj_->GetTimeAtS(initialDistanceOffset_);

	// We want the trajectory to be projected on road surface.
	object_->pos_.SetAlignMode(roadmanager::Position::ALIGN_MODE::ALIGN_HARD);

	// But totally decouple trajectory positioning from road heading
	object_->pos_.SetAlignModeH(roadmanager::Position::ALIGN_MODE::ALIGN_SOFT);

	object_->SetDirtyBits(Object::DirtyBit::LATERAL | Object::DirtyBit::LONGITUDINAL);
}

void FollowTrajectoryAction::End()
{
	OSCAction::End();

	if (object_->GetControllerMode() == Controller::Mode::MODE_OVERRIDE &&
		object_->IsControllerActiveOnDomains(Controller::Domain::CTRL_LATERAL))
	{
		return;
	}

	// Disconnect trajectory
	object_->pos_.SetTrajectory(0);

	// And reset align mode
	object_->pos_.SetAlignMode(roadmanager::Position::ALIGN_MODE::ALIGN_SOFT);
}

void FollowTrajectoryAction::Step(double dt, double simTime)
{
	if (object_->GetControllerMode() == Controller::Mode::MODE_OVERRIDE &&
		object_->IsControllerActiveOnDomains(Controller::Domain::CTRL_LATERAL))
	{
		// lateral motion controlled elsewhere
		// other action or controller already updated lateral dimension of object 
		// potentially longitudinal dimension could be updated separatelly - but skip that for now
		return;
	}

	time_ += timing_scale_ * dt;

	// Measure length of movement for odometer

	if (!traj_->closed_ && object_->pos_.GetTrajectoryS() > (traj_->GetLength() - DISTANCE_TOLERANCE))
	{
		// Reached end of trajectory
		// Calculate road coordinates from final inertia (X, Y) coordinates
		object_->pos_.XYZH2TrackPos(object_->pos_.GetX(), object_->pos_.GetY(), 0, object_->pos_.GetH());
		
		End();
	}
	else
	{
		// Move along trajectory
		if (
			// Ignore any timing info in trajectory
			timing_domain_ == TimingDomain::NONE ||
			// Speed is controlled elsewhere - just follow trajectory with current speed
			(object_->GetControllerMode() == Controller::Mode::MODE_OVERRIDE &&
			 object_->IsControllerActiveOnDomains(Controller::Domain::CTRL_LONGITUDINAL)))
		{
			object_->pos_.MoveTrajectoryDS(object_->speed_ * dt);
			object_->SetDirtyBits(Object::DirtyBit::LATERAL | Object::DirtyBit::LONGITUDINAL);
		}
		else if (timing_domain_ == TimingDomain::TIMING_RELATIVE)
		{
			double s_old = object_->pos_.GetTrajectoryS();
			object_->pos_.SetTrajectoryPosByTime(time_ + timing_offset_);
			object_->SetSpeed((object_->pos_.GetTrajectoryS() - s_old) / MAX(SMALL_NUMBER, dt));
			object_->SetDirtyBits(Object::DirtyBit::LATERAL | Object::DirtyBit::LONGITUDINAL);
		}
		else if (timing_domain_ == TimingDomain::TIMING_ABSOLUTE)
		{
			double s_old = object_->pos_.GetTrajectoryS();
			object_->pos_.SetTrajectoryPosByTime(simTime * timing_scale_ + timing_offset_);
			object_->SetSpeed((object_->pos_.GetTrajectoryS() - s_old) / MAX(SMALL_NUMBER, dt));
			object_->SetDirtyBits(Object::DirtyBit::LATERAL | Object::DirtyBit::LONGITUDINAL);
		}
	}
}

void FollowTrajectoryAction::ReplaceObjectRefs(Object* obj1, Object* obj2)
{
	if (object_ == obj1)
	{
		object_ = obj2;
	}
	if (traj_->shape_->type_ == roadmanager::Shape::ShapeType::CLOTHOID)
	{
		roadmanager::ClothoidShape* cl = (roadmanager::ClothoidShape*)traj_->shape_;
		cl->pos_.ReplaceObjectRefs(&obj1->pos_, &obj2->pos_);
	}
	else if (traj_->shape_->type_ == roadmanager::Shape::ShapeType::POLYLINE)
	{
		roadmanager::PolyLineShape* pl = (roadmanager::PolyLineShape*)traj_->shape_;
		for (size_t i = 0; i < pl->vertex_.size(); i++)
		{
			pl->vertex_[i]->pos_.ReplaceObjectRefs(&obj1->pos_, &obj2->pos_);
		}
	}

}

void AssignControllerAction::Start()
{
	if (controller_ == 0)
	{
		// Detach any controller from object
		if (object_->controller_)
		{
			Controller* ctrl = (Controller*)object_->controller_;
			ctrl->Assign(0);
			object_->SetAssignedController(0);
		}
	}
	else
	{
		controller_->Assign(object_);
	}

	OSCAction::Start();
}


void LatLaneChangeAction::Start()
{
	OSCAction::Start();

	if (object_->GetControllerMode() == Controller::Mode::MODE_OVERRIDE &&
		object_->IsControllerActiveOnDomains(Controller::Domain::CTRL_LATERAL))
	{
		// lateral motion controlled elsewhere
		return;
	}

	if (target_->type_ == Target::Type::ABSOLUTE)
	{
		target_lane_id_ = target_->value_;
	}
	else if (target_->type_ == Target::Type::RELATIVE)
	{
		// Find out target lane relative referred vehicle
		target_lane_id_ = ((TargetRelative*)target_)->object_->pos_.GetLaneId() + target_->value_;

		if (target_lane_id_ == 0 || SIGN(((TargetRelative*)target_)->object_->pos_.GetLaneId()) != SIGN(target_lane_id_))
		{
			// Skip reference lane (id == 0)
			target_lane_id_ = SIGN(target_lane_id_ - object_->pos_.GetLaneId()) * (abs(target_lane_id_) + 1);
		}
	}

	target_t_ =
		SIGN(target_lane_id_) *
		object_->pos_.GetOpenDrive()->GetRoadById(object_->pos_.GetTrackId())->GetCenterOffset(object_->pos_.GetS(), target_lane_id_) +
		target_lane_offset_;

	// if dynamics dimension is rate, transform into distance
	if (transition_dynamics_.dimension_ == DynamicsDimension::RATE)
	{
		double lat_distance = object_->pos_.GetT() - target_t_;
		double rate = fabs(transition_dynamics_.target_value_);

		if (transition_dynamics_.shape_ == DynamicsShape::LINEAR)
		{
			// longitudinal distance = long_speed * time = long_speed * lat_dist / lat_speed 
			if (fabs(rate) > SMALL_NUMBER)
			{
				transition_dynamics_.target_value_ = fabs(object_->speed_ * lat_distance / rate);
			}
			else
			{
				// rate close to zero. Choose a random large distance.
				transition_dynamics_.target_value_ = 500;
			}
		}
		else if (transition_dynamics_.shape_ == DynamicsShape::SINUSOIDAL)
		{
			// distance = longitudinal speed * duration
			// duration based on equation: https://www.desmos.com/calculator/kuq26iz0aw
			transition_dynamics_.target_value_ = object_->GetSpeed() * M_PI * fabs(lat_distance / (2 * rate));
		}
		else if (transition_dynamics_.shape_ == DynamicsShape::CUBIC)
		{
			// Equations: https://www.desmos.com/calculator/j9h7jzcowe
			// special case where duration is a function of lateral speed/rate
			// Calculate corresponding duration:
			// duration = 3 * lat_distance / (2 * rate) = distance / speed
			// Finally, distance = longitudinal speed * duration
			transition_dynamics_.target_value_ = object_->speed_ * 3 * fabs(lat_distance) / (2 * rate);
			LOG("Cubic shape with rate translated into distance=%.2f m", transition_dynamics_.target_value_);
		}
	}

	t_ = start_t_ = object_->pos_.GetT();

	elapsed_ = 0;
}

void LatLaneChangeAction::Step(double dt, double)
{
	double t_old = t_;
	double factor;
	double angle = 0;

	if (object_->GetControllerMode() == Controller::Mode::MODE_OVERRIDE &&
		object_->IsControllerActiveOnDomains(Controller::Domain::CTRL_LATERAL))
	{
		// lateral motion controlled elsewhere
		return;
	}

	if (transition_dynamics_.dimension_ == DynamicsDimension::TIME)
	{
		double dt_adjusted = dt;

		// Set a limit for lateral speed not to exceed longitudinal speed
		if (transition_dynamics_.target_value_ * object_->speed_ < fabs(target_t_ - start_t_))
		{
			dt_adjusted = dt * object_->speed_ * transition_dynamics_.target_value_ / fabs(target_t_ - start_t_);
		}
		elapsed_ += dt_adjusted;
	}
	else if (transition_dynamics_.dimension_ == DynamicsDimension::DISTANCE ||
			transition_dynamics_.dimension_ == DynamicsDimension::RATE)
	{
		elapsed_ += object_->speed_ * dt;
	}
	else
	{
		LOG("Unexpected timing type: %d", transition_dynamics_.dimension_);
	}

	factor = elapsed_ / transition_dynamics_.target_value_;
	t_ = transition_dynamics_.Evaluate(factor, start_t_, target_t_);

	if (object_->pos_.GetRoute())
	{
		// If on a route, stay in original lane
		int lane_id = object_->pos_.GetLaneId();
		object_->pos_.SetTrackPos(object_->pos_.GetTrackId(), object_->pos_.GetS(), t_);
		object_->pos_.ForceLaneId(lane_id);
	}
	else
	{
		object_->pos_.SetTrackPos(object_->pos_.GetTrackId(), object_->pos_.GetS(), t_);
	}
		

	if (factor > 1.0 || abs(t_ - target_t_) < SMALL_NUMBER || SIGN(t_ - start_t_) != SIGN(target_t_ - start_t_))
	{
		OSCAction::End();
		object_->pos_.SetHeadingRelativeRoadDirection(0);
	}
	else
	{
		if (object_->speed_ * dt > SMALL_NUMBER)
		{
			angle = atan((t_ - t_old) / (object_->speed_ * dt));
			object_->pos_.SetHeadingRelativeRoadDirection(angle);
		}
	}

	object_->SetDirtyBits(Object::DirtyBit::LATERAL);
}

void LatLaneChangeAction::ReplaceObjectRefs(Object* obj1, Object* obj2)
{
	if (object_ == obj1)
	{
		object_ = obj2;
	}

	if (target_->type_ == Target::Type::RELATIVE)
	{
		if (((TargetRelative*)target_)->object_ == obj1)
		{
			((TargetRelative*)target_)->object_ = obj2;
		}
	}
}

void LatLaneOffsetAction::Start()
{
	OSCAction::Start();

	if (object_->GetControllerMode() == Controller::Mode::MODE_OVERRIDE &&
		object_->IsControllerActiveOnDomains(Controller::Domain::CTRL_LATERAL))
	{
		// lateral motion controlled elsewhere
		return;
	}

	start_lane_offset_ = object_->pos_.GetOffset();

	if (target_->type_ == Target::Type::ABSOLUTE)
	{
		target_lane_offset_ = target_->value_;
	}
	else if (target_->type_ == Target::Type::RELATIVE)
	{
		// Register what lane action object belongs to
		int lane_id = object_->pos_.GetLaneId();

		// Find out referred object track position
		roadmanager::Position refpos = ((TargetRelative*)target_)->object_->pos_;
		refpos.SetTrackPos(refpos.GetTrackId(), refpos.GetS(), refpos.GetT() + target_->value_);
		refpos.ForceLaneId(lane_id);

		target_lane_offset_ = refpos.GetT() + target_->value_;
	}

	double duration = 0;
	// Convert max lateral acceleration to distance for given shape
	if (dynamics_.transition_.shape_ == DynamicsShape::STEP)
	{
		dynamics_.transition_.target_value_ = 0.0;
	}
	else if (dynamics_.transition_.shape_ == DynamicsShape::LINEAR ||
		dynamics_.transition_.shape_ == DynamicsShape::SINUSOIDAL)
	{
		if (dynamics_.transition_.shape_ == DynamicsShape::LINEAR)
		{
			// linear transition means infinite acc at t=0. Instead, use acceleration profile from sinusoidal
			LOG("Use sinusoidal shape for calculating linear laneOffset action duration based on max lateral acceleration");
		}
		// Equation for duration: https://www.desmos.com/calculator/peck6bzibp
		duration = sqrt(M_PI * M_PI * (fabs(target_lane_offset_ - object_->pos_.GetOffset())) / (dynamics_.max_lateral_acc_ * 2));
		dynamics_.transition_.target_value_ = object_->GetSpeed() * duration;
	}
	else if (dynamics_.transition_.shape_ == DynamicsShape::CUBIC)
	{
		// based on equation: https://www.desmos.com/calculator/j9h7jzcowe
		// special case where duration is a function of lateral acceleration
		// Calculate corresponding duration:
		// duration = sqrt(6t / a)
		// Finally, distance = longitudinal speed * duration
		duration = sqrt(6 * fabs(target_lane_offset_ - object_->pos_.GetOffset()) / dynamics_.max_lateral_acc_);
		dynamics_.transition_.target_value_ = object_->GetSpeed() * duration;;
	}
	else
	{
		throw std::runtime_error("Unexpected shape type: " + dynamics_.transition_.shape_);
	}
	LOG("Started LaneOffset with max lateral acc: %.2f -> duration: %.2f <=> distance: %.2f",
		dynamics_.max_lateral_acc_, duration, dynamics_.transition_.target_value_);
}

void LatLaneOffsetAction::Step(double dt, double)
{
	double factor, lane_offset;
	double angle = 0;
	double old_lane_offset = object_->pos_.GetOffset();

	if (object_->GetControllerMode() == Controller::Mode::MODE_OVERRIDE &&
		object_->IsControllerActiveOnDomains(Controller::Domain::CTRL_LATERAL))
	{
		// lateral motion controlled elsewhere
		return;
	}

	elapsed_ += object_->speed_ * dt;

	factor = elapsed_ / dynamics_.transition_.target_value_;

	lane_offset = dynamics_.transition_.Evaluate(factor, start_lane_offset_, target_->value_);

	object_->pos_.SetLanePos(object_->pos_.GetTrackId(), object_->pos_.GetLaneId(), object_->pos_.GetS(), lane_offset);

	if (object_->speed_ > SMALL_NUMBER)
	{
		angle = atan((lane_offset - old_lane_offset) / (object_->speed_ * dt));
	}

	if (factor > 1.0)
	{
		OSCAction::End();
		angle = 0;
	}

	object_->pos_.SetHeadingRelativeRoadDirection(angle);

	object_->SetDirtyBits(Object::DirtyBit::LATERAL);
}

void LatLaneOffsetAction::ReplaceObjectRefs(Object* obj1, Object* obj2)
{
	if (object_ == obj1)
	{
		object_ = obj2;
	}

	if (target_->type_ == Target::Type::RELATIVE)
	{
		if (((TargetRelative*)target_)->object_ == obj1)
		{
			((TargetRelative*)target_)->object_ = obj2;
		}
	}
}
double LongSpeedAction::TargetRelative::GetValue()
{
	if (!continuous_)
	{
		// sample relative object speed once
		if (!consumed_)
		{
			object_speed_ = object_->speed_;
			consumed_ = true;
		}
	}
	else 
	{
		object_speed_ = object_->speed_;
	}

	if (value_type_ == ValueType::DELTA)
	{
		return object_speed_ + value_;
	}
	else if (value_type_ == ValueType::FACTOR)
	{
		return object_speed_ * value_;
	}
	else
	{
		LOG("Invalid value type: %d", value_type_);
	}

	return 0;
}

void LongSpeedAction::Start()
{
	OSCAction::Start();

	if (object_->GetControllerMode() == Controller::Mode::MODE_OVERRIDE &&
		object_->IsControllerActiveOnDomains(Controller::Domain::CTRL_LONGITUDINAL))
	{
		// longitudinal motion controlled elsewhere
		return;
	}

	if (transition_dynamics_.shape_ == DynamicsShape::STEP)
	{
		object_->SetSpeed(target_->GetValue());
		if (!(target_->type_ == Target::TargetType::RELATIVE && ((TargetRelative*)target_)->continuous_ == true))
		{
			OSCAction::End();
		}
	}
	else
	{
		start_speed_ = object_->speed_;

		if (transition_dynamics_.dimension_ == DynamicsDimension::DISTANCE)
		{
			// Convert to time
			transition_dynamics_.target_value_ = 2 * transition_dynamics_.target_value_ / (start_speed_ + target_->GetValue());
		}
	}
 }

void LongSpeedAction::Step(double dt, double)
{
	double factor = 0.0;
	double new_speed = 0;
	bool target_speed_reached = false;

	if (object_->GetControllerMode() == Controller::Mode::MODE_OVERRIDE &&
		object_->IsControllerActiveOnDomains(Controller::Domain::CTRL_LONGITUDINAL))
	{
		// longitudinal motion controlled elsewhere
		return;
	}

	if (transition_dynamics_.dimension_ == DynamicsDimension::RATE)
	{
		elapsed_ += dt;
		
		// Assume user want to reach target speed, ignore sign of rate
		new_speed = start_speed_ + SIGN(target_->GetValue() - start_speed_) * fabs(transition_dynamics_.target_value_) * elapsed_;

		// Check if speed changed passed target value
		if (fabs(object_->speed_ - target_->GetValue()) < SMALL_NUMBER ||
			(object_->speed_ > target_->GetValue() && new_speed < target_->GetValue()) ||
			(object_->speed_ < target_->GetValue() && new_speed > target_->GetValue()))
		{
			new_speed = target_->GetValue();
			target_speed_reached = true;
		}
	}
	else 
	{
		elapsed_ += dt;

		factor = elapsed_ / (transition_dynamics_.target_value_);

		if(factor > 1.0)
		{
			new_speed = target_->GetValue();
			target_speed_reached = true;
		}
		else
		{
			new_speed = transition_dynamics_.Evaluate(factor, start_speed_, target_->GetValue());
		}
	}

	object_->SetSpeed(new_speed);

	if (target_speed_reached && !(target_->type_ == Target::TargetType::RELATIVE && ((TargetRelative*)target_)->continuous_ == true))
	{
		OSCAction::End();
	}
}

void LongSpeedAction::ReplaceObjectRefs(Object* obj1, Object* obj2)
{
	if (object_ == obj1)
	{
		object_ = obj2;
	}

	if (target_->type_ == Target::TargetType::RELATIVE)
	{
		if (((TargetRelative*)target_)->object_ == obj1)
		{
			((TargetRelative*)target_)->object_ = obj2;
		}
	}
}

void LongDistanceAction::Start()
{
	if (target_object_ == 0)
	{
		LOG("Can't trig without set target object ");
		return;
	}

	OSCAction::Start();
}

void LongDistanceAction::Step(double dt, double)
{
	if (object_->GetControllerMode() == Controller::Mode::MODE_OVERRIDE &&
		object_->IsControllerActiveOnDomains(Controller::Domain::CTRL_LONGITUDINAL))
	{
		// longitudinal motion controlled elsewhere
		return;
	}

	// Find out current distance
	double distance;
	if (freespace_)
	{
		double latDist = 0;
		double longDist = 0;
		object_->FreeSpaceDistance(target_object_, &latDist, &longDist);
		distance = longDist;
	}
	else
	{
		double x, y;
		distance = object_->pos_.getRelativeDistance(target_object_->pos_.GetX(), target_object_->pos_.GetY(), x, y);

		// Just interested in the x-axis component of the distance
		distance = x;
	}

	double speed_diff = object_->speed_ - target_object_->speed_;
	double acc;
	double spring_constant = 4;
	double dc;
	double requested_dist = 0;

	if (dist_type_ == DistType::DISTANCE)
	{
		requested_dist = distance_;
	}
	if (dist_type_ == DistType::TIME_GAP)
	{
		// Convert requested time gap (seconds) to distance (m)
		requested_dist = object_->speed_ * distance_;
	}

	double distance_diff = distance - requested_dist;

	if (continuous_ == false && fabs(distance_diff) < LONGITUDINAL_DISTANCE_THRESHOLD)
	{
		// Reached requested distance, quit action
		OSCAction::End();
	}

	if (dynamics_.none_ == true)
	{
		// Set position according to distance and copy speed of target vehicle
		object_->pos_.MoveAlongS(distance_diff);
		object_->SetSpeed(target_object_->speed_);

		object_->SetDirtyBits(Object::DirtyBit::LONGITUDINAL);
	}
	else
	{
		// Apply damped spring model with critical/optimal damping factor		
		// Adjust tension in spring in proportion to max acceleration. Experimental, may be removed.
		double spring_constant_adjusted = 0.1 * dynamics_.max_acceleration_ * spring_constant;
		dc = 2 * sqrt(spring_constant_adjusted);
		acc = distance_diff * spring_constant_adjusted - speed_diff * dc;
		if (acc > dynamics_.max_acceleration_)
		{
			acc = dynamics_.max_acceleration_;
		}
		else if (acc < -dynamics_.max_deceleration_)
		{
			acc = -dynamics_.max_deceleration_;
		}

		object_->SetSpeed(object_->GetSpeed() + acc * dt);

		if (object_->GetSpeed() > dynamics_.max_speed_)
		{
			object_->SetSpeed(dynamics_.max_speed_);
		}
		else if (object_->GetSpeed() < -dynamics_.max_speed_)
		{
			object_->SetSpeed(-dynamics_.max_speed_);
		}
	}

	//	LOG("Dist %.2f diff %.2f acc %.2f speed %.2f", distance, distance_diff, acc, object_->speed_);
}

void LongDistanceAction::ReplaceObjectRefs(Object* obj1, Object* obj2)
{
	if (object_ == obj1)
	{
		object_ = obj2;
	}

	if (target_object_ == obj1)
	{
		target_object_ = obj2;
	}
}

void TeleportAction::Start()
{
	OSCAction::Start();

	if (object_->GetControllerMode() == Controller::Mode::MODE_OVERRIDE &&
		object_->IsControllerActiveOnDomains(Controller::Domain::CTRL_LONGITUDINAL | Controller::Domain::CTRL_LONGITUDINAL))
	{
		// motion controlled elsewhere
		return;
	}

	roadmanager::Position tmpPos;

	if (position_->GetRelativePosition() == &object_->pos_)
	{
		// Special case: Relative to itself - need to make a copy before reseting
		tmpPos = object_->pos_;

		position_->SetRelativePosition(&tmpPos, position_->GetType());
	}

	object_->pos_.CopyRMPos(position_);

	// Resolve any relative positions
	object_->pos_.ReleaseRelation();

	if (object_->pos_.GetType() == roadmanager::Position::PositionType::ROUTE)
	{
		object_->pos_.CalcRoutePosition();
	}

	LOG("%s New position:", object_->name_.c_str());
	object_->pos_.Print();
	object_->SetDirtyBits(Object::DirtyBit::LATERAL | Object::DirtyBit::LONGITUDINAL);
	object_->reset_ = true;
}

void TeleportAction::Step(double dt, double simTime)
{
	(void)dt;
	(void)simTime;

	OSCAction::Stop();
}

void TeleportAction::ReplaceObjectRefs(Object* obj1, Object* obj2)
{
	if (object_ == obj1)
	{
		object_ = obj2;
	}
}

double SynchronizeAction::CalcSpeedForLinearProfile(double v_final, double time, double dist)
{
	if (time < 0.001 || dist < 0.001)
	{
		// Avoid division by zero, fall back to zero acceleration
		return 0;
	}

	// Compute current speed needed to reach given final speed in given time
	double v0 = 2 * dist / time - v_final;

	return v0;
}

const char* SynchronizeAction::Mode2Str(SynchMode mode)
{
	if (mode == SynchMode::MODE_NONE)
	{
		return "MODE_NONE";
	}
	else if (mode == SynchMode::MODE_NON_LINEAR)
	{
		return "MODE_NON_LINEAR";
	}
	else if (mode == SynchMode::MODE_LINEAR)
	{
		return "MODE_LINEAR";
	}
	else if (mode == SynchMode::MODE_STOPPED)
	{
		return "MODE_STOPPED";
	}
	else if (mode == SynchMode::MODE_STOP_IMMEDIATELY)
	{
		return "MODE_STOP_IMMEDIATELY";
	}
	else if (mode == SynchMode::MODE_WAITING)
	{
		return "MODE_WAITING";
	}
	else if (mode == SynchMode::MODE_STEADY_STATE)
	{
		return "MODE_STEADY_STATE";
	}
	else
	{
		return "Unknown mode";
	}
}

const char* SynchronizeAction::SubMode2Str(SynchSubmode submode)
{
	if (submode == SynchSubmode::SUBMODE_CONCAVE)
	{
		return "SUBMODE_CONCAVE";
	}
	else if (submode == SynchSubmode::SUBMODE_CONVEX)
	{
		return "SUBMODE_CONVEX";
	}
	else if (submode == SynchSubmode::SUBMODE_NONE)
	{
		return "SUBMODE_NONE";
	}
	else
	{
		return "Unknown sub-mode";
	}
}

void SynchronizeAction::PrintStatus(const char* custom_msg)
{
	LOG("%s, mode=%s (%d) sub-mode=%s (%d)", custom_msg,
		Mode2Str(mode_), mode_, SubMode2Str(submode_), submode_);
}

void SynchronizeAction::Start()
{
	// resolve steady state -> translate into dist
	if (steadyState_.type_ == SteadyStateType::STEADY_STATE_TIME)
	{
		steadyState_.dist_ = steadyState_.time_ * final_speed_->GetValue();
		steadyState_.type_ = SteadyStateType::STEADY_STATE_DIST;
	}
	else if (steadyState_.type_ == SteadyStateType::STEADY_STATE_POS)
	{
		// Find out distance between steady state position and final destination
		roadmanager::PositionDiff diff;
		target_position_->Delta(steadyState_.pos_, diff);
		steadyState_.dist_ = diff.ds;
		steadyState_.type_ = SteadyStateType::STEADY_STATE_DIST;
	}

	OSCAction::Start();

	if (object_->GetControllerMode() == Controller::Mode::MODE_OVERRIDE &&
		object_->IsControllerActiveOnDomains(Controller::Domain::CTRL_LONGITUDINAL))
	{
		// longitudinal motion controlled elsewhere
		return;
	}
}

void SynchronizeAction::Step(double dt, double)
{
	(void)dt;
	bool done = false;

	if (object_->GetControllerMode() == Controller::Mode::MODE_OVERRIDE &&
		object_->IsControllerActiveOnDomains(Controller::Domain::CTRL_LONGITUDINAL))
	{
		// longitudinal motion controlled elsewhere
		return;
	}

	// Calculate distance along road/route
	double masterDist, dist;
	roadmanager::PositionDiff diff;

	if (master_object_->pos_.GetTrajectory() || !master_object_->pos_.Delta(target_position_master_, diff))
	{
		// No road network path between master vehicle and master target pos - using world coordinate distance
		diff.ds = GetLengthOfLine2D(master_object_->pos_.GetX(), master_object_->pos_.GetY(),
			target_position_master_->GetX(), target_position_master_->GetY());
	}
	masterDist = fabs(diff.ds);

	if (object_->pos_.GetTrajectory() || !object_->pos_.Delta(target_position_, diff))
	{
		// No road network path between action vehicle and action target pos - using world coordinate distance
		diff.ds = GetLengthOfLine2D(object_->pos_.GetX(), object_->pos_.GetY(),
			target_position_->GetX(), target_position_->GetY());
	}
	dist = fabs(diff.ds);

	// Done when distance increases, indicating that destination just has been reached or passed
	if (dist < tolerance_ + SMALL_NUMBER)
	{
		LOG("Synchronize dist (%.2f) < tolerance (%.2f)", dist, tolerance_);
		if (final_speed_)
		{
			object_->SetSpeed(final_speed_->GetValue());
		}
		done = true;
	}
	else if (masterDist < tolerance_master_ + SMALL_NUMBER)
	{
		LOG("Synchronize masterDist (%.2f) < tolerance (%.2f)", masterDist, tolerance_master_);
		if (final_speed_)
		{
			object_->SetSpeed(final_speed_->GetValue());
		}
		done = true;
	}
	else if (dist > lastDist_)
	{
		LOG("Synchronize dist increasing (%.2f > %.2f) - missed destination", dist, lastDist_);
		done = true;
	}
	else if (masterDist > lastMasterDist_)
	{
		LOG("Synchronize masterDist increasing (%.2f > %.2f) - missed destination", masterDist, lastMasterDist_);
		done = true;
	}

	lastDist_ = dist;
	lastMasterDist_ = masterDist;

	// for calculations, measure distance to toleration area/radius
	dist = MAX(dist - tolerance_, SMALL_NUMBER);
	masterDist = MAX(masterDist - tolerance_master_, SMALL_NUMBER);

	if (done)
	{
		OSCAction::End();
	}
	else
	{
		double masterTimeToDest = LARGE_NUMBER;
		if (master_object_->speed_ > SMALL_NUMBER)
		{
			masterTimeToDest = masterDist / master_object_->speed_;
		}
		double average_speed = dist / masterTimeToDest;
		double acc = 0;

		if (final_speed_)
		{
			if (steadyState_.type_ != SteadyStateType::STEADY_STATE_NONE && mode_ != SynchMode::MODE_STEADY_STATE)
			{
				double time_to_ss = steadyState_.dist_ / final_speed_->GetValue();
				
				if (dist - steadyState_.dist_ < SMALL_NUMBER || masterTimeToDest - time_to_ss < SMALL_NUMBER)
				{
					mode_ = SynchMode::MODE_STEADY_STATE;
					submode_ = SynchSubmode::SUBMODE_NONE;
					if (time_to_ss > masterTimeToDest && (time_to_ss - masterTimeToDest) * final_speed_->GetValue() > tolerance_)
					{
						LOG("Entering Stead State according to criteria but not enough time to reach destination");
					}
					//PrintStatus("SteadyState");
				}
				else
				{
					// subtract steady state distance
					dist -= steadyState_.dist_;

					// subtract steady state duration
					masterTimeToDest -= time_to_ss;
				}
			}

			// For more information about calculations, see 
			// https://docs.google.com/document/d/1dEBUWlJVLUz6Rp9Ol1l90iG0LfNtcsgLyJ0kDdwgPzA/edit?usp=sharing
			// 
			// Interactive Python script plotting calculation result based on various input values
			// https://drive.google.com/file/d/1z902gRYogkLhUAV1pZLc9gcgwnak7TBH/view?usp=sharing
			// (the method described below is "Spedified final speed - alt 1")
			//
			// Here follow a brief description:
			// 
			// Calculate acceleration needed to reach the destination in due time
			// Four cases
			//   1  Linear. Reach final speed with constant acceleration
			//	 2a Non-linear convex (rush). First accelerate, then decelerate.
			//	 2b Non-linear concave (linger). First decelerate, then accelerate.
			//   3  Non-linear with stop. Decelerate to a stop. Wait. Then accelerate.
			//
			//   Case 2-3 involves two (case 2a, 2b) or three (case 3) phases with constant acceleration/deceleration
			//   Last phase in case 2-3 is actually case 1 - a linear change to final speed
			// 
			// Symbols
			//   given:
			//     s = distance to destination
			//     t = master object time to destination
			//     v0 = current speed
			//     v1 = final speed
			//     va = Average speed needed to reach destination 
			//   variables:
			//     s1 = distance first phase 
			//     s2 = distance second phase
			//     x = end time for first phase
			//     y = end time for second (last) phase
			//     vx = speed at x
			// 
			// Equations
			//   case 1
			//     v1 = 2 * s / t - v2
			//     a = (v2 - v1) / t
			//
			//   case 2 (a & b)
			//      system: 
			//        s1 = v1 * x + (vx - v1) * x / 2
			//        s2 = v2 * y + (vx - v2) * y / 2
			//        t = x + y
			//        s = s1 + s2
			// 		  (vx - v1) / x = (vx - v2) / y
			//
			//      solve for x and vx   
			//      a = (vx - v1) / x
			// 
			//   case 3 
			//      system: 
			//        s1 = x * v1 / 2
			//        s2 = y * v2 / 2
			//        s = s1 + s2
			//        v1 / v2 = x / y
			//      
			//      solve for x
			//      a = -v1 / x

			if (mode_ == SynchMode::MODE_STEADY_STATE)
			{
				object_->speed_ = final_speed_->GetValue();
				return;
			}
			if (mode_ == SynchMode::MODE_WAITING)
			{
				if (masterTimeToDest >= LARGE_NUMBER)
				{
					// Continue waiting
					return;
				}
				else
				{
					// Reset mode
					mode_ = SynchMode::MODE_NONE;
				}
			}
			if (mode_ == SynchMode::MODE_STOP_IMMEDIATELY)
			{
				acc = MAX_DECELERATION;
				object_->speed_ += acc * dt;
				if (object_->speed_ < 0)
				{
					object_->SetSpeed(0);
					mode_ = SynchMode::MODE_WAITING;  // wait for master to move
					//PrintStatus("Waiting");
				}

				return;
			}
			else if (mode_ == SynchMode::MODE_STOPPED)
			{
				if (masterTimeToDest < 2 * dist / final_speed_->GetValue())
				{
					// Time to move again after the stop
					mode_ = SynchMode::MODE_LINEAR;
					//PrintStatus("Restart");
				}
				else
				{
					// Stay still
					object_->SetSpeed(0);
					return;
				}
			}
			
			if (mode_ == SynchMode::MODE_LINEAR)
			{
				if (masterTimeToDest > LARGE_NUMBER - 1)
				{
					// Master in effect standing still, do not move
					object_->SetSpeed(0);
				}
				else
				{
					object_->SetSpeed(MAX(0, CalcSpeedForLinearProfile(MAX(0, final_speed_->GetValue()), masterTimeToDest, dist)));
				}
				return;
			}
			else if (mode_ == SynchMode::MODE_NON_LINEAR && masterTimeToDest < LARGE_NUMBER)
			{
				// Check if case 1, i.e. on a straight speed profile line
				double v0_onLine = 2 * dist / masterTimeToDest - final_speed_->GetValue();

				if (fabs(object_->speed_ - v0_onLine) < 0.1)
				{
					// Passed apex. Switch to linear mode (constant acc) to reach final destination and speed
					mode_ = SynchMode::MODE_LINEAR;

					// Keep current speed for this time step
					return;
				}
			}
			
			if (mode_ == SynchMode::MODE_NONE)
			{
				// Since not linear mode, go into non-linear mode
				mode_ = SynchMode::MODE_NON_LINEAR;

				// Find out submode, convex or concave
				if ((final_speed_->GetValue() + object_->speed_) / 2 < dist / masterTimeToDest)
				{
					submode_ = SynchSubmode::SUBMODE_CONVEX;
				}
				else
				{
					submode_ = SynchSubmode::SUBMODE_CONCAVE;
				}
				//PrintStatus("Non-linear");
			}

			// Now, calculate x and vx according to default method oulined in the documentation
			double s = dist;
			double t = masterTimeToDest;
			double v0 = object_->speed_;
			double v1 = final_speed_->GetValue();

			double signed_term = sqrt(2.0) * sqrt(2.0 * s*s - 2 * (v1 + v0)*t*s + (v1*v1 + v0 * v0)*t*t);

			// Calculate both solutions from the quadratic equation
			double vx = 0;
			if (fabs(v1 - v0) < SMALL_NUMBER)
			{
				// When v0 == v1, x is simply t/2
				// s = (T / 2) * v_cur + (T / 2) * vx -> vx = (s - (T / 2) * v_cur) / (T / 2)
				vx = (s - (t / 2) * v0) / (t / 2);
				acc = (vx - v0) / (t / 2);
			}
			else
			{
				double x1 = -(signed_term + 2 * s - 2 * v1*t) / (2 * (v1 - v0));
				double x2 = -(-signed_term + 2 * s - 2 * v1*t) / (2 * (v1 - v0));
				double vx1 = (2 * s - signed_term) / (2 * t);
				double vx2 = (2 * s + signed_term) / (2 * t);
				double a1 = (vx1 - v0) / x1;
				double a2 = (vx2 - v0) / x2;

				// Choose solution, only one is found within the given time span [0:masterTimeToDest]
				if (x1 > 0 && x1 < t)
				{
					vx = vx1;
					acc = a1;
				}
				else if (x2 > 0 && x2 < t)
				{
					vx = vx2;
					acc = a2;
				}
				else
				{
					// No solution 
					acc = 0;
				}
			}

			if (mode_ == SynchMode::MODE_NON_LINEAR &&
				((submode_ == SynchSubmode::SUBMODE_CONCAVE && acc > 0) ||
				(submode_ == SynchSubmode::SUBMODE_CONVEX && acc < 0)))
			{
				// Reached the apex of the speed profile, switch mode and phase
				mode_ = SynchMode::MODE_LINEAR;
				//PrintStatus("Reached apex");

				// Keep speed for this time step
				acc = 0;
			}

			// Check for case 3, where target speed(vx) < 0
			if (mode_ == SynchMode::MODE_NON_LINEAR && vx < 0)
			{
				// In phase one, decelerate to 0, then stop
				// Calculate time needed to cover distance proportional to current speed / final speed
				double t1 = 2 * v0*s / (v0*v0 + v1 * v1);
				if (fabs(t1) > SMALL_NUMBER)
				{
					acc = -v0 / t1;
				}
				else
				{
					acc = 0;
				}

				if (t1 * v0 / 2 > s / 2)
				{
					// If more than half distance to destination needed, then stop immediatelly
					acc = MAX_DECELERATION;
					mode_ = SynchMode::MODE_STOP_IMMEDIATELY;
					//PrintStatus("Stop immediately");
				}

				if (v0 + acc * dt < 0)
				{
					// Reached to a stop
					object_->SetSpeed(0);
					mode_ = SynchMode::MODE_STOPPED;
					//PrintStatus("Stopped");

					return;
				}
			}
		}
		else
		{
			// No final speed specified. Calculate it based on current speed and available time
			double final_speed = 2 * average_speed - object_->speed_;
			acc = (final_speed - object_->speed_) / masterTimeToDest;
		}

		object_->SetSpeed(object_->GetSpeed() + acc * dt);
	}
}

void VisibilityAction::Start()
{
	OSCAction::Start();
	object_->SetVisibilityMask(
		(graphics_ ? Object::Visibility::GRAPHICS : 0) |
		(traffic_ ? Object::Visibility::TRAFFIC : 0) |
		(sensors_ ? Object::Visibility::SENSORS : 0)
	);
}

void VisibilityAction::Step(double dt, double simTime)
{
	(void)dt;
	(void)simTime;

	OSCAction::Stop();
}

void OverrideControlAction::Start()
{
	for (size_t i = 0; i < overrideActionList.size(); i++)
	{
		object_->overrideActionList[overrideActionList[i].type] = overrideActionList[i];
	}
	OSCAction::Start();
}

void OverrideControlAction::End()
{
	OSCAction::End();
}

void OverrideControlAction::Step(double dt, double simTime)
{
	(void)dt;
	(void)simTime;
}

double OverrideControlAction::RangeCheckAndErrorLog(Object::OverrideType type, double valueCheck, double lowerLimit, double upperLimit, bool ifRound)
{
	double temp = valueCheck;
	if(valueCheck<=upperLimit&&valueCheck>=lowerLimit)
	{	
		if(!ifRound){
			LOG("%d value %.2f is within range.", type, valueCheck);
		}
		else
		{
			valueCheck = round(temp);
			LOG("%d value %.1f is within range and the value is rounded to %.1f.", type, temp,valueCheck);
		}
	}
	else
	{
		valueCheck = (valueCheck>upperLimit)?upperLimit:lowerLimit;
		LOG("%d value is not within range and is modified from %f to %.1f.", type, temp, valueCheck);
	}
	return valueCheck;
}