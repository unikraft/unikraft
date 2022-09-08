## 1. Build container image
```
docker build -t pccs:my_tag .
```

## 2. Generate certificates to use with PCCS
```
mkdir -p ~/pccs_tls
cd ~/pccs_tls
openssl genrsa -out private.pem 2048
openssl req -new -key private.pem -out csr.pem
openssl x509 -req -days 365 -in csr.pem -signkey private.pem -out file.crt
rm -rf csr.pem
```
and give read access to the certificate/key in order they're to be readable inside container by user other than host files owner:
```
chmod 644 ~/pccs_tls/*
```

## 3. Fill up configuration file
Create directory for storing configuration file:
```
mkdir -p ~/config
```
Copy `<path_to_repo>/SGXDataCenterAttestationPrimitives/QuoteGeneration/pccs/config/default.json`
to this directory:
```
cp <path_to_repo>/SGXDataCenterAttestationPrimitives/QuoteGeneration/pccs/config/default.json ~/config/
```
Generate UserTokenHash:
```
echo -n "user_password" | sha512sum | tr -d '[:space:]-'
```
and AdminTokenHash:
```
echo -n "admin_password" | sha512sum | tr -d '[:space:]-'
```
and paste generated values into the `~/config/default.json`

Fill other required fields accordingly.

## 4. Run container
```
cd && \
docker run \
--user "65333:0" \
-v $PWD/pccs_tls/private.pem:/opt/intel/pccs/ssl_key/private.pem \
-v $PWD/pccs_tls/file.crt:/opt/intel/pccs/ssl_key/file.crt \
-v $PWD/config/default.json:/opt/intel/pccs/config/default.json \
-p 8081:8081  --name pccs -d pccs:my_tag
```

## 5 . Check if pccs service is running and available:
```
docker logs -f pccs
```

Output:

```
2021-08-01 20:54:24.700 [info]: DB Migration -- Update pcs_version table
2021-08-01 20:54:24.706 [info]: DB Migration -- update pck_crl.pck_crl from HEX string to BINARY
2021-08-01 20:54:24.709 [info]: DB Migration -- update pcs_certificates.crl from HEX string to BINARY
2021-08-01 20:54:24.711 [info]: DB Migration -- update platforms(platform_manifest,enc_ppid) from HEX string to BINARY
2021-08-01 20:54:24.713 [info]: DB Migration -- update platforms_registered(platform_manifest,enc_ppid) from HEX string to BINARY
2021-08-01 20:54:24.715 [info]: DB Migration -- Done.
2021-08-01 20:54:24.831 [info]: HTTPS Server is running on: https://localhost:8081

```

Execute command:
```
curl -kv https://localhost:8081
```
to check if pccs service is available.

## 6. Stop container:
```
docker stop pccs
```

