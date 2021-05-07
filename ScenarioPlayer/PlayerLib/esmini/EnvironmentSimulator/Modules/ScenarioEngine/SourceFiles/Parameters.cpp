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

#include "Parameters.hpp"

using namespace scenarioengine;

void Parameters::addParameterDeclarations(pugi::xml_node xml_node)
{
	parseParameterDeclarations(xml_node, &parameterDeclarations_);
}

void Parameters::parseGlobalParameterDeclarations(pugi::xml_node osc_root_)
{
	parseParameterDeclarations(osc_root_.child("ParameterDeclarations"), &parameterDeclarations_);
	paramDeclarationsSize_ = (int)parameterDeclarations_.Parameter.size();
}

void Parameters::RestoreParameterDeclarations()
{
	parameterDeclarations_.Parameter.erase(
		parameterDeclarations_.Parameter.begin(),
		parameterDeclarations_.Parameter.begin() + parameterDeclarations_.Parameter.size() - paramDeclarationsSize_);
	catalog_param_assignments.clear();
}

int Parameters::setParameter(std::string name, std::string value)
{
	// If string already present in parameterDeclaration
	for (size_t i = 0; i < parameterDeclarations_.Parameter.size(); i++)
	{
		if (PARAMETER_PREFIX + parameterDeclarations_.Parameter[i].name == name || // parameter names should not include prefix
			parameterDeclarations_.Parameter[i].name == name)  // But support also parameter name including prefix
		{
			parameterDeclarations_.Parameter[i].value._string = value;
			return 0;
		}
	}

	return -1;
}

std::string Parameters::getParameter(OSCParameterDeclarations& parameterDeclaration, std::string name)
{
	// If string already present in parameterDeclaration
	for (size_t i = 0; i < parameterDeclaration.Parameter.size(); i++)
	{
		if (PARAMETER_PREFIX + parameterDeclaration.Parameter[i].name == name || // parameter names should not include prefix
			parameterDeclaration.Parameter[i].name == name)  // But support also parameter name including prefix
		{
			return parameterDeclaration.Parameter[i].value._string;
		}
	}
	LOG("Failed to resolve parameter %s", name.c_str());
	throw std::runtime_error("Failed to resolve parameter");
}

OSCParameterDeclarations::ParameterStruct* Parameters::getParameterEntry(std::string name)
{
	// If string already present in parameterDeclaration
	for (size_t i = 0; i < parameterDeclarations_.Parameter.size(); i++)
	{
		if (PARAMETER_PREFIX + parameterDeclarations_.Parameter[i].name == name || // parameter names should not include prefix
			parameterDeclarations_.Parameter[i].name == name)  // But support also parameter name including prefix
		{
			return &parameterDeclarations_.Parameter[i];
		}
	}

	return 0;
}

int Parameters::GetNumberOfParameters()
{
	return (int)parameterDeclarations_.Parameter.size();
}

const char* Parameters::GetParameterName(int index, int* type)
{
	if (index < 0 || index >= parameterDeclarations_.Parameter.size())
	{
		LOG_AND_QUIT("index %d out of range [0:%d]", index, parameterDeclarations_.Parameter.size() - 1);
		return 0;
	}
	
	*type = parameterDeclarations_.Parameter[index].type;

	return parameterDeclarations_.Parameter[index].name.c_str();
}

int Parameters::setParameterValue(std::string name, const void* value)
{
	OSCParameterDeclarations::ParameterStruct* ps = getParameterEntry(name);
	
	if(!ps)
	{ 
		return -1;
	}

	if (ps->type == OSCParameterDeclarations::ParameterType::PARAM_TYPE_INTEGER)
	{
		ps->value._int = *((int*)value);
	}
	else if (ps->type == OSCParameterDeclarations::ParameterType::PARAM_TYPE_DOUBLE)
	{
		ps->value._double = *((double*)value);
	}
	else if (ps->type == OSCParameterDeclarations::ParameterType::PARAM_TYPE_STRING)
	{
		ps->value._string = *((std::string*)value);
	}
	else
	{
		LOG("Unexpected type: %d", ps->type);
		return -1;
	}

	return 0;
}

