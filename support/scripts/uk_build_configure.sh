#!/bin/bash

OPT_BASENAME=`basename $0`
OPT_STRING="a:e:ho:p:"

read -r -d '' OPT_HELP  <<- EOH
	a - The application location
	e - The location of the external libraries
	p - The location of external platforms
	o - The output configuration file
	h - Print Usage
EOH

print_usage() {
	printf "%s [%s]\n" ${OPT_BASENAME} ${OPT_STRING};
	printf "%s\n" "${OPT_HELP}"
}

fetch_plats() {
	local files=;
	files=`find ${@} -maxdepth 1 -name "Config.uk"`
	echo ${files}
}

fetch_libs() {
	local files=;
	files=`find ${@} -name "Config.uk"`
	echo ${files}
}

fetch_app() {
	local files=;
	files=`find ${1} -name "Config.uk"`
	echo ${files}
}

config_out_create() {

	[[ -f ${2} ]] || touch ${2};

	for file in ${1}
	do
		[[ -z `cat ${2} | grep ${file}` ]] && \
			{ echo "source \"${file}\"" >> ${2}; }
	done
}

if [ $# -eq 0 ];
then
	print_usage
	exit 1;
fi

[[ -n ${CONFIG_UK_BASE} ]] && UK_BASE=${CONFIG_UK_BASE};
[[ -n ${UK_BASE} ]] || UK_BASE=$(readlink -f $(dirname $0)/../..)

CONFIG_FILES=;

while getopts ${OPT_STRING} opt
do
	case ${opt} in
	a)
		APP_DIR="${OPTARG}"
		[[ -d ${APP_DIR} ]] || \
			{ echo "Cannot find the application"; exit 1; }
		if [ ${UK_BASE} != ${APP_DIR} ]
		then
			CONFIG_FILES=$(fetch_app ${APP_DIR})
			echo ${CONFIG_FILES};
		else
			CONFIG_FILES=${BUILD_DIR}/app.uk
			[[ -f ${BUILD_DIR}/app.uk ]] || \
				{ touch ${CONFIG_FILES}; }
			echo '# external application' >> ${CONFIG_FILES}
			echo 'comment "No external application specified"'\
				 >> ${CONFIG_FILES}
			echo ${CONFIG_FILES};
		fi
		exit 0;
	;;
	e)
		CONFIG_FILES=`fetch_libs "${OPTARG}"`
	;;
	p)
		CONFIG_FILES=`fetch_plats "${OPTARG}"`
	;;
	h)
		print_usage;
		exit 0;
	;;
	o)
		CONFIG_OUT_FILE=${OPTARG}
	;;
	*)
		print_usage
		exit 1;
	;;
	esac
done

config_out_create "${CONFIG_FILES}" ${CONFIG_OUT_FILE}
echo ${CONFIG_OUT_FILE}
