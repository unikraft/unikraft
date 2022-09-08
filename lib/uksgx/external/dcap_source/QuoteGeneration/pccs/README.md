## Provisioning Certificate Caching Service (PCCS)

This is a lightweight Provisioning Certificate Caching Service implemented in nodejs for reference. It retrieves PCK Certificates and other collaterals on-demand using the internet at runtime, and then caches them in local database. The PCCS exposes similar HTTPS interfaces as Intel's Provisioning Certificate Service.

## <h3>How to install</h3>

- **Prerequisites**

  Install node.js (Version <ins>12.22</ins> or later)

  - For Debian and Ubuntu based distributions, you can use the following command:<br/>
    $ curl -sL https://deb.nodesource.com/setup_16.x | sudo -E bash - <br/>
    $ sudo apt-get install -y nodejs
  - To download and install, goto https://nodejs.org/en/download/

  Install dependencies (python3, cracklib-runtime) if they are not installed. Also make sure "python" is linked to "python3"
    $ sudo apt-get install python3 cracklib-runtime

- **Install via Linux Debian package installer**

  $ dpkg -i sgx-dcap-pccs*${version}-${os}*\${arch}.deb

  All configurations can be set during the installation process.

  _**NOTE** : If you have installed old libsgx-dcap-pccs releases with root privilege before, some folders may remain even after you uninstall it.
  You can delete them manually with root privilege, for example, ~/.npm/, etc._

- **Install via RPM package installer**

  $ rpm -ivh sgx-dcap-pccs*\${version}*\${arch}.rpm

  After the RPM package was installed, goto the root directory of the PCCS(/opt/intel/sgx-dcap-pccs/) and run install.sh with account 'pccs':
  $ sudo -u pccs ./install.sh

- **Linux manual installation**

  1. Put all the files and sub folders in this directory to your preferred place with right permissions set to launch a
     web service.
  2. Install dependencies if they are not already installed. Also make sure "python" is linked to "python3"
     nodejs build-essential(gcc gcc-c++ make) python3 cracklib-runtime
  3. Goto ../../tools/PCKCertSelection/ and build libPCKCertSelection.so, copy it to ./lib/
  4. From the root directory of your installation folder, run ./install.sh