int Parameters::getParameterValue(std::string name, void* value)
{
	OSCParameterDeclarations::ParameterStruct* ps = getParameterEntry(name);

	if (!ps)
	{
		return -1;
	}

	if (ps->type == OSCParameterDeclarations::ParameterType::PARAM_TYPE_INTEGER)
	{
		*((int*)value) = ps->value._int;
	}
	else if (ps->type == OSCParameterDeclarations::ParameterType::PARAM_TYPE_DOUBLE)
	{
		*((double*)value) = ps->value._double;
	}
	else if (ps->type == OSCParameterDeclarations::ParameterType::PARAM_TYPE_BOOL)
	{
		*((bool*)value) = ps->value._bool;
	}
	else if (ps->type == OSCParameterDeclarations::ParameterType::PARAM_TYPE_STRING)
	{
		*((std::string*)value) = ps->value._string;
	}
	else
	{
		LOG("Unexpected type: %d", ps->type);
		return -1;
	}

	return 0;
}

int Parameters::getParameterValueInt(std::string name, int &value)
{
	OSCParameterDeclarations::ParameterStruct* ps = getParameterEntry(name);

	if (!ps || ps->type != OSCParameterDeclarations::ParameterType::PARAM_TYPE_INTEGER)
	{
		return -1;
	}

	value = ps->value._int;
	
	return 0;
}

int Parameters::getParameterValueDouble(std::string name, double& value)
{
	OSCParameterDeclarations::ParameterStruct* ps = getParameterEntry(name);

	if (!ps || ps->type != OSCParameterDeclarations::ParameterType::PARAM_TYPE_DOUBLE)
	{
		return -1;
	}

	value = ps->value._double;

	return 0;
}

int Parameters::getParameterValueString(std::string name, const char*& value)
{
	OSCParameterDeclarations::ParameterStruct* ps = getParameterEntry(name);

	if (!ps || ps->type != OSCParameterDeclarations::ParameterType::PARAM_TYPE_STRING)
	{
		return -1;
	}

	value = ps->value._string.c_str();

	return 0;
}

int Parameters::getParameterValueBool(std::string name, bool& value)
{
	OSCParameterDeclarations::ParameterStruct* ps = getParameterEntry(name);

	if (!ps || ps->type != OSCParameterDeclarations::ParameterType::PARAM_TYPE_BOOL)
	{
		return -1;
	}

	value = ps->value._bool;

	return 0;
}

std::string Parameters::getParameterValueAsString(std::string name)
{
	OSCParameterDeclarations::ParameterStruct* ps = getParameterEntry(name);

	if (!ps)
	{
		return "";
	}

	if (ps->type == OSCParameterDeclarations::ParameterType::PARAM_TYPE_STRING)
	{
		return ps->value._string;
	}
	else if (ps->type == OSCParameterDeclarations::ParameterType::PARAM_TYPE_INTEGER)
	{
		return std::to_string(ps->value._int);
	}
	else if (ps->type == OSCParameterDeclarations::ParameterType::PARAM_TYPE_DOUBLE)
	{
		return std::to_string(ps->value._double);
	}
	else if (ps->type == OSCParameterDeclarations::ParameterType::PARAM_TYPE_BOOL)
	{
		return ps->value._bool ? "true" : "false";
	}
	else
	{
		return "";
	}
}

int Parameters::setParameterValueByString(std::string name, std::string value)
{
	OSCParameterDeclarations::ParameterStruct* ps = getParameterEntry(name);

	if (!ps)
	{
		return -1;
	}

	if (ps->type == OSCParameterDeclarations::ParameterType::PARAM_TYPE_INTEGER)
	{
		ps->value._int = strtoi(value);
	}
	else if (ps->type == OSCParameterDeclarations::ParameterType::PARAM_TYPE_DOUBLE)
	{
		ps->value._double = strtod(value);
	}
	else if (ps->type == OSCParameterDeclarations::ParameterType::PARAM_TYPE_BOOL)
	{
		ps->value._bool = (value == "true" ? true : false);
	}
	else if (ps->type == OSCParameterDeclarations::ParameterType::PARAM_TYPE_STRING)
	{
		ps->value._string = value;
	}
	else
	{
		LOG("Unexpected type: %d", ps->type);
		return -1;
	}

	return 0;
}

