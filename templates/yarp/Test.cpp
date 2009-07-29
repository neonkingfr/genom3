<?
def parse_input(type, name):
  if type.kind() == IdlType.Enum:?>
  {
      int tmp;
      cin >> tmp;
      <!name!> = (<!MapTypeToCpp(type, True)!>) tmp;
  }
<?
  else: ?>
      cin >> <!name!>;
<?
?>
#include <iostream>
#include <yarp/os/all.h>
#include "<!comp.name()!>Struct.h"
#include "lib/RepliesReader.hpp"
#include "lib/RqstWriter.hpp"
#include "lib/DataServer.hpp"
#include "lib/Events.hpp"

using namespace std;
using namespace yarp::os;
using namespace GenomYarp;

class TestEventReader : public TypedReaderCallback<Bottle>
{
    virtual void onRead(Bottle& b)
    {
	cout << "Received event " << b.get(0).asString().c_str() << endl;
    }
};

<?
for t in tasksMap:
  task = t.data()
  ?>
class <!task.name!>ReplyReader : public TypedReaderCallback<Bottle>
{
    virtual void onRead(Bottle& b)
    {
	string req = ReplyReader::readRequestName(&b);
<?
  for s in servicesMap:
    service = s.data()
    if service.type == Service.Control or service.taskName != task.name:
      continue
    serviceInfo = services_info_dict[service.name]
    if not serviceInfo.outputFlag:
      continue
    ?>
	if(req == "<!service.name!>") {
	    ReplyAnswer<<!serviceInfo.outputTypeCpp!>> answer(&b);
	    cout << "Received reply: " << answer << endl;
	    return;
	}
<?
  ?>
	ReplyAnswer<VoidIO> answer(&b);
	cout << "Received reply: " << answer << endl;
    }
};
<?
?>

class <!comp.name()!>Test {

public:

<?
for t in tasksMap:
  task = t.data()
  ?>
  yarp::os::BufferedPort<yarp::os::Bottle> <!task.name!>_req_port;
  yarp::os::BufferedPort<yarp::os::Bottle> <!task.name!>_reply_port;
  <!task.name!>ReplyReader <!task.name!>Reader;

