# -*- coding: utf-8 -*-
# This file groups some functions used by multiple templates

def isDynamic(t):
  if t.kind() == IdlKind.Sequence:
    return True
  elif t.kind() == IdlKind.Named or t.kind() == IdlKind.Typedef:
    return isDynamic(t.unalias())
  elif t.kind() == IdlKind.Struct:
    s = t.asStructType()
    for m in s.members():
      if isDynamic(m.data()):
	return True
    return False
  else:
    return False

def isDynamicPort(port):
  return isDynamic(port.idlType)

def dynamicMembers(t, name, recursive = False):
  if t.kind() == IdlKind.Sequence:
    if recursive:
      s = t.asSequenceType()
      l = [(t, name)]
      l.extend(dynamicMembers(s.seqType(), name + ".data", True))
      return l
    else:
      return [(t, name)]
  elif t.kind() == IdlKind.Named or t.kind() == IdlKind.Typedef:
    return dynamicMembers(t.unalias(), name, recursive)
  elif t.kind() == IdlKind.Struct:
    s = t.asStructType()
    l = []
    for m in s.members():
      l.extend(dynamicMembers(m.data(), name + "." + m.key(), recursive))
    return l
  else:
    return []

def toIdentifier(n):
  res = n.replace(".", "_")
  res = res.replace("[", "")
  res = res.replace("]", "")
  res = res.replace("(*", "")
  res = res.replace(")", "")
  res = res.replace("->", "")
  return res

def counterName(n):
  return toIdentifier(n) + "_counter"

def lengthVar(n):
  return toIdentifier(n) + "_length"

def isEmptyVar(n):
  return toIdentifier(n) + "_is_empty"


# try to find an init service
def findInitService():
  i=-1
  for s in comp.servicesMap():
    i += 1
    if s.data().type == ServiceType.Init:
      return s.data(), i
  return 0,-1

initService,initServiceNb = findInitService()

# returns a flat list of the structure of a type
def flatStruct(t, name, separator = "_"):
    if t.kind() == IdlKind.Named:
	n = t.asNamedType()
	return flatStruct(n.type(), name, separator)
    elif t.kind() == IdlKind.Struct:
	s = t.asStructType()
	l = []
	for m in s.members():
	    l.extend(flatStruct(m.data(), name + separator + m.key(), separator))
	return l
    else:
	return [(t, name)]  

def inputList(service):
  res = []
  for i in service.inputs():
    res.extend(flatStruct(inputType(i), i.identifier, '.'))
  return res

def inputType(i):
  if i.kind == ServiceInputKind.IDSMember:
    return comp.typeFromIdsName(i.identifier)
  else:
    return i.type

# codel related functions
def pointerTo(t):
  s = MapTypeToC(t,True)
  if t.kind() == IdlKind.String:
    return s
  else:
    return s+"*"

def addressOf(t, s):
 if t.kind() == IdlKind.String:
   return s
 else:
    return "&" + s

def real_codel_signature(codel, service=None):
  proto = ""
  if service is not None:
    for s in service.inputs():
	idstype = inputType(s);
	proto += "const " + pointerTo(idstype) + " in_" + s.identifier + ", ";
    if service.output.identifier:
	idstype = inputType(service.output);
	proto += pointerTo(idstype) + " out_" + service.output.identifier + ", "; 

  for type in codel.inTypes:
    idstype = comp.typeFromIdsName(type);
    proto += "const " + pointerTo(idstype) + " in_" + type + ", ";
  for type in codel.outTypes:
    idstype = comp.typeFromIdsName(type);
    proto += pointerTo(idstype) + " out_" + type + ", ";
  for port in codel.outPorts:
    p = comp.port(port)
    if p is not None:
      proto += pointerTo(p.idlType) + " outport_" + port + ", "; 
    else:
      proto += port + ", "
  for port in codel.inPorts:
    p = comp.port(port)
    if p is not None:
	proto += pointerTo(p.idlType) + " inport_" + port + ", "; 
    else:
	proto += port + ", "
  proto = codel.name + "(" + proto[:-2] + ")"
  return proto

def sizeCodelSignature(port):
  sizeCodelArgs = ""
  for x in dynamicMembers(port.idlType, port.name + "_outport", True):
    sizeCodelArgs += "int *" + lengthVar(x[1]) + ", "

  for s in port.sizeCodel.inTypes:
    idstype = comp.typeFromIdsName(s);
    sizeCodelArgs += pointerTo(idstype) + " in_" + s + ", "
  for s in port.sizeCodel.outTypes:
    idstype = comp.typeFromIdsName(s);
    sizeCodelArgs += pointerTo(idstype) + " out_" + s + ", "
  for s in port.sizeCodel.inPorts:
    p = comp.port(s)
    sizeCodelArgs += pointerTo(port.idlType) + " " + s + "_inport, "
  for s in port.sizeCodel.outPorts:
    p = comp.port(s)
    sizeCodelArgs += pointerTo(port.idlType) + " " + s + "_outport, "

  return "int " + port.sizeCodel.name + "(" + sizeCodelArgs[:-2] + ")"


# create a list of outports and inports
outports = []
inports = []
for p in comp.portsMap():
    if p.data().type == PortType.Outgoing:
	outports.append(p.data())
    else:
	inports.append(p.data())

# error related functions
def createErrorList():
  l = []
  for s in comp.servicesMap():
    service = s.data()
    for e in service.errorMessages():
	l.append(e)
  for t in comp.tasksMap():
    for e in t.data().errorMessages():
	l.append(e)
  return set(l)
errorList = createErrorList();