int Parameters::setParameterValue(std::string name, int value)
{
	OSCParameterDeclarations::ParameterStruct* ps = getParameterEntry(name);

	if (!ps || ps->type != OSCParameterDeclarations::ParameterType::PARAM_TYPE_INTEGER)
	{
		return -1;
	}

	ps->value._int = value;

	return 0;
}

int Parameters::setParameterValue(std::string name, double value)
{
	OSCParameterDeclarations::ParameterStruct* ps = getParameterEntry(name);

	if (!ps || ps->type != OSCParameterDeclarations::ParameterType::PARAM_TYPE_DOUBLE)
	{
		return -1;
	}

	ps->value._double = value;

	return 0;
}

int Parameters::setParameterValue(std::string name, const char* value)
{
	OSCParameterDeclarations::ParameterStruct* ps = getParameterEntry(name);

	if (!ps || ps->type != OSCParameterDeclarations::ParameterType::PARAM_TYPE_STRING)
	{
		return -1;
	}

	ps->value._string = value;

	return 0;
}

int Parameters::setParameterValue(std::string name, bool value)
{
	OSCParameterDeclarations::ParameterStruct* ps = getParameterEntry(name);

	if (!ps || ps->type != OSCParameterDeclarations::ParameterType::PARAM_TYPE_BOOL)
	{
		return -1;
	}

	ps->value._bool = value;

	return 0;
}

std::string Parameters::ReadAttribute(pugi::xml_node node, std::string attribute_name, bool required)
{
	if (!strcmp(attribute_name.c_str(), ""))
	{
		if (required)
		{
			LOG("Warning: Empty attribute");
		}
		return "";
	}

	pugi::xml_attribute attr;

	if ((attr = node.attribute(attribute_name.c_str())))
	{
		if (attr.value()[0] == '$')
		{
			// Resolve variable
			return getParameter(parameterDeclarations_, attr.value());
		}
		else
		{
			return attr.value();
		}
	}
	else
	{
		if (required)
		{
			LOG("Warning: missing required attribute: %s -> %s", node.name(), attribute_name.c_str());
		}
	}

	return "";
}

void Parameters::parseParameterDeclarations(pugi::xml_node parameterDeclarationsNode, OSCParameterDeclarations* pd)
{
	for (pugi::xml_node pdChild = parameterDeclarationsNode.first_child(); pdChild; pdChild = pdChild.next_sibling())
	{
		OSCParameterDeclarations::ParameterStruct param = { "", OSCParameterDeclarations::ParameterType::PARAM_TYPE_STRING, {0, 0, ""} };

		param.name = pdChild.attribute("name").value();

		// Check for catalog parameter assignements, overriding default value
		param.value._string = pdChild.attribute("value").value();
		for (size_t i = 0; i < catalog_param_assignments.size(); i++)
		{
			if (param.name == catalog_param_assignments[i].name)
			{
				param.value._string = catalog_param_assignments[i].value._string;
				break;
			}
		}

		std::string type_str = pdChild.attribute("parameterType").value();
		
		if (type_str == "integer" || type_str == "int")
		{
			if (type_str == "int")
			{
				LOG("INFO: int type should renamed into integer - accepting int this time.");
			}
			param.type = OSCParameterDeclarations::ParameterType::PARAM_TYPE_INTEGER;
			param.value._int = strtoi(param.value._string);
		}
		else if (type_str == "double")
		{
			param.type = OSCParameterDeclarations::ParameterType::PARAM_TYPE_DOUBLE;
			param.value._double = strtod(param.value._string);
		}
		else if (type_str == "boolean" || type_str == "bool")
		{
			if (type_str == "bool")
			{
				LOG("INFO: bool type should renamed into boolean - accepting bool this time.");
			}

			param.type = OSCParameterDeclarations::ParameterType::PARAM_TYPE_BOOL;
			param.value._bool = param.value._string == "true" ? true : false;
		}
		else if (type_str == "string")
		{
			param.type = OSCParameterDeclarations::ParameterType::PARAM_TYPE_STRING;
		}
		else if (type_str == "unsignedInt" || type_str == "unsignedShort" || type_str == "dateTime")
		{
			LOG_TRACE("Type %s is not supported yet", type_str.c_str());
		}
		else
		{
			LOG_TRACE_AND_QUIT("Unexpected Type: %s", type_str.c_str());
		}
		pd->Parameter.insert(pd->Parameter.begin(), param);
	}
}


