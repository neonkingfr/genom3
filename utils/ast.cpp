/*
 * Copyright (c) 2009 LAAS/CNRS
 * All rights reserved.
 *
 * Redistribution and use  in source  and binary  forms,  with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   1. Redistributions of  source  code must retain the  above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice,  this list of  conditions and the following disclaimer in
 *      the  documentation  and/or  other   materials provided  with  the
 *      distribution.
 *
 * THIS  SOFTWARE IS PROVIDED BY  THE  COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND  ANY  EXPRESS OR IMPLIED  WARRANTIES,  INCLUDING,  BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES  OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR  PURPOSE ARE DISCLAIMED. IN  NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR      CONTRIBUTORS  BE LIABLE FOR   ANY    DIRECT, INDIRECT,
 * INCIDENTAL,  SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE   OF THIS SOFTWARE, EVEN   IF ADVISED OF   THE POSSIBILITY OF SUCH
 * DAMAGE.
 */
#include "ast.h"

#include <iostream>

#include "dumptypes.h"

using namespace std;
using namespace G3nom;
using namespace Idl;

/******** Component ***************/

Component::Component()
{}

Component::~Component()
{
}


void Component::debug()
{
	DumpType dump(cout);

	cout << "Component: " << name << " v. " << version << endl;
	cout << "\tLanguage (plugin) : " << pluginLanguage << endl;
	cout << "\tIDS Struct: " << endl;
	if (IDSType)
		IDSType->accept(dump);
	cout << endl;

	cout << endl << "Services: " << services.size() << endl;
	Service::Map::const_iterator it;
	for (it = services.begin(); it != services.end(); ++it) {
		cout << "\t" << it->first << " : " << endl;
		it->second->debug();
		cout << endl;
	}

	cout << endl << "Tasks: " << tasks.size() << endl;
	Task::Map::const_iterator it2;
	for (it2 = tasks.begin(); it2 != tasks.end(); ++it2) {
		cout << "\t-- " << it2->first << " : " << endl;
		it2->second->debug();
		cout << endl;
	}

	cout << endl << "Ports: " << ports.size() << endl;
	Port::Map::const_iterator it3;
	for (it3 = ports.begin(); it3 != ports.end(); ++it3) {
		cout << "\t* " << it3->first << " : ";
		it3->second->debug();
		cout << endl;
	}

	cout << endl << "Types: " << types.size() << endl;
	IdlType::Vector::const_iterator it4;
	for (it4 = types.begin(); it4 != types.end(); ++it4) {
		cout << "* ";
		(*it4)->accept(dump);
		cout << endl;
	}
}

void Component::addTask(Task::Ptr task)
{
	/// \todo throw exception ? return code ?
	if (tasks.find(name) != tasks.end())
		cerr << "Warning: already existing task name: " << name << endl;
	tasks[task->name] = task;
}

void Component::addService(Service::Ptr s)
{
	if (services.find(name) != services.end())
		cerr << "Warning: already existing service name: " << name << endl;
	services[s->name] = s;
}

void Component::addPort(Port::Ptr port)
{
	if (ports.find(name) != ports.end())
		cerr << "Warning: already existing port name: " << name << endl;
	ports[port->name] = port;
}

void Component::addType(IdlType::Ptr type)
{
	types.push_back(type);
}

Task* Component::task(const std::string &name)
{
	Task::Map::iterator it = tasks.find(name);
	if (it != tasks.end())
		return it->second.get();
	return 0;
}

Task::Map& Component::tasksMap()
{
	return tasks;
}

std::vector<std::string> Component::tasksList()
{
	vector<string> vec;

	Task::Map::const_iterator it;
	for (it = tasks.begin(); it != tasks.end(); ++it)
		vec.push_back(it->first);

	return vec;
}

IdlType::Ptr Component::typeFromName(const std::string &name)
{
	cout << "Searching type " << name << endl;

	IdlType::Vector::const_iterator it4;
	for (it4 = types.begin(); it4 != types.end(); ++it4) {
		switch ((*it4)->kind()) {
			case IdlType::Struct: {
					StructType *s = static_cast<StructType*>(it4->get());
					if (s->identifier() == name)
						return *it4;
					break;
				}
			case IdlType::Typedef: {
					TypedefType *t = static_cast<TypedefType*>(it4->get());
					if (t->hasIdentifier(name))
						return *it4;
					break;
				}
			default:
				break;
		}
	}
	return IdlType::Ptr();
}

/******** Port ***************/

void Port::debug()
{
	if (type == Incoming)
		cout << "Inport; ";
	else
		cout << "Outport; ";

	cout << "type: " << /*idlType->debug() <<*/ endl;
}

/******** Task ***************/

void Task::debug()
{
	cout << "priority: " << priority << ", stackSize: " << stackSize << ", period: " << period << endl;
	cout << "Codels:" << endl;
	Codel::Map::const_iterator it;
	for (it = codels.begin(); it != codels.end(); ++it) {
		cout << "\t** " << it->first << " : ";
		it->second->debug();
		cout << endl;
	}
}

void Task::addCodel(const std::string& name, G3nom::Codel::Ptr c)
{
	codels.insert(make_pair(name,c));
}

/******** Service ***************/

void Service::debug()
{
	cout << "doc: " << doc << endl;
	cout << "Parent task: " << taskName << endl;
	cout << "Codels:" << endl;
	Codel::Map::const_iterator it;
	for (it = codels.begin(); it != codels.end(); ++it) {
		cout << "\t** " << it->first << " : ";
		it->second->debug();
		cout << endl;
	}
}

void Service::addInput(const std::string &s)
{
	inputs.push_back(s);
}

void Service::addCodel(const std::string &name, Codel::Ptr c)
{
	codels.insert(make_pair(name,c));
}

/******** Codel ***************/

void Codel::debug()
{
	cout << name << "( inports: ";
	for (vector<string>::const_iterator it = inPorts.begin(); it != inPorts.end(); ++it)
		cout << *it << ", ";

	cout << "; outports: ";
	for (vector<string>::const_iterator it = outPorts.begin(); it != outPorts.end(); ++it)
		cout << *it << ", ";

	cout << "; intType: ";
	for (vector<string>::const_iterator it = inTypes.begin(); it != inTypes.end(); ++it)
		cout << *it << ", ";

	cout << "; outTypes: ";
	for (vector<string>::const_iterator it = outTypes.begin(); it != outTypes.end(); ++it)
		cout << *it << ", ";
	cout << ")";
}

void Codel::addInType(const std::string &t)
{
	inTypes.push_back(t);
}

void Codel::addOutType(const std::string &t)
{
	outTypes.push_back(t);
}

void Codel::addInPort(const std::string &t)
{
	inPorts.push_back(t);
}

void Codel::addOutPort(const std::string &t)
{
	outPorts.push_back(t);
}


// kate: indent-mode cstyle; replace-tabs off; tab-width 4;  replace-tabs off;