  yarp::os::BufferedPort<yarp::os::Bottle> events_inport;
  yarp::os::BufferedPort<yarp::os::Bottle> events_outport;
  TestEventReader testReader;
<?
?>
  yarp::os::BufferedPort<yarp::os::Bottle> Control_req_port;
  yarp::os::BufferedPort<yarp::os::Bottle> Control_reply_port;
<?
for port in outports:
  typeName = MapTypeToCpp(port.idlType)
  ?>
  DataServer<<!typeName!>> <!port.name!>_inport;
<?
for port in inports:
  typeName = MapTypeToCpp(port.idlType)
  ?>
  OutPort<<!typeName!>> <!port.name!>_outport;
<?
?>

<!comp.name()!>Test()
{
    Control_req_port.open("/<!comp.name()!>/Test/Services/Control");
    Network::connect("/<!comp.name()!>/Test/Services/Control", "/<!comp.name()!>/Services/Control");
    Control_reply_port.open("/<!comp.name()!>/Test/<!comp.name()!>/Services/Replies/Control");
//     Network::connect("/<!comp.name()!>/Services/Replies/Control", "/<!comp.name()!>/Test/Services/Replies/Control");
<?
for t in tasksMap:
  task = t.data()
  ?>
    <!task.name!>_req_port.open("/<!comp.name()!>/Test/Services/<!task.name!>");
    Network::connect("/<!comp.name()!>/Test/Services/<!task.name!>", "/<!comp.name()!>/Services/<!task.name!>");
    <!task.name!>_reply_port.open("/<!comp.name()!>/Test/<!comp.name()!>/Services/Replies/<!task.name!>");
//     Network::connect("/<!comp.name()!>/Services/Replies/<!task.name!>", "/<!comp.name()!>/Test/Services/Replies/<!task.name!>");
    <!task.name!>_reply_port.useCallback(<!task.name!>Reader);
<?
for port in outports: ?>
    <!port.name!>_inport.open("/<!comp.name()!>/Test/InPorts/<!port.name!>");
    Network::connect("/<!comp.name()!>/OutPorts/<!port.name!>", "/<!comp.name()!>/Test/InPorts/<!port.name!>");
<?
for port in inports: ?>
    <!port.name!>_outport.open("/<!comp.name()!>/Test/OutPorts/<!port.name!>");
    Network::connect("/<!comp.name()!>/Test/OutPorts/<!port.name!>", "/<!comp.name()!>/InPorts/<!port.name!>");
<?
?>

    events_outport.open("/<!comp.name()!>/Test/Events/Out");
    Network::connect("/<!comp.name()!>/Test/Events/Out", "/<!comp.name()!>/Events/In");

    events_inport.open("/<!comp.name()!>/Test/Events/In");
    Network::connect("/<!comp.name()!>/Events/Out", "/<!comp.name()!>/Test/Events/In");
    events_inport.useCallback(testReader);
}

void printUsage()
{
  // print usage
  cout << "Available actions:" << endl;
  cout << "  (0) Exit" << endl;
  
<?
idx = 10
for s in servicesMap:
  service = s.data()
  idx += 1
  print "  cout << \"  (" + str(idx) + ") " + service.name + "\" << endl;"

print "  cout << \"---------------------------\" << endl;"
for port in outports:
  idx += 1
  print "  cout << \"  (" + str(idx) + ") Show " + port.name + "\" << endl;"
print "  cout << \"---------------------------\" << endl;"
for port in inports:
  idx += 1
  print "  cout << \"  (" + str(idx) + ") Update " + port.name + "\" << endl;"
print "  cout << \"---------------------------\" << endl;"
for e in comp.eventsMap():
  ev = e.data()
  if not ev.asNamedEvent().aliasEvent() is None:
    continue
  idx += 1
  print "  cout << \"  (" + str(idx) + ") Send " + ev.identifier() + "\" << endl;"
?>

}

<?
for s in servicesMap:
  service = s.data()
  serviceInfo = services_info_dict[service.name]
  idx += 1
  
