#!/bin/bash

# pack.sh
# Create PCK Cert Selection library package

# local variables 
ROOT_DIR=$(pwd)
BIN_DIR=$ROOT_DIR/out
INC_DIR=$ROOT_DIR/include
TRUNK_DIR=$ROOT_DIR/../../../..
ROOT_DIR=$ROOT_DIR/../../

SGX_VERSION=$(awk '/STRFILEVER/ {print $3}' ${ROOT_DIR}/QuoteGeneration/common/inc/internal/se_version.h|sed 's/^\"\(.*\)\"$/\1/')




# zip tool
ZIP=zip
ZIP_CMD=-r
ZIP_OPT=-j

# files to archive 
SO_FILE=libPCKCertSelection.so
INC_FILE=pck_cert_selection.h
README_FILE=README.txt
EULA_FILE="INTEL Software License Agreement 11.2.17.pdf"
SAMPLE_DATA=SampleData

# output file
ZIP_FILE=PCKCertSelectionLinux_$SGX_VERSION.zip

# copy files to root dir
cp $BIN_DIR/$SO_FILE .
cp $INC_DIR/$INC_FILE .

# archive
$ZIP $BIN_DIR/$ZIP_FILE $ZIP_OPT $SO_FILE $INC_FILE "$README_FILE" "$EULA_FILE"
$ZIP $BIN_DIR/$ZIP_FILE $ZIP_CMD $SAMPLE_DATA

# delete copied files
rm $SO_FILE $INC_FILE
