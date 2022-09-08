#!/bin/bash

arg1=$1
argnum=$#
## Set mydir to the directory containing the script
mydir="${0%/*}"
configFile="$mydir"/config/default.json
YELLOW='\033[1;33m'
RED='\033[1;31m'
NC='\033[0m'

function version_gt() { test "$(printf '%s\n' "$@" | sort -V | head -n 1)" != "$1"; }

# check nodejs version
function checkDependencies() {
    echo "Checking nodejs version ..."
    expected_node_v="v10.20.0"
    if which node > /dev/null
    then
        cur_node_v=$(node -v)
        if version_gt $cur_node_v $expected_node_v; then
            echo "nodejs is installed, continue..."
        else
            echo -e "${RED}The minimum node.js version required is ${expected_node_v}. Installation aborted. ${NC} "
            exit 1
        fi
    else
        echo -e "${RED}Node.js not installed. Please install Node.js version ${expected_node_v} or later. ${NC} "
        exit 1
    fi

    echo "Checking cracklib-runtime ..."
    if ! which cracklib-check > /dev/null
    then
        echo -e "${RED}cracklib-runtime not installed. Please install cracklib-runtime first and then retry. ${NC} "
        exit 1
    fi
}

function checkPCKSelectionLib() {
    if [ ! -f "$mydir"/lib/libPCKCertSelection.so ]; then
        echo -e "${YELLOW}Warning: lib/libPCKCertSelection.so not found. ${NC} "
    fi
}

function promptDbMigration() {
    auto_update_db=""
    echo -e "${YELLOW}Warning: If you are upgrading PCCS from an old release, the existing cache database will be updated automatically. ${NC} "
    echo -e "${YELLOW}         It's strongly recommended to backup your existing cache database first and then continue the installation. ${NC} "
    echo -e "${YELLOW}         For DCAP releases 1.8 and earlier, the cache database can't be updated so you need to delete it manually. ${NC} "
    while [ "$auto_update_db" == "" ]
    do
        read -p "Do you want to install PCCS now? (Y/N) :" auto_update_db 
        if [[ "$auto_update_db" == "Y" || "$auto_update_db" == "y" ]] 
        then
            break
        elif [[ "$auto_update_db" == "N" || "$auto_update_db" == "n" ]] 
        then
            exit 1
        else
            auto_update_db=""
        fi
    done
}

checkDependencies
promptDbMigration

#Ask for proxy server
echo "Check proxy server configuration for internet connection... "
if [ "$http_proxy" == "" ]
then
    read -p "Enter your http proxy server address, e.g. http://proxy-server:port (Press ENTER if there is no proxy server) :" http_proxy 
fi
if [ "$https_proxy" == "" ]
then
    read -p "Enter your https proxy server address, e.g. http://proxy-server:port (Press ENTER if there is no proxy server) :" https_proxy 
fi

cd `dirname $0`
npm config set proxy $http_proxy
npm config set http-proxy $http_proxy
npm config set https-proxy $https_proxy
npm config set engine-strict true
npm install
[ $? -eq 0 ] || exit $?;

doconfig=""
while [ "$doconfig" == "" ]
do
    read -p "Do you want to configure PCCS now? (Y/N) :" doconfig 
    if [[ "$doconfig" == "Y" || "$doconfig" == "y" ]] 
    then
        break
    elif [[ "$doconfig" == "N" || "$doconfig" == "n" ]] 
    then
        #Check PCK Cert Selection Library
        checkPCKSelectionLib
        exit 0
    else
        doconfig=""
    fi
done

#Ask for HTTPS port number
port=""
while :
do
    read -p "Set HTTPS listening port [8081] (1024-65535) :" port
    if [ -z $port ]; then 
        port=8081
        break
    elif [[ $port -lt 1024  ||  $port -gt 65535 ]] ; then
        echo -e "${YELLOW}The port number is out of range, please input again.${NC} "
    else
        sed "/\"HTTPS_PORT\"*/c\ \ \ \ \"HTTPS_PORT\" \: ${port}," -i ${configFile}
        break
    fi
done

#Ask for HTTPS port number
local_only=""
while [ "$local_only" == "" ]
do
    read -p "Set the PCCS service to accept local connections only? [Y] (Y/N) :" local_only 
    if [[ -z $local_only  || "$local_only" == "Y" || "$local_only" == "y" ]] 
    then
        local_only="Y"
        sed "/\"hosts\"*/c\ \ \ \ \"hosts\" \: \"127.0.0.1\"," -i ${configFile}
    elif [[ "$local_only" == "N" || "$local_only" == "n" ]] 
    then
        sed "/\"hosts\"*/c\ \ \ \ \"hosts\" \: \"0.0.0.0\"," -i ${configFile}
    else
        local_only=""
    fi
done

#Ask for API key 
apikey=""
while :
do
    read -p "Set your Intel PCS API key (Press ENTER to skip) :" apikey 
    if [ -z $apikey ]
    then
        echo -e "${YELLOW}You didn't set Intel PCS API key. You can set it later in config/default.json. ${NC} "
        break
    elif [[ $apikey =~ ^[a-zA-Z0-9]{32}$ ]] && sed "/\"ApiKey\"*/c\ \ \ \ \"ApiKey\" \: \"${apikey}\"," -i ${configFile}
    then
        break
    else
        echo "Your API key is invalid. Please input again. "
    fi
