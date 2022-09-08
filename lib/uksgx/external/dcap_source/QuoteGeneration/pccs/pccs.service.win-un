var Service = require('node-windows').Service;

// Create a new service object
var svc = new Service({
  name:'Intel(R) SGX PCK Certificate Caching Service',
  script: require('path').join(__dirname,'pccs_server.js')
});

// Listen for the "uninstall" event so we know when it's done.
svc.on('uninstall',function(){
  console.log('Uninstall Intel(R) SGX PCK Certificate Caching Service complete.');
  console.log('The service exists: ',svc.exists);
});

// Uninstall the service.
svc.uninstall();
