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

/*
 * This controller simulates a bad or dizzy driver by manipulating 
 * the speed and lateral offset in a random way.
 * The purpose is purely to demonstrate how to implement a controller.
 */

#include "ControllerSloppyDriver.hpp"
#include "CommonMini.hpp"
#include "Entities.hpp"
#include "ScenarioGateway.hpp"

#include <random>

using namespace scenarioengine;

static std::mt19937 mt_rand;

#define WHEEL_RADIUS 0.35

double SinusoidalTransition::GetValue()
{
	return start_ + amplitude_ * (offset_ + cos(startAngle_ + factor_ * M_PI));
}

double SinusoidalTransition::GetHeading()
{
	return -amplitude_ * sin(startAngle_ + factor_ * M_PI);
}

Controller* scenarioengine::InstantiateControllerSloppyDriver(void* args)
{
	Controller::InitArgs* initArgs = (Controller::InitArgs*)args;

	return new ControllerSloppyDriver(initArgs);
}

ControllerSloppyDriver::ControllerSloppyDriver(InitArgs* args) : sloppiness_(0.5), time_(0), 
	Controller(args)
{
	if (args->properties->ValueExists("sloppiness"))
	{
		sloppiness_ = strtod(args->properties->GetValueStr("sloppiness"));
	}
}

void ControllerSloppyDriver::Init()
{
	mt_rand.seed((unsigned int)time(0));

	Controller::Init();
}

void ControllerSloppyDriver::Step(double timeStep)
{
	if (object_ == 0)
	{
		return;
	}

	time_ += timeStep;

	if (object_->CheckDirtyBits(Object::DirtyBit::SPEED))
	{
		// Speed has been updated by Default Driver, update our reference speed
		referenceSpeed_ = object_->GetSpeed();
	}
	currentSpeed_ = object_->GetSpeed();

	// Do modification to a local position object and then report to gateway
	if (object_ && domain_ & Controller::Domain::CTRL_LONGITUDINAL)
	{
		if (speedTimer_.Expired(time_))
		{
			// restart timer - 50% variation
			double timerValue = speedTimerAverage_ * (1.0 + (1.0 * mt_rand() / mt_rand.max() - 0.5));
			speedTimer_.Start(time_, timerValue);

			// target speed +/- 35%	
			initSpeed_ = referenceSpeed_ * targetFactor_;
			targetFactor_ = 1 + 0.7*sloppiness_ * MIN(sloppiness_, 1.0) * (1.0 * mt_rand() / mt_rand.max() - 0.5);
		}

		double steplen = 0;
		double weight = speedTimer_.Elapsed(time_) / speedTimer_.GetDuration();		

		currentSpeed_ = initSpeed_ * (1-weight) + targetFactor_ * referenceSpeed_ * weight;
		currentSpeed_ = MAX(currentSpeed_, 0);

		if (mode_ == Mode::MODE_OVERRIDE)
		{
			steplen = currentSpeed_ * timeStep;
			object_->SetSpeed(currentSpeed_);
		}
		else
		{
			if (object_->CheckDirtyBits(Object::DirtyBit::SPEED))
			{
				// Default Driver has moved by reference speed
				steplen = (currentSpeed_ - referenceSpeed_) * timeStep;
			}
			else
			{
				// Default Driver has moved by old_speed
				steplen = (currentSpeed_ - object_->GetSpeed()) * timeStep;
			}
		}

		// Adjustment movement to heading and road direction
		if (GetAbsAngleDifference(object_->pos_.GetH(), object_->pos_.GetDrivingDirection()) > M_PI_2)
		{
			// If pointing in other direction
			steplen *= -1;
		}
		
		if (fabs(object_->GetSpeed()) > SMALL_NUMBER)
		{
			object_->pos_.MoveAlongS(steplen, 0);
		}
	}

	if (object_ && domain_ & Controller::Domain::CTRL_LATERAL)
	{
		if (lateralTimer_.Expired(time_))
		{
			// max lateral displacement is about half lane width (7/2) 
			tFuzz0 = tFuzzTarget;
			tFuzzTarget = 2 * sloppiness_ * MIN(sloppiness_, 1.0) * (1.0 * mt_rand() / mt_rand.max() - 0.5);

			// restart timer - 50% variation
			double timerValue = lateralTimerAverage_ * (1.0 + (1.0 * mt_rand() / mt_rand.max() - 0.5));
			lateralTimer_.Start(time_, timerValue);
		}
		double h_error;
		if (mode_ == Mode::MODE_OVERRIDE)
		{
			h_error = object_->pos_.GetHRelative();
		}
		else
		{
			h_error = currentH_;
		}
		
		// Normalize h_error to [-PI, PI]
		h_error = h_error > M_PI ? h_error -= 2 * M_PI : h_error;

		double tFuzz = tFuzz0 + (tFuzzTarget - tFuzz0) * lateralTimer_.Elapsed(time_) / lateralTimer_.duration_;
		double lat_error = lat_error = currentT_ + tFuzz;
		
		// Adjust lane offset for driving direction and tweak these to tune performance
		double lat_constant = -0.02;
		double h_constant = -0.05;
		double dh = (lat_constant * lat_error + h_constant * h_error) * timeStep * object_->GetSpeed();
		double dt = object_->GetSpeed() * timeStep * sin(currentH_ + dh);

		// Move car according to speed and heading
		if (object_->GetSpeed() > SMALL_NUMBER)  // Use old speed set by Default Controller to decide whether heading should be updated
		{
			object_->pos_.SetTrackPos(object_->pos_.GetTrackId(), object_->pos_.GetS(), object_->pos_.GetT() + dt);
			if (mode_ == Mode::MODE_OVERRIDE)
			{
				object_->pos_.SetHeadingRelative(currentH_ + dh);
			}
			else
			{
				object_->pos_.SetHeadingRelative(object_->pos_.GetHRelative() + currentH_ + dh);
			}
			currentT_ += dt;
			currentH_ += dh;
		}
	}

	gateway_->reportObject(object_->id_, object_->name_, static_cast<int>(object_->type_), object_->category_, object_->model_id_,
		object_->GetActivatedControllerType(), object_->boundingbox_, 0, currentSpeed_, object_->wheel_angle_, object_->wheel_rot_, &object_->pos_);

	Controller::Step(timeStep);
}

void ControllerSloppyDriver::Activate(int domainMask)
{
	if (object_)
	{
		if (sloppiness_ < 0 || sloppiness_ > 1)
		{
			LOG("Warning, sloppiness is %.2f recommended range is [0:1]", sloppiness_);
		}
		speedTimerAverage_ = 3;
		speedTimer_.Start(0, speedTimerAverage_);
		targetFactor_ = 1;
		currentSpeed_ = initSpeed_ = referenceSpeed_ = object_->GetSpeed();

		lateralTimerAverage_ = 1.5;
		lateralTimer_.Start(0, lateralTimerAverage_);
		tFuzzTarget = 0;
		tFuzz0 = 0;
		currentT_ = 0;
		currentH_ = object_->pos_.GetHRelative();
	}

	Controller::Activate(domainMask);
}

void ControllerSloppyDriver::ReportKeyEvent(int key, bool down)
{
}