  serviceArgs = "input"
  if serviceInfo.inputFlag:
    serviceArgs = serviceInfo.inputName 
  ?>
void run<!service.name!>()
{
    static int rqst_id = 0;
<?
  if not serviceInfo.inputFlag: ?>
      VoidIO input; // fake input object for the rqst writer
<?
  else: ?>
    <!serviceInfo.inputTypeCpp!> <!serviceInfo.inputName!>;
<?
  if service.output.identifier and service.type == Service.Control: ?>
    <!serviceInfo.outputTypeCpp!> <!serviceInfo.outputName!>;
<?
  if len(service.inputs()) > 1:
    prefix = serviceInfo.inputName + "."
  else:
    prefix = ""

  for i in service.inputs():
    flat = flat_struct(input_type(i), prefix + i.identifier, ".", i.defaultValue, i.doc)
    if i.defaultValue.isEmpty() or i.defaultValue.kind() == Literal.Struct:
      defValue = ""
    else:
      defValue = "(default : \" << " + i.defaultValue.toString() + " << \")"
    ?>
     cout << "Enter <!i.identifier!> (<!i.doc.doc!>) <!defValue!>:" << endl;
<?
    for x in flat:
      t = MapTypeToCpp(x[0], True)
      if len(flat) > 1: 
	if x[2] is None or x[2].isEmpty():
	  value = ""
	else:
	  value = "(default : \" << " + x[2].toString() + " << \")"

	if not x[3] is None:
	  docStr = ": " + x[3].doc
	else:
	  docStr = ""
	?>
      cout << "Enter <!t!> <!x[1]!> <!docStr!> <!value!>:  " << endl;
<?
    parse_input(x[0], x[1]);

  if service.type == Service.Control:
    ?>
      RqstWriter<<!serviceInfo.requestType!>>::send(Control_req_port, "/<!comp.name()!>/Test", rqst_id++, "<!service.name!>", <!serviceArgs!>);
      while(!Control_reply_port.getInputCount()) // wait for the connection to be made
	usleep(50);

      ReplyAnswer<<!serviceInfo.replyType!>> answer(Control_reply_port.read());
      cout << "Final reply: " << answer;
<?
  else:?>
      RqstWriter<<!serviceInfo.requestType!>>::send(<!service.taskName!>_req_port, "/<!comp.name()!>/Test", rqst_id++, "<!service.name!>", <!serviceArgs!>);
//       ReplyAnswer<<!serviceInfo.replyType!>> answer(<!service.taskName!>_reply_port.read());
//       cout << "Intermediate reply: " << answer;

//       cout << "Wait final reply ?";
//       string res;
//       cin >> res;
//       if(res == "y") {
// 	  ReplyAnswer<<!serviceInfo.replyType!>> answer(<!service.taskName!>_reply_port.read());
// 	  cout << "Final reply: " << answer; 
//       }
<?
  ?>
}

<?
for port in outports:
  typeName = MapTypeToCpp(port.idlType)
  ?>
void read<!port.name!>()
{
  <!typeName!> res;
  <!port.name!>_inport.getLatestData(res);
  cout << endl << "<!port.name!> :" << endl;
  YarpCodec<<!typeName!>>::print(res);
  cout << endl;
}
<?
for port in inports:
  typeName = MapTypeToCpp(port.idlType)
  if is_dynamic(port.idlType): ?>
void write<!port.name!>()
{
    cout << "Sorry, port contains dynamic data, cannot modify it" << endl;
}
<?
  else: 
    ?>
void write<!port.name!>()
{
  <!typeName!> *v = new <!typeName!>();
<?
    flatList = flat_struct(port.idlType, "(*v)", ".")
    for x in flatList:
      t = MapTypeToCpp(x[0], True)
      ?>
      cout << "Enter <!t!> <!x[1]!>:  " << endl;
<?
      parse_input(x[0], x[1]);
    ?>

  <!port.name!>_outport.exportData(v);
  cout << endl;
}
<?
?>

<?
for e in comp.eventsMap():
  ev = e.data()
  if not ev.asNamedEvent().aliasEvent() is None:
    continue
  ?>
void send<!ev.identifier()!>()
{
  Bottle &b = events_outport.prepare();
  b.clear();
  b.addString("<!ev.identifier()!>");

  events_outport.writeStrict();
}
<?
?>

void executeAction(int action)
{
  switch(action) {
    case 0:
	exit(0);

<?
idx = 10
for s in servicesMap:
  service = s.data()
  idx += 1
  ?>
    case <!idx!>: {
      run<!service.name!>();
      break;
    }

<?
for port in outports:
  idx += 1
  ?>
    case <!idx!>: {
      read<!port.name!>();
      break;
    }
<?
for port in inports:
  idx += 1
  ?>
      case <!idx!>: {
      write<!port.name!>();
      break;
    }
<?
?>      

<?
for e in comp.eventsMap():
  ev = e.data()
  if not ev.asNamedEvent().aliasEvent() is None:
    continue
  idx += 1
  ?>
      case <!idx!>: {
      send<!ev.identifier()!>();
      break;
    }
<?
?>   
  }
}

};

int main()
{
    yarp::os::Network yarp;

//     Contact c = yarp.queryName("<!comp.name()!>");
//     if(!c.isValid()) {
//       cout << "Impossible to contact module." << endl;
//       return 1;
//     }

    <!comp.name()!>Test test;
    while(true) {
	cout << endl;
	test.printUsage();
	int action;  
	std::cin >> action;
	test.executeAction(action);
    }
}