done

if [ "$https_proxy" != "" ]
then
    sed "/\"proxy\"*/c\ \ \ \ \"proxy\" \: \"${https_proxy}\"," -i ${configFile}
fi

#Ask for CachingFillMode
caching_mode=""
while [ "$caching_mode" == "" ]
do
    read -p "Choose caching fill method : [LAZY] (LAZY/OFFLINE/REQ) :" caching_mode 
    if [[ -z $caching_mode  || "$caching_mode" == "LAZY" ]] 
    then
        caching_mode="LAZY"
        sed "/\"CachingFillMode\"*/c\ \ \ \ \"CachingFillMode\" \: \"${caching_mode}\"," -i ${configFile}
    elif [[ "$caching_mode" == "OFFLINE" || "$caching_mode" == "REQ" ]] 
    then
        sed "/\"CachingFillMode\"*/c\ \ \ \ \"CachingFillMode\" \: \"${caching_mode}\"," -i ${configFile}
    else
        caching_mode=""
    fi
done

#Ask for administrator password
admintoken1=""
admintoken2=""
admin_pass_set=false
cracklib_limit=4
while [ "$admin_pass_set" == false ]
do
    while test "$admintoken1" == ""
    do
        read -s -p "Set PCCS server administrator password:" admintoken1
        printf "\n"
    done
    
    # check password strength
    result="$(cracklib-check <<<"$admintoken1")"
    okay="$(awk -F': ' '{ print $NF}' <<<"$result")"
    if [[ "$okay" != "OK" ]]; then
        if [ "$cracklib_limit" -gt 0 ]; then
            echo -e "${RED}The password is too weak. Please try again($cracklib_limit opportunities left).${NC}"
            admintoken1=""
            cracklib_limit=$(expr $cracklib_limit - 1)
            continue
        else
            echo "Installation aborted. Please try again."
            exit 1
        fi
    fi

    while test "$admintoken2" == ""
    do
        read -s -p "Re-enter administrator password:" admintoken2
        printf "\n"
    done

    if test "$admintoken1" != "$admintoken2"
    then
        echo "Passwords don't match."
        admintoken1=""
        admintoken2=""
        cracklib_limit=4
    else
        HASH="$(echo -n "$admintoken1" | sha512sum | tr -d '[:space:]-')"
        sed "/\"AdminTokenHash\"*/c\ \ \ \ \"AdminTokenHash\" \: \"${HASH}\"," -i ${configFile}
        admin_pass_set=true
    fi
done

#Ask for user password
cracklib_limit=4
usertoken1=""
usertoken2=""
user_pass_set=false
while [ "$user_pass_set" == false ]
do
    while test "$usertoken1" == ""
    do
        read -s -p "Set PCCS server user password:" usertoken1
        printf "\n"
    done

    # check password strength
    result="$(cracklib-check <<<"$usertoken1")"
    okay="$(awk -F': ' '{ print $NF}' <<<"$result")"
    if [[ "$okay" != "OK" ]]; then
        if [ "$cracklib_limit" -gt 0 ]; then
            echo -e "${RED}The password is too weak. Please try again($cracklib_limit opportunities left).${NC}"
            usertoken1=""
            cracklib_limit=$(expr $cracklib_limit - 1)
            continue
        else
            echo "Installation aborted. Please try again."
            exit 1
        fi
    fi

    while test "$usertoken2" == ""
    do
        read -s -p "Re-enter user password:" usertoken2
        printf "\n"
    done

    if test "$usertoken1" != "$usertoken2"
    then
        echo "Passwords don't match."
        usertoken1=""
        usertoken2=""
        cracklib_limit=4
    else
        HASH="$(echo -n "$usertoken1" | sha512sum | tr -d '[:space:]-')"
        sed "/\"UserTokenHash\"*/c\ \ \ \ \"UserTokenHash\" \: \"${HASH}\"," -i ${configFile}
        user_pass_set=true
    fi
done

if which openssl > /dev/null 
then 
    genkey=""
    while [ "$genkey" == "" ]
    do
        read -p "Do you want to generate insecure HTTPS key and cert for PCCS service? [Y] (Y/N) :" genkey 
        if [[ -z "$genkey" ||  "$genkey" == "Y" || "$genkey" == "y" ]] 
        then
            if [ ! -d ssl_key  ];then
                mkdir ssl_key
            fi
            openssl genrsa -out ssl_key/private.pem 2048
            openssl req -new -key ssl_key/private.pem -out ssl_key/csr.pem
            openssl x509 -req -days 365 -in ssl_key/csr.pem -signkey ssl_key/private.pem -out ssl_key/file.crt
            break
        elif [[ "$genkey" == "N" || "$genkey" == "n" ]] 
        then
            break
        else
            genkey=""
        fi
    done
else
    echo -e "${YELLOW}You need to setup HTTPS key and cert for PCCS to work. For how-to please check README. ${NC} "
fi

#Check PCK Cert Selection Library
checkPCKSelectionLib