- **Windows manual installation**

  1. Put all the files and sub folders in this directory to your preferred place with right permissions set to launch a
     web service.
  2. (Optional) If the target machine connects to internet through a proxy server, configure proxy server first
     before continuing. <br/>
     npm config set http-proxy http://your-proxy-server:port <br/>
     npm config set https-proxy http://your-proxy-server:port <br/>
     npm config set proxy http://your-proxy-server:port

  3. Update config file based on your environment, see section [Configuration file](#Configuration)

  4. Private key and public certificate
     The PCCS requires a private key and certificate pair to run as HTTPS server. For production environment
     you should use formally issued key and certificate. Please put the key files in ssl_key sub directory.
     You can also genarate an insecure key and certificate pair with following commands: (only for debug purpose)
     openssl genrsa -out private.pem 2048
     openssl req -new -key private.pem -out csr.pem
     openssl x509 -req -days 365 -in csr.pem -signkey private.pem -out file.crt

  5. Install python if it's not already installed

  6. From the root directory of your installation folder, run install.bat with administrator privilege
     This will install the required npm packages and install the Window service.

  7. PCKCertSelection Library
     You need to compile the PCKCertSelection library in ../../tools/PCKCertSelection, then put the binary files
     (PCKCertSelectionLib.dll and libcrypto-1_1-x64.dll from openSSL) in a folder that is in OS's search path,
     for example, %SYSTEMROOT%\system32.

  **NOTE** : If self-signed insecure key and certificate are used, you need to set USE_SECURE_CERT=FALSE when
  configuring the default QPL library (see ../qpl/README.md)

## <h3 id="Configuration">Configuration file (config/default.json) </h3>

- **HTTPS_PORT** - The port you want the PCCS to listen on. The default listening port is 8081.
- **hosts** - The hosts that will be accepted for connections. Default is localhost only. To accept all connections use 0.0.0.0
- **uri** - The URL of Intel Provisioning Certificate Service. The default URL is https://api.trustedservices.intel.com/sgx/certification/v3/
- **ApiKey** - The PCCS uses this API key to request collaterals from Intel's Provisioning Certificate Service. User needs to subscribe first to obtain an API key. For how to subscribe to Intel Provisioning Certificate Service and receive an API key, goto https://api.portal.trustedservices.intel.com/provisioning-certification and click on 'Subscribe'.
- **proxy** - Specify the proxy server for internet connection, for example, "http://192.168.1.1:80". Leave blank for no proxy or system proxy.
- **RefreshSchedule** - cron-style refresh schedule for the PCCS to refresh cached artifacts including CRL/TCB Info/QE Identity/QVE Identity.
  The default setting is "0 0 1 \* \* \*", which means refresh at 1:00 AM every day.
- **UserTokenHash** - Sha512 hash of the user token for the PCCS client user to register a platform. For example, PCK Cert ID retrieval tool will use the user token to send platform information to PCCS.
- **AdminTokenHash** - Sha512 hash of the administrator token for the PCCS administrator to perform a manual refresh of cached artifacts.

  _NOTE_ : For Windows you need to set the UserTokenHash and AdminTokenHash manually. You can calculate SHA512 hash with the help of openssl:

      <nul: set /p password="mytoken" | openssl dgst -sha512

- **CachingFillMode** - The method used to fill the cache DB. Can be one of the following: LAZY/REQ/OFFLINE. For more details see section [Caching Fill Mode](#CachingMode).
- **LogLevel** - Log level. Use the same levels as npm: error, warn, info, http, verbose, debug, silly. Default is info.
- **DB_CONFIG** - You can choose sqlite or mysql and many other DBMSes. For sqlite, you don't need to change anything. For other DBMSes, you need to set database connection options correctly. Normally you need to change database, username, password, host and dialect to connect to your DBMS.
  <br/>**NOTE: It's recommended to delete the cache database first if you have installed a version older than 1.9 because the database is not compatible.**

## <h3 id="CachingMode">Caching Fill Mode</h3>

When a new server platform is introduced to the data center or the cloud service provider that will require SGX remote attestation, the caching service will need to import the platform’s SGX attestation collateral retrieved from Intel. This collateral will be used for both generating and verifying ECDSA quotes. Currently PCCS supports three caching fill methods.

- **LAZY** mode <br/>
  In this mode, when the caching service gets a retrieval request(PCK Cert, TCB, etc.) at runtime, it will look for the collaterals in its database to see if they are already in the cache. If they don't exist, it will contact the Intel PCS to retrieve the collaterals. This mode only works when internet connection is available.

- **REQ** mode <br/>
  In this method of filling the cache, the caching service will create a platform database entry when the caching service receives the registration requests. It will not return any data to the caller, but will contact the Intel PCS to retrieve the platform's collaterals if they are not in the cache. It will save the retrived collaterals in cache database for later use. This mode requires internet connection at deployment time. During runtime the caching service will use cache data only and will not contact Intel PCS.

- **OFFLINE** mode <br/>
  In this method of filling the cache, the caching service will not have access to the Intel hosted PCS service on the internet. It will create a platform database entry to save platform registration information sent by PCK Cert ID retrieval tool. It will provide an interface to allow an administration tool to retrieve the contents of the registration queue. The administrator tool will run on a platform that does have access to the internet. It can fetch platform collaterals from Intel PCS and send them to the caching service. The tool can be found at [SGXDataCenterAttestationPrimitives/tools/PccsAdminTool](https://github.com/intel/SGXDataCenterAttestationPrimitives/tree/master/tools/PccsAdminTool)

## <h3>Local service vs Remote service</h3>

You can run PCCS on localhost for product development or setup it as a public remote service in datacenter.
Typical setup flow for Local Service mode (Ubuntu 18.04 as example):

    1) Install Node.js via package manager (version 10.20 or later from official Node.js site)
    2) Request an API key from Intel's Provisioning Certificate Service
    3) Install PCCS through Debian package

You can test PCCS by running QuoteGeneration sample:

    1) Set "use_secure_cert": false in /etc/sgx_default_qcnl.conf
    2) Build and run QuoteGeneration sample and verify CertType=5 quote is generated

For Remote service mode, you must use a formal key and certificate pair. You should also change 'hosts' to 0.0.0.0 to accept remote connections. Also make sure the firewall is not blocking your listening port.
In /etc/sgx_default_qcnl.conf, set "use_secure_cert": true (For Windows see ../qpl/README.md)

## <h3>Manage the PCCS service</h3>

- If PCCS was installed by Debian package or RPM package

  1. Check status:<br/>
     $ sudo systemctl status pccs
  2. Start/Stop/Restart PCCS:<br/>
     $ sudo systemctl start/stop/restart pccs

- If PCCS was installed manually by current user, you can start it with the following command <br/>
  $ node -r esm pccs_server.js

## <h3>Uninstall</h3>

    If the PCCS service was installed through Debian/RPM package, you can use Linux package manager to uninstall it.
    If the PCCS service was installed manually, you can run below script to uninstall it:
        ./uninstall.sh (or uninstall.bat with administrator privilege on Windows)
