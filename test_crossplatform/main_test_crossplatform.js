var extension = require('../dtrace-provider');
console.log('required');

try {
  //var provider = extension.createDTraceProvider("nodeapp");
  var provider = extension.createTraceProvider({provider_name: 'nodeapp', module_name: 'my_module', guid: '5A391F32-A079-42E1-97B1-8A45295AA1FD'});
} catch (ex) {
  console.log(ex);
  process.exit(1);
}
console.log('created provider');

// add probes with descriptor
provider.addProbe('event1', 'char *', 'int');
provider.addProbe('event2', 'char *');
// use auto created descriptor
provider.addProbe('event3', 'int', 'int', 'char *');
provider.addProbe('event4', 'json');
console.log('added probes');

provider.enable();
console.log('enabled probes');

var obj = new Object;
obj.foo = 42;
obj.bar = 'forty-two';

provider.fire('event1', function () { return ['abc', 77]; });
provider.fire('event1', function () { return ['MMMMMMMMM', 513]; });
provider.fire('event2', function () { return ['MMMMMMMMM']; });
provider.fire('event3', function () { return [500, 999, 'xxxxxx yyyyyy']; });
provider.fire('event4', function (p) { return [obj]; });

console.log('fired probes');

process.exit(0);