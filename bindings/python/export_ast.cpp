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

#include <boost/python.hpp>

#include "utils/ast.h"

using namespace G3nom;
using namespace Idl;
using namespace boost::python;

// BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(ServiceAddInputOverloads, Service::addInput, 1,3)
// bool    (X::*fx1)(int)              = &X::f;

void export_ast()
{
	class_<IObjectProperties>("IObjectProperties")
	.def("property", &IObjectProperties::property, return_value_policy<reference_existing_object>())
	.def("addProperty", &IObjectProperties::addProperty);

	class_<Component, bases<IObjectProperties> >("Component")
	.def("name", &Component::name)
	.def_readonly("IDSType", &Component::IDSType)
	.def_readonly("uniqueId", &Component::uniqueId)
	.def_readonly("language", &Component::pluginLanguage)
	.def_readonly("version", &Component::version)
	.def_readonly("email", &Component::email)
	.def("task", &Component::task)
	.def("debug", &Component::debug)
	.def("tasksMap", &Component::tasksMap, return_value_policy<reference_existing_object>())
	.def("taskIndex", &Component::taskIndex)
	.def("service", &Component::service)
	.def("servicesMap", &Component::servicesMap, return_value_policy<reference_existing_object>())
	.def("serviceIndex", &Component::serviceIndex)
	.def("port", &Component::port)
	.def("portsMap", &Component::portsMap, return_value_policy<reference_existing_object>())
	.def("portIndex", &Component::portIndex)
	.def("typesVect", &Component::typesVect, return_value_policy<reference_existing_object>())
	.def("nativeTypesIncludes", &Component::nativeTypesIncludes, return_value_policy<reference_existing_object>())
	.def("valuesMap", &Component::valuesMap, return_value_policy<reference_existing_object>())
	.def("importedComponents", &Component::importedComponents, return_value_policy<reference_existing_object>())
	.def("eventsMap", &Component::eventsMap, return_value_policy<reference_existing_object>())
	.def("eventsForService", &Component::eventsForService)
	.def("typeFromIdsName", &Component::typeFromIdsName);

	class_<Task, Task::Ptr, bases<IObjectProperties> >("Task")
	.def("debug", &Task::debug)
	.def("codel", &Task::codel)
	.def("hasCodel", &Task::hasCodel)
	.def_readonly("name", &Task::name)
	.def_readonly("stackSize", &Task::stackSize)
	.def_readonly("priority", &Task::priority)
	.def_readonly("delay", &Task::delay)
	.def_readonly("period", &Task::period)
	.def("errorMessages", &Task::errorMessages, return_value_policy<reference_existing_object>());

  {
	class_<Service, Service::Ptr, bases<IObjectProperties> > Service_exposer 
		= class_<Service, Service::Ptr, bases<IObjectProperties> >("Service", init<const std::string &>());
	scope Service_scope(Service_exposer);
	enum_<Service::Type>("ServiceType")
	.value("Init", Service::Init)
	.value("Control", Service::Control)
	.value("Exec", Service::Exec)
	.export_values();

	Service_exposer
	.def("debug", &Service::debug)
	.def_readonly("name", &Service::name)
	.def_readonly("doc", &Service::doc)
	.def_readwrite("taskName", &Service::taskName)
	.def_readwrite("type", &Service::type)
	.def("codel", &Service::codel)
	.def("hasCodel", &Service::hasCodel)
	.def("addCodel", &Service::addCodel)
	.def("addInput", &Service::addInput/*, ServiceAddInputOverloads()*/)
	.def_readwrite("output", &Service::output)
	.def("events", &Service::events, return_value_policy<reference_existing_object>())
	.def("codels", &Service::codels, return_value_policy<reference_existing_object>())
	.def("inputs", &Service::inputs, return_value_policy<reference_existing_object>())
	.def("errorMessages", &Service::errorMessages, return_value_policy<reference_existing_object>())
	.def("incompatibleServices", &Service::incompatibleServices, return_value_policy<reference_existing_object>());
  }
  {
	class_<ServiceInput> ServiceInput_exposer = class_<ServiceInput>("ServiceInput");
	scope ServiceInput_scope(ServiceInput_exposer);
	enum_<ServiceInput::Kind>("ServiceInputKind")
	.value("IDSMember", ServiceInput::IDSMember)
	.value("Type", ServiceInput::Type)
	.export_values();

	ServiceInput_exposer
	.def_readwrite("kind", &ServiceInput::kind)
	.def_readwrite("identifier", &ServiceInput::identifier)
	.def_readwrite("type", &ServiceInput::type)
	.def_readwrite("doc", &ServiceInput::doc)
	.def_readwrite("defaultValue", &ServiceInput::defaultValue);
  }

	class_<InputDoc>("InputDoc")
	.def_readonly("doc", &InputDoc::doc)
	.def_readonly("members", &InputDoc::members);

	class_<Codel, Codel::Ptr>("Codel", init<const std::string &>())
	.def_readonly("name", &Codel::name)
	.def_readonly("inTypes", &Codel::inTypes)
	.def_readonly("outTypes", &Codel::outTypes)
	.def_readonly("outPorts", &Codel::outPorts)
	.def_readonly("inPorts", &Codel::inPorts)
	.def("addInPort", &Codel::addInPort)
	.def("addOutPort", &Codel::addOutPort)
	.def("addInType", &Codel::addInType)
	.def("addOutType", &Codel::addOutType);
  {
	class_<Port, Port::Ptr, bases<IObjectProperties> > Port_exposer = class_<Port, Port::Ptr, bases<IObjectProperties> >("Port");
	scope Port_scope(Port_exposer);
	enum_<Port::Type>("PortType")
	.value("Incoming", Port::Incoming)
	.value("Outgoing", Port::Outgoing)
	.export_values();

	Port_exposer
	.def_readonly("name", &Port::name)
	.def_readonly("idlType", &Port::idlType)
	.def_readonly("sizeCodel", &Port::sizeCodel)
	.def_readonly("type", &Port::type);
  }
  {
	class_<Event, Event::Ptr, boost::noncopyable> Event_exposer = class_<Event, Event::Ptr, boost::noncopyable>("Event", no_init);
	scope Event_scope(Event_exposer);
	enum_<Event::Kind>("EventKind")
	.value("NamedEv", Event::NamedEv)
	.value("ServiceEv", Event::ServiceEv)
	.value("PortEv", Event::PortEv)
	.export_values();

	Event_exposer
	.def("identifier", &Event::identifier)
	.def("kind", &Event::kind)
	.def("asPortEvent", &Event::asPortEvent, return_value_policy<reference_existing_object>())
	.def("asNamedEvent", &Event::asNamedEvent, return_value_policy<reference_existing_object>())
	.def("asServiceEvent", &Event::asServiceEvent, return_value_policy<reference_existing_object>());
  }
	class_<NamedEvent, bases<Event> >("NamedEvent", init<const std::string &, Event::Ptr>())
	.def("aliasEvent", &NamedEvent::aliasEvent);
  {
	class_<PortEvent, bases<Event> > PortEvent_exposer = class_<PortEvent, bases<Event> >("PortEvent", init<const std::string &, PortEvent::Kind>());
	scope PortEvent_scope(PortEvent_exposer);
	enum_<PortEvent::Kind>("PortEventKind")
	.value("OnUpdate", PortEvent::OnUpdate)
	.value("OnRead", PortEvent::OnRead)
	.value("OnWrite", PortEvent::OnWrite)
	.value("OnInitialize", PortEvent::OnInitialize)
	.export_values();

	PortEvent_exposer
	.def("portName", &PortEvent::portName)
	.def("kindAsString", &PortEvent::kindAsString);
  }
  {
	class_<ServiceEvent, bases<Event> > ServiceEvent_exposer 
	  = class_<ServiceEvent, bases<Event> >("ServiceEvent", init<const std::string &, ServiceEvent::Kind>());
	scope ServiceEvent_scope(ServiceEvent_exposer);
	enum_<ServiceEvent::Kind>("ServiceEventKind")
	.value("OnCalled", ServiceEvent::OnCalled)
	.value("OnStart", ServiceEvent::OnStart)
	.value("OnEnd", ServiceEvent::OnEnd)
	.value("OnInter", ServiceEvent::OnInter)
	.value("OnCodel", ServiceEvent::OnCodel)
	.export_values();

	ServiceEvent_exposer
	.def("serviceName", &ServiceEvent::serviceName)
	.def("kindAsString", &ServiceEvent::kindAsString);
  }
}
