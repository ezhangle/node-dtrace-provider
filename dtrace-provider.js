var TraceProviderCreatorFunction;
var platform = process.platform;

function TraceProviderStub() { }
TraceProviderStub.prototype.addProbe = function() {
    return {
        'fire': function() { }
    };
};

TraceProviderStub.prototype.removeProbe = function () { };
TraceProviderStub.prototype.enable = function () { };
TraceProviderStub.prototype.disable = function () { };
TraceProviderStub.prototype.fire = function () { };

var builds = ['Release', 'default', 'Debug'];

for (var i in builds) {
	try {
		var binding = require('./build/' + builds[i] + '/TraceProviderBindings');
		TraceProviderCreatorFunction = binding.createTraceProvider;
		break;
	} catch (e) {
		//If the platform looks like it _should_ support the extension,
		//log a failure to load the bindings.
		if (process.platform == 'darwin' ||
			process.platform == 'solaris' ||
			process.platform == 'freebsd' || 
			process.platform == 'win32') {
			console.log(e);
		}
	}
}

if (!TraceProviderCreatorFunction) {
	TraceProviderCreatorFunction = TraceProviderStub;
} 

//Expose the universal function for provider creation.
exports.TraceProvider = TraceProviderCreatorFunction;
exports.createTraceProvider = function (object) {
	return new TraceProviderCreatorFunction(object);
};

//If we are not on win32, we must also expose the dtrace-only creator function to support existing code as well as namesFromGuid.
if (platform != 'win32') {
	exports.createDTraceProvider = function (name, module) {
      if (arguments.length == 2)
          return (new TraceProviderCreatorFunction(name, module));
      return (new TraceProviderCreatorFunction(name));
	};
	exports.namesFromGuid = binding.namesFromGuid;
} else { //Otherwise, expose the guidFromNames function to see what guid will be generated for the ETW provider if the user didn't specify it.
	exports.guidFromNames = binding.guidFromNames;
}
