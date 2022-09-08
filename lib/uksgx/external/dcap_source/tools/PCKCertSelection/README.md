Intel(R) Software Guard Extensions Data Center Attestation Primitives (Intel(R) SGX DCAP) PCK Cert Selection Library Quick Start Guide
================================================

# SGX PCK Cert Selection Library

Reference implementation of PCK Cert Selection in SGX ECDSA model.

This library is an interface encapsulating all the processing involved in PCK Cert Selection.
It requires providing Intel-issued PCK Certificates and TCB Information corresponding to the platform that is attested.

## SGX PCK Cert Selection Sample App
This repository contains also a sample application meant to present the way dynamic-link PCK Cert Selection application is implemented.
Sample Application can be used to perform PCK Cert Selection using PCK Cert Selection Library.

## Documentation.
Project code is documented using doxygen style.
Doxygen configuration file is located under doc folder. It can be used to generate HTML documentation.
To generate documentation run: 
````
$ doxygen pck_cert_selection.doxy
```` 
From `doc` folder, or open `pck_cert_selection.doxy` using doxywizard and generate documentation.
For more info visit: http://www.doxygen.nl/index.html.

## Build
Requirements:
The library is using the AttestationParsers code from the QuoteVerification\QVL project in this repository.

Linux:
* make
* gcc 
* bash shell

Prebuild openssl v1.1.1d in prebuilt/openssl will be used for Linux & Windows




### Build on Linux:
#### Release mode:
````
$ make
````
#### Debug mode:
````
$ make debug
````

Both Library and Sample Application binaries are built.
Binaries to be found in `out`

### Build on Windows:
````
VS: Open PCKCertSelection.sln, select Release or Debug.
````

Both Library and Sample Application binaries are built.
Binaries to be found in `x64/Release` or `x64/Debug`

## Run SGX PCK Cert Selection Sample App

### Linux
From out directory run:
````
$ LD_LIBRARY_PATH=. ./PCKSelectionSample
````
### Windows
From output directory run:
````
PCKSelectionSample.exe
````

### Provided sample data
Build includes sample data for SGX PCK Cert Selection Sample App in `SampleData` directory. All files use default names so they will be loaded by app without any parameters.
